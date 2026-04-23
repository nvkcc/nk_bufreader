#include "nk_bufreader.h"

#include <nk_log.h>

#include <string.h>

void nk_bufreader_init(nk_bufreader *r) {
    // Initialize all three pointers to be the same.
    r->left = r->right = r->end = r->buf;
    // Set the last byte to NUL to prevent any case of overflow by string
    // reading. We shall never touch this byte again.
    *(r->buf + r->len - 1) = '\0';
    r->err = NK_BUFREAD_OK;
}

// Uncomment this line only in development. Remove it in production.
#define NK_BUFREAD_DEBUG_PRINT
#ifdef NK_BUFREAD_DEBUG_PRINT
#define I(P) (int)(P - r->buf)
#define REMAIN_B(P) (r->len - (P - r->buf) - 1)
#include <stdio.h>
#define debug_print(r)                                                         \
    {                                                                          \
        fprintf(stderr, "inner [");                                            \
        int i;                                                                 \
        for (i = 0; i < r->len; ++i) {                                         \
            fprintf(stderr, "%d", r->buf[i]);                                  \
            if (i + 1 < r->len) {                                              \
                fprintf(stderr, ", ");                                         \
            }                                                                  \
        }                                                                      \
        fprintf(stderr, "] (%d, %d, %d)\n", I(r->left), I(r->right),           \
                I(r->end));                                                    \
    }
#else
#define debug_print(r)
#endif

// [ 0   1   2   3   4   5   6   7   8   9 ]
//   ^^^^valid^^^^   ^end                ^invariant NUL byte
// Consider a buffer of length 10, and the `end` points to index 4 (i.e.: a
// VALID_LEN of 4). Then we can only afford to read 5 bytes (the 5 being
// 4..=8) because we need to keep buf[9] as the NUL byte.
//
// That arithmetic, 5 = 10 - 4 - 1, leads us here:
#define VALID_LEN(r) (r->end - r->buf)
#define BYTES_TO_READ(r) (r->len - VALID_LEN(r) - 1)
// Note that this function returns 0 if and only if `end` points to
// index 9 in the above scenario, which is when it is the last address in
// the allocated buffer.

// Let S := r->buf, A := r->newl, B := r->end.
// The assumption is that we've just got done consuming (externally) the data in
// buffer[S..A]. So here's what we'll do:
// (1.) Clear out the old data by doing memmove S <- A.
// (2.) Look for the next newline character. If one is found, then we NUL that
//      and return.
// (3.) Read more data from the file.
// (4.) Look for the next newline character.
//      (4a.) If one is found, then we NUL that and return.
//      (4b.) Else, the buffer is too small. Send error.
//

//

/// Moves `r->left` to the front, and moves all other points accordingly.
void nk_bufreader_shift(nk_bufreader *r) {
    if (r->left >= r->end) {
        return;
    }
    debug_print(r);
    nklog_trace("memmove()");
    memmove(r->buf, r->left, r->end - r->left);
    debug_print(r);
    r->end -= r->left - r->buf;
}

/// Reads as many bytes as we can into the buffer, while never touching the last
/// byte to keep it set to NUL. Advances the `r->end` pointer if necessary.
inline void nk_bufreader_read(nk_bufreader *r) {
    int n;
    switch ((n = read(r->fd, r->end, REMAIN_B(r->end)))) {
    case 0:
        nklog_trace("read() returned 0. Iteration over.");
        r->err = NK_BUFREAD_ITER_OVER;
        break;
    case -1:
        nklog_trace("read() returned -1. I/O error.");
        r->err = NK_BUFREAD_IO_ERROR;
        break;
    default:
        r->end += n;
    }
}

/*
r->left   The byte that the previous consumption read from.
r->right  The first '\n' byte to the left of `r->left`. NULL implies that there
          are no '\n' bytes to the left of `r->left` in the buffer.
r->end    The first invalid byte. Suitable as destination for read() calls.


INVARIANTS
    1. Last byte in buffer should ALWAYS be zero.

POLICIES
    1. Only read more data when LEFT has nowhere to go.

ALGORITHM
    1. Try to advance LEFT to where RIGHT is.
        If (found)
            advance RIGHT (may be null).
        Else
            Gotta read more data.

 */

// "*left" points to the first byte to consume.
// "*right" points to the byte after the first '\n' char.
// "*end" points to the byte after the last char from `fd`.
int nk_bufreader_next(nk_bufreader *r) {
    // If this reader already terminated before, then return that same error.
#define RETURN_LAST_ERR_IF_FOUND                                               \
    if (r->err != NK_BUFREAD_OK) {                                             \
        nklog_info("\x1b[31mReturn last error code\x1b[m [%d]", r->err);       \
        return r->err;                                                         \
    }
    RETURN_LAST_ERR_IF_FOUND;
    if (r->right == NULL) {
        // Even after the previous iteration, we couldn't find any '\n' for
        // r->right to point to -> the '\n' to the left of r->left is the last
        // '\n'. End iteration.
        nklog_info("\x1b[31mReturn\x1b[m #1");
        return (r->err = NK_BUFREAD_ITER_OVER);
    }

    nklog_trace("Call next()");
    debug_print(r);

    // Handle first iteration separately for now.
    if (r->end == r->buf) {
        nklog_trace("First iteration");
        nk_bufreader_read(r);
        RETURN_LAST_ERR_IF_FOUND;
        r->right = memchr(r->left, '\n', REMAIN_B(r->left));
        if (r->right == NULL) {
            nklog_info("\x1b[31mReturn\x1b[m #F0");
            r->err = NK_BUFREAD_ITER_OVER;
            return NK_BUFREAD_OK;
        } else {
            nklog_info("\x1b[31mReturn\x1b[m #F1");
            return NK_BUFREAD_OK;
        }
    }

    nklog_trace("Non-first");
    // Not on the first iteration.
    r->left = r->right + 1;
    r->right = memchr(r->left, '\n', REMAIN_B(r->left));

    if (r->right) {
        nklog_info("\x1b[31mReturn\x1b[m #2");
        return NK_BUFREAD_OK;
    }

    if (r->right == NULL) {
        nklog_trace("No newlines found");
        debug_print(r);
        nk_bufreader_shift(r);
        nk_bufreader_read(r);
        nklog_trace("(Shift + Read) complete");
        debug_print(r);
        //     RETURN_LAST_ERR_IF_FOUND;
        //     r->right = memchr(r->left, '\n', REMAIN_B(r->left));
        //     if (r->right == NULL) {
        //         *r->end = '\0';
        //         nklog_info("\x1b[31mReturn\x1b[m #4");
        //         return NK_BUFREAD_OK;
        //     }
    }
    return NK_BUFREAD_OK;
    // // At this line, r->right should point to a '\n'.
    // // *r->right = '\0';
    // nklog_info("\x1b[31mReturn\x1b[m #5");
    // return NK_BUFREAD_OK;
}
