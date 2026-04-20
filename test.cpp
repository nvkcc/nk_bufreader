#include <nk_test_printer.h>
#include <unistd.h>

#include "nk_bufread.h"

#define PIPE_SETUP(N, ...)                                                     \
    int __fd[2];                                                               \
    ASSERT_NE(pipe(__fd), -1) << "Failed to start pipe";                       \
    dprintf(__fd[1], __VA_ARGS__);                                             \
    close(__fd[1]);                                                            \
    char __buf[N];                                                             \
    nk_buf_reader br = {.fd = __fd[0], .buf = __buf, .len = N};                \
    nk_buf_reader_init(&br);

// Pushes forward the reader, and asserts on the error code returned.
// Also checks that the end is
//  1. Set to the NUL character.
//  2. Within bounds.
#define ASSERT_NEXT(br, err_code)                                              \
    ASSERT_EQ(nk_buf_reader_next(&br), err_code);                              \
    ASSERT_EQ(*br.end, 0);                                              \
    ASSERT_LE(br.buf, br.end);                                          \
    ASSERT_LT(br.end, br.buf + br.len);

void print_br(nk_buf_reader *r) {
    std::cout << '[';
    for (int i = 0; i < r->len; ++i) {
        std::cout << (int)(r->buf[i]);
        if (i + 1 < r->len) {
            std::cout << ", ";
        }
    }
    std::cout << "] (" << r->buf << ')' << std::endl;
}

TEST(BufRead, BasicAssertions) {
    PIPE_SETUP(4, "a\nbb\nccc");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "a");
    print_br(&br);
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "bb");
    print_br(&br);
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "ccc");
    ASSERT_NEXT(br, NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, Counting) {
    PIPE_SETUP(10, "one\ntwo\nthree");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "one");
    print_br(&br);
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "two");
    print_br(&br);
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "three");
    ASSERT_NEXT(br, NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, BufferTooSmall) {
    PIPE_SETUP(7, "gold\nsilver\nbronze");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "gold");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "silver");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "bronze");
}

TEST(BufRead, BufferExactlyEnough) {
    GTEST_SKIP();
    PIPE_SETUP(6, "adieu\nocean\nsoare");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ(br.buf, "adieu");
    ASSERT_EQ(br.buf[5], '\0');
    // ASSERT_EQ((int)(br.newl - br.buf), 0);
    // ASSERT_NEXT(br, NK_BUFREAD_OK);
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
