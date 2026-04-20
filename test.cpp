#include <nk_test_printer.h>
#include <unistd.h>

#include "nk_bufread.h"

#define PIPE_SETUP(N, ...)                                                     \
    int __fd[2];                                                               \
    ASSERT_NE(pipe(__fd), -1) << "Failed to start pipe";                       \
    dprintf(__fd[1], __VA_ARGS__);                                             \
    close(__fd[1]);                                                            \
    char __buf[N];                                                             \
    nk_buf_reader br = {.buf = __buf};                                         \
    nk_buf_reader_init(&br, __fd[0], N);

// Pushes forward the reader, and asserts on the error code returned.
// Also checks that the end_of_buf is
//  1. Set to the NUL character.
//  2. Within bounds.
#define ASSERT_NEXT(br, err_code)                                              \
    ASSERT_EQ(nk_buf_reader_next(&br), err_code);                              \
    ASSERT_EQ(*br.end_of_buf, 0);                                              \
    ASSERT_LE(br.buf, br.end_of_buf);                                          \
    ASSERT_LT(br.end_of_buf, br.buf + br.buf_len);

TEST(BufRead, BasicAssertions) {
    PIPE_SETUP(10, "one\ntwo\nthree");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "one");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "two");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "three");
    ASSERT_NEXT(br, NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, BufferTooSmall) {
    PIPE_SETUP(5, "gold\nsilver\nbronze");
    ASSERT_EQ(nk_buf_reader_next(&br), 0);
    ASSERT_STREQ(br.buf, "gold");
    ASSERT_EQ(nk_buf_reader_next(&br), NK_BUFREAD_INSUFFICIENT_SPACE);
}

TEST(BufRead, BufferExactlyEnough) {
    PIPE_SETUP(6, "adieu\nocean\nsoare");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "adieu");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    // ASSERT_STREQ(br.buf, "ocean");
    // ASSERT_NEXT(br, NK_BUFREAD_OK);
    // ASSERT_STREQ(br.buf, "soare");
}

int main(int argc, char *argv[]) {
    // Override the default result printer.
    testing::TestEventListeners &listeners =
        testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new NkTestPrinter);

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
