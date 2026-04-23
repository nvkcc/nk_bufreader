#include "nk_bufreader.h"

#include <nk_log.h>

#include <string.h>

void nk_bufreader_init(nk_bufreader *r) {
    // Initialize all three pointers to be the same.
    r->left = r->right = r->end = r->buf;
    // Set the last byte to NUL to prevent any case of overflow by string
    // reading. We shall never touch this byte again.
    *(r->buf + r->len - 1) = '\0';
}

// Uncomment this line only in development. Remove it in production.
#define NK_BUFREAD_DEBUG_PRINT
#ifdef NK_BUFREAD_DEBUG_PRINT
#define I(P) (int)(P - r->buf)
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

// "*left" points to the first byte to consume.
// "*right" points to the byte after the first '\n' char.
// "*end" points to the byte after the last char from `fd`.
int nk_bufreader_next(nk_bufreader *r) {
    nklog_trace("Call next()");
#define REMAINING_BYTES(P) (r->len - (P - r->buf) - 1)
    if (r->last_error_code != NK_BUFREAD_OK) {
        nklog_info("\x1b[31mReturn\x1b[m #0");
        return r->last_error_code;
    }
    if (r->right == NULL) {
        // This means that even after the previous iteration's read, we couldn't
        // find any '\n' for r->right to go. Implies that the '\n' to the left
        // of r->left was the last '\n'. End iteration.
        nklog_info("\x1b[31mReturn\x1b[m #1");
        return (r->last_error_code = NK_BUFREAD_ITER_OVER);
    }

    r->left = r->right + 1;
    r->right = memchr(r->left, '\n', REMAINING_BYTES(r->left));
    if (r->right == NULL) {
        nk_bufreader_shift(r);
        nklog_trace("Gonna \x1b[33mread\x1b[m %d bytes at [%d]",
                    REMAINING_BYTES(r->end), I(r->end));
        int n = read(r->fd, r->end, REMAINING_BYTES(r->end));
        if (n == -1) {
            return (r->last_error_code = NK_BUFREAD_IO_ERROR);
        } else if (n == 0) {
            return (r->last_error_code = NK_BUFREAD_ITER_OVER);
        }
        nklog_trace("End shift-right: %d -> %d", I(r->end), I(r->end + n));
        r->end += n;
        nklog_trace("read() returned: %d", n);
        r->right = memchr(r->left, '\n', REMAINING_BYTES(r->left));
        if (r->right == NULL) {
            *r->end = '\0';
            nklog_info("\x1b[31mReturn\x1b[m #2");
            return NK_BUFREAD_OK;
        }
    }
    // At this line, r->right should point to a '\n'.
    *r->right = '\0';
    nklog_info("\x1b[31mReturn\x1b[m #3");
    return NK_BUFREAD_OK;
#undef REMAINING_BYTES
}

#undef VALID_LEN
#undef BYTES_TO_READ
