#ifndef _NK_BUFREAD_H
#define _NK_BUFREAD_H 1

#ifdef __cplusplus
extern "C" {
#endif

/// A buffered reader that reads up until the newline character. The buffer must
/// be long enough to contain the longest line to be read.
///
/// NOTE: after initializing with {.buf = ...}, please call `nk_buf_reader_init`
/// on the object initialize a valid state before doing any other operations.
/// This struct does not own any data so it need not be freed.
typedef struct nk_buf_reader {
    /// File descriptor to read from.
    int fd;
    /// Byte buffer.
    char *const buf;
    /// Points to the first newline in the buffer.
    char *newl;
    /// Points to the first invalid character of the buffer. That is, the
    /// character behind this pointer is not read from `fd`. However, this
    /// pointer still points to a valid place in `buf`'s allocated memory.
    /// Should always be set to NUL.
    char *end_of_buf;
    /// Byte buffer length.
    int buf_len;
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

#ifdef __cplusplus
}
#endif

#endif
