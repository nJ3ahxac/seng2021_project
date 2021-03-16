#include "util_test.hh"

static void declare_internet() {
    std::cerr << "example.com is unreachable, test will auto pass\n";
}

TEST_F(UtilTest, request_url) {
    if (!utiltest::has_internet) {
        declare_internet();
        return;
    }
    EXPECT_EQ(util::request{utiltest::test_url}.is_good(), true);
}
