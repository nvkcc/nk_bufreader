#ifndef _NK_BUFREAD_H
#define _NK_BUFREAD_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /**
     * No error occurred; the call was successful.
     */
    NK_BUFREAD_OK = 0,
    /**
     * There was an error returned when calling `read()`. Check `errno` for the
     * error that `read()` left behind.
     */
    NK_BUFREAD_IO_ERROR = 1,
    /**
     * The space allocated to the buffer is not enough to contain one of the
     * lines that the buffer is used to read. You have to allocate more space.
     */
    NK_BUFREAD_INSUFFICIENT_SPACE = 2,
    /**
     * The `nk_bufreader` has reached the end of iteration safely.
     */
    NK_BUFREAD_ITER_OVER = 3,
    /**
     * Tried running a method on an invalid state of nk_bufreader.
     */
    NK_BUFREAD_INVALID = 4,
} nk_bufreader_error_code;

/// A buffered reader that reads line by line (delimited by '\n'). The buffer
/// must be two bytes longer than the longest line to be read, since we
/// guarantee that the buffer always contains the NUL byte. So one byte for the
/// delimiter, and one byte for the NUL byte.
///
/// Please call `nk_bufreader_init` on the object initialize a valid state
/// before doing any other operations.
///
/// Using a `len` of 0 results in undefined behavior. Using an invalid address
/// for `buf` also is undefined behavior.
///
/// This struct does not own any data so it need not be freed.
typedef struct nk_bufreader {
    /** [Internal Notes]
     * The buffer is split into 3 sections, by 3 pointers. A: `*buf`, B:
     * `*newl`, and C: `*end`. Now, imagine we're in Rust. The slice
     * buffer[A..B] is the part to be consumed externally. Hence, buffer[B]
     * should always be set to the NUL byte before being exposed. Then, [B..C]
     * contains buffered data that will be consumed next. And finally [C..] is
     * invalid data.
     */

    /// File descriptor to read from.
    const int fd;
    /// Byte buffer length. This is assumed to be greater than zero, otherwise
    /// we have UB.
    const unsigned int len;
    /// Byte buffer.
    char *const buf;
    char *left, *right;
    nk_bufreader_error_code last_error_code;
} nk_bufreader;

void nk_bufreader_init(nk_bufreader *);

/// Reads data from `r.fd`, and puts it at the front of the buffer `r.buf`.
/// There is guaranteed to be a NUL character within the memory allocated to
/// `r.buf`.
///
/// Returns 0 upon success, and -1 if any error is encountered (including
/// reaching end of file). `errno` would be set as per the result of the call to
/// `read`.
int nk_bufreader_next(nk_bufreader *r);

#ifdef __cplusplus
}
#endif

#endif
