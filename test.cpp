#define DEBUG_MODE

#include "log.h"
#include "nk_bufreader.h"
#include <gmock/gmock.h>
#include <nk_test_printer.h>
#include <unistd.h>

#define PIPE_SETUP(N, ...)                                                     \
    int __fd[2], len;                                                          \
    nk_bufreader_error_code err;                                               \
    ASSERT_NE(pipe(__fd), -1) << "Failed to start pipe";                       \
    dprintf(__fd[1], __VA_ARGS__);                                             \
    write(__fd[1], "", 1);                                                     \
    close(__fd[1]);                                                            \
    char __buf[N], *ptr;                                                       \
    nk_bufreader r = {.fd = __fd[0], .len = N, .buf = __buf};                  \
    nk_bufreader_init(&r);

// Asserts that the C-string when read from the front matches the test case, and
// also that the length defined by r->newl is correct.
#define ASSERT_STREQ2(result, test_case)                                       \
    ASSERT_EQ(len, sizeof(test_case) - 1);                                     \
    ASSERT_THAT(result, ::testing::StartsWith(test_case));

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

#define NK_BUFREAD_TEST(r, len, ERR)                                           \
    ptr = nk_bufreader_next(&r, &len, &err);                                   \
    ASSERT_EQ(err, ERR);

TEST(BufRead, EmptyString) {
    PIPE_SETUP(8, "");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, OneLiner) {
    PIPE_SETUP(8, "hello");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "hello");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, TwoLiner) {
    PIPE_SETUP(8, "hello\nworld");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "hello\n");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "world");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, ABCs) {
    PIPE_SETUP(5, "a\nbb\nccc");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "a\n");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "bb\n");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "ccc");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, Counting) {
    PIPE_SETUP(10, "one\ntwo\nthree");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "one\n");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "two\n");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "three");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_ITER_OVER);
}

TEST(BufRead, BufferTooSmall) {
    PIPE_SETUP(5, "aa\nbbb\ncccc");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "aa\n");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "bbb\n");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_INSUFFICIENT_SPACE);
    ASSERT_EQ(ptr, nullptr);
}

TEST(BufRead, BufferExactlyEnough) {
    PIPE_SETUP(7, "adieu\nocean\nsoare");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "adieu\n");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "ocean\n");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_OK);
    ASSERT_STREQ2(ptr, "soare");
    NK_BUFREAD_TEST(r, len, NK_BUFREAD_ITER_OVER);
}

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
