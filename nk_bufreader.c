#include "nk_bufreader.h"

#include <nk_log.h>

#include <string.h>

#define VALID_LEN(r) (r->end - r->buf)

void nk_bufreader_init(nk_bufreader *r) {
    // Initialize all three pointers to be the same.
    r->newl = r->end = r->buf;
    // Set the last byte to NUL to prevent any case of overflow by string
    // reading. We shall never touch this byte again.
    *(r->buf + r->len - 1) = '\0';
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
        fprintf(stderr,                                                        \
                "] (newl=\x1b[33m%d\x1b[m, valid_len=\x1b[32m%d\x1b[m)\n",     \
                r->newl ? (int)(r->newl - r->buf) : -1, (int)VALID_LEN(r));    \
    }
#else
#define debug_print(r)
#endif

inline int nk_bufreader_bytes_to_read(nk_bufreader *r) {
    //  0   1   2   3   4   5   6   7   8   9
    // Consider a buffer of length 10, and the `end` points to index 4 (i.e.: a
    // VALID_LEN of 4). Then we can only afford to read 5 bytes (the 5 being
    // 4..=8) because we need to keep buf[9] as the NUL byte.
    //
    // That arithmetic, 5 = 10 - 4 - 1, leads us here:
    return r->len - VALID_LEN(r) - 1;

    // Note that this function returns 0 if and only if `end` points to
    // index 9 in the above scenario, which is when it is the last address in
    // the allocated buffer.
}

/// Shifts `r->newl` to `r->buf` and updates `r->end` to remain correct.
void nk_bufreader_memmove(nk_bufreader *r) {
    if (r->newl == r->buf) {
        return;
    }
    // +1 to skip the newline character.
    int diff = r->newl + 1 - r->buf;
    memmove(r->buf, r->newl + 1, r->len - diff);
    r->end -= diff;
}

/// Appends data to `r->end`. Works as long the position of `r->end` is valid.
inline int nk_bufreader_append_data(nk_bufreader *r) {
    int n = read(r->fd, r->end, nk_bufreader_bytes_to_read(r));
    switch (n) {
    case 0: // No bytes were read, and end of file is reached.
        return 0;
    case -1: // An error occured in `read()`. See the `errno` variable.
        return NK_BUFREAD_IO_ERROR;
    default: // Successful read.
        *(r->end += n) = '\0';
        return n;
    }
}

// =============================================================================
// The main logic
// =============================================================================

/*
 * Recall: S := r->buf, A := r->newl, B := r->end
 * The assumption is that we've just got done consuming (externally) the data in
 * buffer[S..A]. So here's what we'll do:
 * (1.) Clear out the old data by doing memmove S <- A.
 * (2.) Look for the next newline character. If one is found, then we NUL that
 *      and return.
 * (3.) Read more data from the file.
 * (4.) Look for the next newline character.
 *      (4a.) If one is found, then we NUL that and return.
 *      (4b.) Else, the buffer is too small. Send error.
 */

//

int nk_bufreader_next(nk_bufreader *r) {
    if (!r->end) {
        return NK_BUFREAD_INVALID;
    }
    if (!r->newl && r->end > r->buf) {
        return NK_BUFREAD_ITER_OVER;
    }
    int n;

    // (1.)
    nk_bufreader_memmove(r);
    nklog_trace("Call memmove():");
    debug_print(r);

    // (2.)
    r->newl = (char *)memchr(r->buf, '\n', sizeof(char) * VALID_LEN(r));
    if (r->newl) {
        *r->newl = '\0';
        nklog_trace("\x1b[31mReturn\x1b[m {#1}");
        return NK_BUFREAD_OK;
    }
    nklog_trace("\x1b[33m1st\x1b[m memchr('\\n', v.len=%d):", VALID_LEN(r));
    debug_print(r);

    // (3.)
    nklog_trace("Call read(%d) at [%d]", nk_bufreader_bytes_to_read(r),
                VALID_LEN(r));
    if ((n = nk_bufreader_append_data(r)) <= 0) {
        if (r->end == r->buf) {
            nklog_trace("\x1b[31mReturn\x1b[m {#2}");
            return NK_BUFREAD_ITER_OVER;
        }
        nklog_trace("\x1b[31mReturn\x1b[m {#3}");
        return n;
    }
    debug_print(r);
    nklog_trace("read() returned %d", n);

    // (4.)
    nklog_trace("\x1b[34m2nd\x1b[m memchr('\\n', v.len=%d):", VALID_LEN(r));
    r->newl = (char *)memchr(r->buf, '\n', sizeof(char) * VALID_LEN(r));
    debug_print(r);
    if (r->newl) {
        if (r->newl + 2 == r->buf + r->len) {
            *r->buf = '\0';
            r->end = NULL;
            nklog_trace("\x1b[31mReturn\x1b[m {#4}");
            return NK_BUFREAD_INSUFFICIENT_SPACE;
        }
        *r->newl = '\0';
        nklog_trace("\x1b[31mReturn\x1b[m {#5}");
        return NK_BUFREAD_OK;
    } else if (VALID_LEN(r) + 1 >= r->len) {
        *r->buf = '\0';
        r->end = NULL;
        nklog_trace("\x1b[31mReturn\x1b[m {#6}");
        return NK_BUFREAD_INSUFFICIENT_SPACE;
    }
    nklog_trace("\x1b[31mReturn\x1b[m {#7}");
    return NK_BUFREAD_OK;
}

#undef VALID_LEN
