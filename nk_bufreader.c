#include "nk_bufreader.h"

#include <nk_log.h>

#include <string.h>

void nk_bufreader_init(nk_bufreader *r) {
    r->ptr = r->buf;
    // Set the last byte to NUL to prevent any case of overflow by string
    // reading. We shall never touch this byte again.
    *(r->buf + r->len - 1) = '\0';
    nklog_trace("Set position[%d] to NUL", r->len - 1);
    r->err = NK_BUFREAD_OK;
}

// Uncomment this line only in development. Remove it in production.
// #define NK_BUFREAD_DEBUG_PRINT
#ifdef NK_BUFREAD_DEBUG_PRINT
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
        fprintf(                                                               \
            stderr,                                                            \
            "] (newl=\x1b[33m%d\x1b[m, valid_len=\x1b[32m%d\x1b[m, n=%d)\n",   \
            r->newl ? (int)(r->newl - r->buf) : -1, (int)VALID_LEN(r), n);     \
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

int nk_bufreader_next(nk_bufreader *r) {
    if (r->err != NK_BUFREAD_OK) {
        nklog_trace("Return 0");
        return r->err;
    }
    int n;
    r->ptr = r->buf;

    while (1) {
        if (r->ptr + 1 == r->buf + r->len) {
            nklog_trace("Return 1");
            *r->buf = '\0';
            return (r->err = NK_BUFREAD_INSUFFICIENT_SPACE);
        }
        nklog_trace("Reading to position[%d]", r->ptr - r->buf);
        switch (read(r->fd, r->ptr, 1)) {
        case 1:
            if (*r->ptr == '\n') {
                *(r->ptr + 1) = '\0';
                return NK_BUFREAD_OK;
            }
            r->ptr++;
            continue;
        case 0:
            r->err = NK_BUFREAD_ITER_OVER;
            if (r->ptr > r->buf) {
                *(r->ptr - 1) = '\n';
                *r->ptr = '\0';
                nklog_trace("Return 3");
                return NK_BUFREAD_OK;
            } else {
                nklog_trace("Return 4");
                return NK_BUFREAD_ITER_OVER;
            }
        case -1:
            return NK_BUFREAD_IO_ERROR;
        default:
            nklog_error("Not supposed to be here");
            return NK_BUFREAD_INVALID;
        }
    }
}

#undef VALID_LEN
#undef BYTES_TO_READ
