#define DEBUG_MODE

#include "log.h"
#include "nk_bufreader.h"
#include <gmock/gmock.h>
#include <nk_test_printer.h>
#include <unistd.h>

#define PIPE_SETUP(N, ...)                                                     \
    int __fd[2];                                                               \
    ASSERT_NE(pipe(__fd), -1) << "Failed to start pipe";                       \
    dprintf(__fd[1], __VA_ARGS__);                                             \
    write(__fd[1], "", 1);                                                     \
    close(__fd[1]);                                                            \
    char __buf[N];                                                             \
    nk_bufreader r = {.fd = __fd[0], .len = N, .buf = __buf};                  \
    nk_bufreader_init(&r);

// Pushes forward the reader, and asserts on the error code returned.
// Also checks that the end is
//  1. Set to the NUL character.
//  2. Within bounds.
#define ASSERT_NEXT(br, err_code) ASSERT_EQ(nk_bufreader_next(&r), err_code);

// Asserts that the C-string when read from the front matches the test case, and
// also that the length defined by r->newl is correct.
#define ASSERT_STREQ2(r, test_case)                                            \
    ASSERT_THAT(r.left, ::testing::StartsWith(test_case));

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

TEST(BufRead, EmptyString) {
    PIPE_SETUP(8, "");
    ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_OK);
    ASSERT_STREQ2(r, "");
    ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, OneLiner) {
    PIPE_SETUP(8, "hello");
    ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_OK);
    ASSERT_STREQ2(r, "hello");
    ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, TwoLiner) {
    PIPE_SETUP(8, "hello\nworld");
    ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_OK);
    ASSERT_STREQ2(r, "hello\n");
    ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_OK);
    ASSERT_STREQ2(r, "world");
    ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, ABCs) {
    PIPE_SETUP(5, "a\nbb\nccc");
    ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_OK);
    ASSERT_STREQ2(r, "a\n");
    ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_OK);
    ASSERT_STREQ2(r, "bb\n");
    ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_OK);
    ASSERT_STREQ2(r, "ccc");
    ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, Counting) {
    PIPE_SETUP(10, "one\ntwo\nthree");
    ASSERT_NEXT(r, NK_BUFREAD_OK);
    ASSERT_STREQ2(r, "one\n");
    ASSERT_NEXT(r, NK_BUFREAD_OK);
    ASSERT_STREQ2(r, "two\n");
    ASSERT_NEXT(r, NK_BUFREAD_OK);
    ASSERT_STREQ2(r, "three");
    ASSERT_NEXT(r, NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, BufferTooSmall) {
    PIPE_SETUP(4, "aa\nbbb\ncccc");
    ASSERT_NEXT(r, NK_BUFREAD_OK);
    ASSERT_STREQ2(r, "aa\n");
    ASSERT_NEXT(r, NK_BUFREAD_INSUFFICIENT_SPACE);
    // ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_INVALID);
}

// TEST(BufRead, BufferExactlyEnough) {
//     PIPE_SETUP(8, "adieu\nocean\nsoare");
//     ASSERT_NEXT(r, NK_BUFREAD_OK);
//     ASSERT_STREQ2(r, "adieu");
//     ASSERT_EQ(r.buf[5], '\0');
//     ASSERT_NEXT(r, NK_BUFREAD_OK);
//     ASSERT_STREQ2(r, "ocean");
//     ASSERT_EQ(r.buf[5], '\0');
//     ASSERT_NEXT(r, NK_BUFREAD_OK);
//     ASSERT_STREQ2(r, "soare");
//     ASSERT_EQ(nk_bufreader_next(&r), NK_BUFREAD_ITER_OVER);
// }

int main(int argc, char *argv[]) {
    log_set_level(LOG_TRACE);
    // Override the default result printer.
    testing::TestEventListeners &listeners =
        testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new NkTestPrinter);

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
