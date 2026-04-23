#include "nk_bufreader.h"
#include <string.h>
#include <unistd.h>

void nk_bufreader_init(nk_bufreader *r) {
    // Initialize all three pointers to be the same.
    r->left = r->right = r->end = r->buf;
    // Set the last byte to NUL to prevent any case of overflow by string
    // reading. We shall never touch this byte again.
    *(r->buf + r->len - 1) = '\0';
    r->err = NK_BUFREAD_OK;
}

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

/// Moves `r->left` to the front, and moves all other points accordingly.
void nk_bufreader_shift(nk_bufreader *r) {
    memmove(r->buf, r->left, r->end - r->left);
    r->end -= r->left - r->buf;
    r->left = r->buf;
}

/// Reads as many bytes as we can into the buffer, while never touching the last
/// byte to keep it set to NUL. Advances the `r->end` pointer if necessary.
int nk_bufreader_read(nk_bufreader *r, int bytes_to_read) {
    int n;
    if ((n = read(r->fd, r->end, bytes_to_read)) > 0) {
        r->end += n;
    }
    return n;
}

int nk_bufreader_next(nk_bufreader *r) {
#define REMAIN_B(P) (r->len - (P - r->buf) - 1)
    // If this reader already terminated before, then return that same error.
    if (r->err != NK_BUFREAD_OK) {
        return r->err;
    }
    if (r->right == NULL) {
        // Even after the previous iteration, we couldn't find any '\n' for
        // r->right to point to -> the '\n' to the left of r->left is the last
        // '\n'. End iteration.
        return (r->err = NK_BUFREAD_ITER_OVER);
    }
    r->left = r->right;
    if ((r->right = memchr(r->left, '\n', r->end - r->left))) {
        r->right++;
        return NK_BUFREAD_OK;
    }
    nk_bufreader_shift(r);
    int bytes_to_read = REMAIN_B(r->end);
    int n = nk_bufreader_read(r, bytes_to_read);
    if (n == -1) {
        *r->left = '\0';
        return r->err = NK_BUFREAD_IO_ERROR;
    } else if (n == 0) {
        r->err = NK_BUFREAD_ITER_OVER;
    }

    if ((r->right = memchr(r->left, '\n', r->end - r->left))) {
        r->right++;
        return NK_BUFREAD_OK;
    } else {
        if (n == bytes_to_read && *(r->end - 1) != '\0') {
            *r->left = '\0';
            return NK_BUFREAD_INSUFFICIENT_SPACE;
        }
        r->err = NK_BUFREAD_ITER_OVER;
        return NK_BUFREAD_OK;
    }
#undef REMAIN_B
}
