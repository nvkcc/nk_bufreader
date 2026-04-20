#ifndef _NK_BUFREAD_H
#define _NK_BUFREAD_H 1

/// A buffered reader that reads up until the newline character. The buffer must
/// be long enough to contain the longest line to be read.
typedef struct nk_buf_reader {
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
} nk_buf_reader;

/// Initialize this by first setting `buf` with the {.buf = ...}, and then
/// calling this method. This will start with with an empty buffer of NUL
/// characters.
void nk_buf_reader_init(nk_buf_reader *, int fd, int buf_len);

/// Reads data from `r`, and puts it at the front of the buffer `r.buf`. There
/// is guaranteed to be a NUL character within the memory allocated to `r.buf`.
///
/// Returns 0 upon success, and -1 if any error is encountered (including
/// reaching end of file). `errno` would be set as per the result of the call to
/// `read`.
int nk_buf_reader_next(nk_buf_reader *);

#endif
