#define DEBUG_MODE

#include "nk_bufreader.h"
#include "log.h"
#include <string.h>
#include <unistd.h>

#define L (r->left)
#define R (r->right)
#define E (r->end)

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
#define debug_print(msg, r)                                                    \
    {                                                                          \
        fprintf(stderr, "inner (" msg ", len=%d) [", r->len);                  \
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
    log_trace("memmove(). [left=%d, end=%d]", I(r->left), I(r->end));
    memmove(r->buf, r->left, r->end - r->left);
    r->end -= r->left - r->buf;
    r->left = r->buf;
    debug_print("After memmove", r);
}

/// Reads as many bytes as we can into the buffer, while never touching the last
/// byte to keep it set to NUL. Advances the `r->end` pointer if necessary.
void nk_bufreader_read(nk_bufreader *r) {
    int n;
    switch ((n = read(r->fd, r->end, REMAIN_B(r->end)))) {
    case 0:
        log_trace("read() returned 0. Iteration over.");
        r->err = NK_BUFREAD_ITER_OVER;
        break;
    case -1:
        log_trace("read() returned -1. I/O error.");
        r->err = NK_BUFREAD_IO_ERROR;
        break;
    default:
        log_trace("read() returned %d.", n);
        r->end += n;
        debug_print("", r);
    }
}

/*
L   The byte that the previous consumption read from.
R   The byte to the right of the first '\n' after L. Minimally L + 1.
    NULL implies that there are no '\n' to the left of L in the buffer.
E   The first invalid byte. Suitable as destination for read() calls.

INVARIANTS
    1. Last byte in buffer should ALWAYS be zero.

POLICIES
    1. Only read more data when LEFT has nowhere to go.

ALGORITHM
    1.  If RIGHT is NULL, then the entire buffer was consumed the last
        iteration. Return NK_BUFREAD_ITER_OVER.
    2.  Advance LEFT to where RIGHT is.
    3.  Advance RIGHT to next '\n' char.
        If (found)
            Return
        Else
            Shift left and read more data, and try to advance again.

 */

// "*left" points to the first byte to consume.
// "*right" points to the byte after the first '\n' char.
// "*end" points to the byte after the last char from `fd`.
int nk_bufreader_next(nk_bufreader *r) {
    log_trace("\x1b[33m--- Call next() ---\x1b[m");
    // If this reader already terminated before, then return that same error.
    if (r->err != NK_BUFREAD_OK) {
        log_trace("Already terminated with [%d]", r->err);
        return r->err;
    }
    if (R == NULL) {
        // Even after the previous iteration, we couldn't find any '\n' for
        // r->right to point to -> the '\n' to the left of r->left is the last
        // '\n'. End iteration.
        log_trace("\x1b[31mReturn\x1b[m #1");
        debug_print("", r);
        return (r->err = NK_BUFREAD_ITER_OVER);
    }
    L = R;
    debug_print("#1", r);
    if ((R = memchr(L, '\n', E - L))) {
        R++;
        log_trace("Return #1");
        debug_print("", r);
        return NK_BUFREAD_OK;
    }
    nk_bufreader_shift(r);
    nk_bufreader_read(r);
    if ((R = memchr(L, '\n', E - L))) {
        R++;
        log_trace("Return #2");
        debug_print("", r);
        return NK_BUFREAD_OK;
    } else {
        r->err = NK_BUFREAD_ITER_OVER;
        log_trace("Return #3");
        debug_print("", r);
        return NK_BUFREAD_OK;
    }
}
