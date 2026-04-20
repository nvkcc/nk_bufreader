#include <nk_log.h>
#include <string.h>
#include <unistd.h>

/// A buffered reader that reads up until the newline character. The buffer must
/// be long enough to contain the longest line to be read.
typedef struct {
  /// File descriptor to read from.
  int fd;
  /// Byte buffer.
  char *const buf;
  /// Points to the first newline in the buffer.
  char *newl;
  /// Points to the first invalid character of the buffer.
  char *end_of_buf;
  /// Byte buffer length.
  int buf_len;
} gitnv_buf_reader;

/// Initialize this by first setting `buf` with the {.buf = ...}, and then
/// calling this method. This will start with with an empty buffer of NUL
/// characters.
void gitnv_buf_reader_new(gitnv_buf_reader *out, int fd, int buf_len) {
  out->fd = fd;
  out->newl = NULL;
  out->end_of_buf = out->buf;
  out->buf_len = buf_len;
}

/// Shifts the buffer such that the `r->buf` points to the start of the next
/// line to read, and replace the newline character with a NUL character. Does
/// no I/O.
void gitnv_buf_reader_shift(gitnv_buf_reader *r) {
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
  r->newl = memchr(r->buf, '\n', r->end_of_buf - r->buf);
  if (r->newl) {
    *r->newl = '\0';
  }
}

/// Reads data from `r`, and puts it at the front of the buffer `r.buf`. There
/// is guaranteed to be a NUL character within the memory allocated to `r.buf`.
///
/// Returns 0 upon success, and -1 if any error is encountered (including
/// reaching end of file). `errno` would be set as per the result of the call to
/// `read`.
int gitnv_buf_reader_next(gitnv_buf_reader *r) {
  int n, bytes_to_read;

  gitnv_buf_reader_shift(r);
  if (r->newl != NULL) {
    return 0;
  }
  // At this point, `r->newl == NULL`, and so there is no newline found.

  nklog_trace("EOB after shift = %d", (int)(r->end_of_buf - r->buf));

  n = r->buf_len - (r->end_of_buf - r->buf) - 1;
  if (n == 0) {
    SEND_STDERR_LN("\x1b[33mWARNING: gitnv_buf_reader ran out of space.\x1b[m");
    return -1;
  }

  n = read(r->fd, r->end_of_buf, n);
  if (n == 0) {
    nklog_trace("End of file! (n = %d)", n);
    // Successful read, and reached end of file.
    return -1;
  } else if (n == -1) {
    nklog_trace("Error! (n = %d)", n);
    // `read` has set errno to indicate the error.
    return -1;
  }
  r->end_of_buf += n;
  *r->end_of_buf = '\0';

  nklog_trace("Executed `read`, bytes read = %d", n);
  nklog_trace("EOB after read = %d", (int)(r->end_of_buf - r->buf));

  r->newl = memchr(r->buf, '\n', r->end_of_buf - r->buf);
  if (r->newl) {
    nklog_trace("Found newline!");
    *r->newl = '\0';
  } else {
    nklog_trace("No newline immediately after read!");
  }

  return 0;
  // int x = errno;
  // switch (errno) {
  // case EAGAIN:
  //     return 0;
  // default:
  //     return 0;
  // }
}
