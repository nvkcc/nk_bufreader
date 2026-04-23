#include <nk_test_printer.h>
#include <unistd.h>

#include <gmock/gmock.h>

#include "nk_bufreader.h"

#define PIPE_SETUP(N, ...)                                                     \
    int __fd[2];                                                               \
    ASSERT_NE(pipe(__fd), -1) << "Failed to start pipe";                       \
    dprintf(__fd[1], __VA_ARGS__);                                             \
    write(__fd[1], "", 1);                                                     \
    close(__fd[1]);                                                            \
    char __buf[N];                                                             \
    nk_bufreader br = {.fd = __fd[0], .len = N, .buf = __buf};                 \
    nk_bufreader_init(&br);

// Pushes forward the reader, and asserts on the error code returned.
// Also checks that the end is
//  1. Set to the NUL character.
//  2. Within bounds.
#define ASSERT_NEXT(br, err_code) ASSERT_EQ(nk_bufreader_next(&br), err_code);

// Asserts that the C-string when read from the front matches the test case, and
// also that the length defined by r->newl is correct.
#define ASSERT_STREQ2(br, test_case) ASSERT_STREQ(br.buf, test_case);

void print_br(nk_bufreader *r) {
    std::cout << '[';
    for (int i = 0; i < r->len; ++i) {
        std::cout << (int)(r->buf[i]);
        if (i + 1 < r->len) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
}

TEST(BufRead, HelloWorld) {
    PIPE_SETUP(8, "hello\nworld");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ2(br, "hello");
    ASSERT_NEXT(br, NK_BUFREAD_OK);
    ASSERT_STREQ2(br, "world");
}

TEST(BufRead, ABCs) {
    PIPE_SETUP(5, "a\nbb\nccc");
    ASSERT_EQ(nk_bufreader_next(&br), NK_BUFREAD_OK);
    ASSERT_STREQ2(br, "a");
    ASSERT_EQ(nk_bufreader_next(&br), NK_BUFREAD_OK);
    ASSERT_STREQ2(br, "bb");
    ASSERT_EQ(nk_bufreader_next(&br), NK_BUFREAD_OK);
    ASSERT_STREQ2(br, "ccc");
    ASSERT_EQ(nk_bufreader_next(&br), NK_BUFREAD_ITER_OVER);
}

// TEST(BufRead, Counting) {
//     PIPE_SETUP(10, "one\ntwo\nthree");
//     ASSERT_NEXT(br, NK_BUFREAD_OK);
//     ASSERT_STREQ2(br, "one");
//     ASSERT_NEXT(br, NK_BUFREAD_OK);
//     ASSERT_STREQ2(br, "two");
//     ASSERT_NEXT(br, NK_BUFREAD_OK);
//     ASSERT_STREQ2(br, "three");
//     ASSERT_NEXT(br, NK_BUFREAD_ITER_OVER);
// }
//
// TEST(BufRead, BufferTooSmall) {
//     PIPE_SETUP(5, "aa\nbbb\ncccc");
//     ASSERT_NEXT(br, NK_BUFREAD_OK);
//     ASSERT_STREQ2(br, "aa");
//     ASSERT_NEXT(br, NK_BUFREAD_INSUFFICIENT_SPACE);
//     ASSERT_STREQ2(br, "");
//     ASSERT_EQ(nk_bufreader_next(&br), NK_BUFREAD_INVALID);
// }
//
// TEST(BufRead, BufferExactlyEnough) {
//     PIPE_SETUP(8, "adieu\nocean\nsoare");
//     ASSERT_NEXT(br, NK_BUFREAD_OK);
//     ASSERT_STREQ2(br, "adieu");
//     ASSERT_EQ(br.buf[5], '\0');
//     ASSERT_NEXT(br, NK_BUFREAD_OK);
//     ASSERT_STREQ2(br, "ocean");
//     ASSERT_EQ(br.buf[5], '\0');
//     ASSERT_NEXT(br, NK_BUFREAD_OK);
//     ASSERT_STREQ2(br, "soare");
//     ASSERT_EQ(nk_bufreader_next(&br), NK_BUFREAD_ITER_OVER);
// }
//
// TEST(BufRead, EmptyString) {
//     PIPE_SETUP(8, "");
//     ASSERT_NEXT(br, NK_BUFREAD_OK);
//     ASSERT_STREQ2(br, "");
//     ASSERT_NEXT(br, NK_BUFREAD_ITER_OVER);
// }

int main(int argc, char *argv[]) {
    // Override the default result printer.
    testing::TestEventListeners &listeners =
        testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new NkTestPrinter);

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
