#include "nk_bufread.h"

#include <nk_log.h>

#include <string.h>

void nk_buf_reader_init(nk_buf_reader *out, int fd, int buf_len) {
    out->fd = fd;
    out->newl = NULL;
    out->end_of_buf = out->buf;
    out->buf_len = buf_len;
    if (buf_len > 0) {
        *out->end_of_buf = '\0';
    }
}

/// Shifts the buffer such that the `r->buf` points to the start of the next
/// line to read, and replace the newline character with a NUL character. Does
/// no I/O.
void nk_buf_reader_shift(nk_buf_reader *r) {
    // Haven't even started as the position of the newline isn't computed yet.
    if (r->newl == NULL) {
        return;
    }
    // Newline character is the last valid character in the buffer. Just begin
    // a fresh read.
    if (r->newl + 1 == r->buf + r->buf_len) {
        r->newl = NULL;
        return;
    }

    // At this point, the newline character is anywhere from the 0th index to
    // the second-last valid index of the buffer. Move it to the start.

    // +1 to skip the newline character.
    int diff = r->newl + 1 - r->buf;
    memmove(r->buf, r->newl + 1, r->buf_len - diff);
    r->end_of_buf -= diff;

    r->newl = (char *)memchr(r->buf, '\n', r->end_of_buf - r->buf);
    if (r->newl) {
        *r->newl = '\0';
    }
}

int nk_buf_reader_next(nk_buf_reader *r) {
    int n, bytes_to_read;

    nk_buf_reader_shift(r);
    if (r->newl != NULL) {
        return NK_BUFREAD_OK;
    }
    // At this point, `r->newl == NULL`, and so there is no newline found.

    nklog_trace("EOB after shift = %d", (int)(r->end_of_buf - r->buf));

    n = r->buf_len - (r->end_of_buf - r->buf) - 1;
    if (n == 0) {
        nklog_error("WARNING: nk_buf_reader ran out of space.");
        return NK_BUFREAD_INSUFFICIENT_SPACE;
    }

    nklog_trace("Block to read...");
    n = read(r->fd, r->end_of_buf, n);
    nklog_trace("read() call returned");

    if (n == 0) {
        return NK_BUFREAD_ITER_OVER;
    } else if (n == -1) {
        return NK_BUFREAD_IO_ERROR;
    }
    r->end_of_buf += n;
    *r->end_of_buf = '\0';

    r->newl = (char *)memchr(r->buf, '\n', r->end_of_buf - r->buf);
    if (r->newl) {
        *r->newl = '\0';
    }

    return 0;
}
