#include "nk_bufread.h"

#include <nk_log.h>

#include <string.h>

/// Invalidates the entire buffer, ready to use as a fresh read buffer.
inline void nk_buf_reader_clear(nk_buf_reader *r) {
    r->newl = NULL;
    r->end = r->buf;
    if (r->len > 0) {
        *r->end = '\0';
    }
}

void nk_buf_reader_init(nk_buf_reader *r) { nk_buf_reader_clear(r); }

#include <stdio.h>
void debug_print(nk_buf_reader *r) {
    fprintf(stderr, "inner [");
    int i;
    for (i = 0; i < r->len; ++i) {
        fprintf(stderr, "%d", r->buf[i]);
        if (i + 1 < r->len) {
            fprintf(stderr, ", ");
        }
    }
    fprintf(stderr, "] (%s)\n", r->buf);
}

inline int nk_buf_reader_bytes_to_read(nk_buf_reader *r) {
    // Say we have a buffer of length 16, and the `end` points to
    // index 6 (valid indices are 0..=15). Then we can only afford to read 9
    // bytes (the 9 being 6..=14) because we need to set buf[15] to the NUL
    // byte.
    //
    // That arithmetic, 9 = 16 - 6 - 1, leads us here:
    return r->len - (r->end - r->buf) - 1;

    // Note that this function returns 0 if and only if `end` points to
    // index 15 in the above scenario, which is when it is the last address in
    // the allocated buffer.
}

int nk_buf_reader_read_data(nk_buf_reader *r) {
    if (r->end + 1 == r->buf + r->len) {
        nklog_error("WARNING: buf reader ran out of space[1].");
        return NK_BUFREAD_INSUFFICIENT_SPACE;
    }
    int n = read(r->fd, r->end, nk_buf_reader_bytes_to_read(r));
    nklog_info("Read %d bytes to position[%d] -> %d",
               nk_buf_reader_bytes_to_read(r), r->end - r->buf, n);
    if (n == 0) {
        // Hit EOF. No data is read.
        nklog_trace("Iteration over");
        *r->end = '\0';
        return NK_BUFREAD_ITER_OVER;
    }
    if (n == -1) {
        // read() error'ed out. See the `errno` variable.
        nklog_trace("I/O error");
        return NK_BUFREAD_IO_ERROR;
    }
    // Update the `end` address. Note that after this advancement, it
    // still points to a byte that is not read from `fd`, thus preserving the
    // invariance.
    r->end += n;
    *r->end = '\0';
    nklog_trace("End of buffer after read: [%d]", r->end - r->buf);
    return NK_BUFREAD_OK;
}

int nk_buf_reader_next(nk_buf_reader *r) {
    nklog_trace("Called next()");
    int n, err;

    if (r->newl) {
        // Shift the buffer such that the `r->buf` points to the start of the
        // next line to read.

        // +1 to skip the newline character.
        int diff = r->newl + 1 - r->buf;
        memmove(r->buf, r->newl + 1, r->len - diff);

        // Update the invalidated states of r->newl and r->end.
        r->end -= diff;

        r->newl = (char *)memchr(r->buf, '\n', r->end - r->buf);
        if (r->newl) {
            // Set the newline character to NUL, since we're not reading it.
            *r->newl = '\0';
            nklog_trace("shift: found a newline without I/O.");
            return NK_BUFREAD_OK;
        }
        nklog_trace("shift: waiting for I/O.");
        // Here, r->newl is NULL and hence is in a temporarily invalid state.
        // i.e. there is no newline found in this line. We'll need to fetch more
        // data with `read()` before we do anything.
    } else {
        // There was no newline in the buffer. That means we would have consumed
        // the entire buffer in the last iteration.
        nklog_trace("shift: newl not found. resetting.");
        r->end = r->buf;
    }
    /* At this point, `r->newl == NULL`. */

    nklog_trace("End of buffer after shift = %d", r->end - r->buf);

    // Read the data.
    if ((err = nk_buf_reader_read_data(r)) != NK_BUFREAD_OK) {
        return err;
    }

    debug_print(r);

    r->newl = (char *)memchr(r->buf, '\n',
                             sizeof(char) * (r->end - r->buf + 1));
    if (r->newl) {
        nklog_info("Newline found");
        *r->newl = '\0';
        return NK_BUFREAD_OK;
    } else {
        nklog_warn("No newline found");
        return NK_BUFREAD_OK;
        // At this point, n > 0 so there is more data to be read, but then there
        // is no newline to be found, and so the buffer is not big enough to
        // contain the longest line.
        return NK_BUFREAD_INSUFFICIENT_SPACE;
    }
}
