#include "util_test.hh"

static void declare_internet() {
    std::cerr << "example.com is unreachable, test will auto pass\n";
}

TEST_F(UtilTest, request_good) {
    if (!utiltest::has_internet) {
        declare_internet();
        return;
    }
    EXPECT_EQ(util::request{utiltest::test_url}.is_good(), true);
}

TEST_F(UtilTest, request_code_200) {
    if (!utiltest::has_internet) {
        declare_internet();
        return;
    }
    EXPECT_EQ(util::request(utiltest::test_url).get_code(), 200L);
}

TEST_F(UtilTest, request_code_404) {
    if (!utiltest::has_internet) {
        declare_internet();
        return;
    }
    EXPECT_EQ(util::request(utiltest::test_url + "/___404").get_code(), 404L);
}

TEST_F(UtilTest, request_has_contents) {
    if (!utiltest::has_internet) {
        declare_internet();
        return;
    }
    EXPECT_NE(util::request(utiltest::test_url).get_contents(), std::string{});
}
