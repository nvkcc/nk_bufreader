#include "nk_bufread.h"

#include <nk_log.h>

#include <string.h>

/// Invalidates the entire buffer, ready to use as a fresh read buffer.
void nk_buf_reader_clear(nk_buf_reader *r) {
    r->newl = NULL;
    r->end_of_buf = r->buf;
    if (r->len > 0) {
        *r->end_of_buf = '\0';
    }
}

void nk_buf_reader_init(nk_buf_reader *r) { nk_buf_reader_clear(r); }

/// Shifts the buffer such that the `r->buf` points to the start of the next
/// line to read, and replace the newline character with a NUL character. Does
/// no I/O.
void nk_buf_reader_shift(nk_buf_reader *r) {
    // There is no newline in the buffer. That means it is either uninitialized,
    // or we would have read the entire buffer in the last iteration. Either
    // way, the whole buffer is now invalidated and so we can clear it.
    if (r->newl == NULL || r->newl + 1 >= r->buf + r->len) {
        nklog_trace("shift: newl not found. resetting.");
        nk_buf_reader_clear(r);
        return;
    }

    // At this point, the newline character is anywhere from the 0th index to
    // the second-last valid index of the buffer. Move it to the start.

    // +1 to skip the newline character.
    int diff = r->newl + 1 - r->buf;
    memmove(r->buf, r->newl + 1, r->len - diff);
    r->end_of_buf -= diff;

    r->newl = (char *)memchr(r->buf, '\n', r->end_of_buf - r->buf);
    if (r->newl) {
        // Set the newline character to NUL, since we're not reading it.
        *r->newl = '\0';
    }
}

int nk_buf_reader_next(nk_buf_reader *r) {
    nklog_trace("Called next()");
    int n, bytes_to_read;

    nk_buf_reader_shift(r);
    if (r->newl != NULL) {
        nklog_trace("Exit ok[1] with \"%s\"", r->buf);
        return NK_BUFREAD_OK;
    }
    // At this point, `r->newl == NULL`, and so there is no newline found.

    nklog_trace("End of buffer after shift = %d", r->end_of_buf - r->buf);

    n = r->len - (r->end_of_buf - r->buf) - 1;
    if (n == 0) {
        nklog_error("WARNING: nk_buf_reader ran out of space[1].");
        return NK_BUFREAD_INSUFFICIENT_SPACE;
    }
    nklog_error("Trying to read %d bytes", n);
    if ((n = read(r->fd, r->end_of_buf, n)) == 0) {
        return NK_BUFREAD_ITER_OVER;
    }
    if (n == -1) {
        return NK_BUFREAD_IO_ERROR;
    }
    r->end_of_buf += n;
    *r->end_of_buf = '\0';
    nklog_trace("End of buffer after read = %d", r->end_of_buf - r->buf);

    r->newl = (char *)memchr(r->buf, '\n',
                             sizeof(char) * (r->end_of_buf - r->buf + 1));
    if (r->newl) {
        *r->newl = '\0';
        return NK_BUFREAD_OK;
    } else {
        nklog_error("WARNING: nk_buf_reader ran out of space[2].");
        return NK_BUFREAD_INSUFFICIENT_SPACE;
    }

    // nklog_trace("Exit ok[2] with \"%s\"", r->buf);
    //
    // return NK_BUFREAD_OK;
}
