#include <nk_test_printer.h>

#include "nk_bufread.h"

TEST(BufRead, BasicAssertions) {
    nk_buf_reader br = {.buf = STDIN_FILENO};
    write(STDIN_FILENO, "one\ntwo\nthree", 3);
    int err;
    err = nk_buf_reader_next(&br);
    ASSERT_EQ(err, 0);
}

TEST(BufRead, BasicAssertions2) {
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
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
