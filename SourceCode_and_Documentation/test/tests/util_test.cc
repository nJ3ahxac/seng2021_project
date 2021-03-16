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

TEST_F(UtilTest, download_url_has_contents) {
    if (!utiltest::has_internet) {
        declare_internet();
        return;
    }
    EXPECT_NE(util::download_url(utiltest::test_url), std::string{});
}

TEST_F(UtilTest, decompress_gzip_valid) {
    // hexdump of a gzip compressed file with the letter "a".
    static const std::uint16_t compressed[] = {0x8b1f, 0x0808, 0x5b35, 0x6050,
                                               0x0300, 0x0061, 0x044b, 0x4300,
                                               0xb7be, 0x01e8, 0x0000, 0x0000};
    const char* begin = reinterpret_cast<const char*>(compressed);
    // File ends at 0x17, subtract one byte from the array.
    const char* end = begin + std::size(compressed) * 2 - 1;
    EXPECT_EQ(util::decompress_gzip({begin, end}), "a");
}

TEST_F(UtilTest, decompress_gzip_invalid) {
    try {
        util::decompress_gzip("raw, not compressed");
    } catch (...) {
        return;
    }
    throw std::runtime_error("decompress_gzip succeeded on invalid input");
}

TEST_F(UtilTest, read_file_empty) {
    EXPECT_EQ(util::read_file(utiltest::test_filename), std::string{});
}
