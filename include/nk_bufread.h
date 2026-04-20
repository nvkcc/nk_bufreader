#ifndef _NK_BUFREAD_H
#define _NK_BUFREAD_H 1

#ifdef __cplusplus
extern "C" {
#endif

/// A buffered reader that reads up until the newline character. The buffer must
/// be two bytes longer than the longest line to be read, since we guarantee
/// that the buffer always contains the NUL byte. So one byte for the newline
/// char, and one byte for the NUL byte.
///
/// NOTE: after initializing with {.buf = ...}, please call `nk_buf_reader_init`
/// on the object initialize a valid state before doing any other operations.
/// This struct does not own any data so it need not be freed.
typedef struct nk_buf_reader {
    /// File descriptor to read from.
    const int fd;
    /// Byte buffer.
    char *const buf;
    /// Byte buffer length.
    const int len;
    /// Points to the first newline in the buffer. If this is NULL, then there
    /// is no newline in the buffer.
    char *newl;
    /// Points to the first invalid byte of the buffer. That is, the byte behind
    /// this pointer is not read from `fd`. However, this pointer still points
    /// to a valid place in the buffer's allocated memory. Should always be set
    /// to the NUL byte.
    char *end_of_buf;
} nk_buf_reader;

typedef enum {
    /**
     * No error occurred; the call was successful.
     */
    NK_BUFREAD_OK = 0,
    /**
     * The space allocated to the buffer is not enough to contain one of the
     * lines that the buffer is used to read. You have to allocate more space.
     */
    NK_BUFREAD_INSUFFICIENT_SPACE = 1,
    /**
     * There was an error returned when calling `read()`. Check `errno` for the
     * error that `read()` left behind.
     */
    NK_BUFREAD_IO_ERROR = 2,
    /**
     * The `nk_buf_reader` has reached the end of iteration safely.
     */
    NK_BUFREAD_ITER_OVER = 3,
} nk_buf_reader_error_code;

void nk_buf_reader_init(nk_buf_reader *);

/// Assumes that everything from the front of `r.buf` until the first newline
/// (or the end of the buffer, if there is no newline) is already consumed and
/// hence can be overwritten.
///
/// Reads data from `r`, and puts it at the front of the buffer `r.buf`. There
/// is guaranteed to be a NUL character within the memory allocated to `r.buf`.
///
/// Returns 0 upon success, and -1 if any error is encountered (including
/// reaching end of file). `errno` would be set as per the result of the call to
/// `read`.
int nk_buf_reader_next(nk_buf_reader *r);

#ifdef __cplusplus
}
#endif

#endif
