#include "server_test.hh"

TEST_F(ServerTest, visible_web_directory) {
    // The ctor of PageHandler throws a std::runtime_error if it cannot
    // access a requested resource, which is caught by gtest, failing the test.
    PageHandler{*servertest::test_moviedata};
}

TEST_F(ServerTest, bindings_code_200) {
    const PageHandler handler{*servertest::test_moviedata};
    for (const auto& b : handler.get_bindings()) {
        const auto url = "localhost" + b.first;
        EXPECT_EQ(util::request(url, servertest::test_port).get_code(), 200L);
    }
}

TEST_F(ServerTest, resources_code_200) {
    const PageHandler handler{*servertest::test_moviedata};
    for (const auto& r : handler.get_resources()) {
        const auto url = "localhost" + r.first;
        EXPECT_EQ(util::request(url, servertest::test_port).get_code(), 200L);
    }
}

TEST_F(ServerTest, non_existent_code_404) {
    const auto url = "localhost/_____DOES_NOT_EXIST.wav";
    EXPECT_EQ(util::request(url, servertest::test_port).get_code(), 404L);
}

TEST_F(ServerTest, bindings_first_same_file) {
    const PageHandler handler{*servertest::test_moviedata};
    for (const auto& b : handler.get_bindings()) {
        const std::string file = util::read_file("web" + b.second);
        const auto url = "localhost" + b.first;
        EXPECT_EQ(file,
                  util::request(url, servertest::test_port).get_contents());
    }
}

TEST_F(ServerTest, bindings_second_same_file) {
    const PageHandler handler{*servertest::test_moviedata};
    for (const auto& b : handler.get_bindings()) {
        const std::string file = util::read_file("web" + b.second);
        const auto url = "localhost" + b.second;
        EXPECT_EQ(file,
                  util::request(url, servertest::test_port).get_contents());
    }
}

TEST_F(ServerTest, resources_first_same_file) {
    const PageHandler handler{*servertest::test_moviedata};
    for (const auto& r : handler.get_resources()) {
        const std::string file = util::read_file("web" + r.first);
        const auto url = "localhost" + r.first;
        EXPECT_EQ(file,
                  util::request(url, servertest::test_port).get_contents());
    }
}

static bool contents_contains_error(const std::string& contents) {
    const auto pos = contents.find("error");
    return pos != std::string::npos;
}

TEST_F(ServerTest, post_token_init) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port, servertest::init_postdata);
    EXPECT_EQ(contents_contains_error(init_request.get_contents()), false);
}

TEST_F(ServerTest, post_token_init_invalid) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port,
                      servertest::init_postdata + "Now invalid json");
    EXPECT_EQ(contents_contains_error(init_request.get_contents()), true);
}

static std::string get_token_from_contents(const std::string& contents) {
    const auto token_start = contents.find(":", contents.find("\"token\"")) + 1;
    const auto token_end = contents.find(",", token_start);
    return contents.substr(token_start, token_end - token_start);
}

static std::string get_advance_request_contents(const std::string& token,
                                                const bool& should_include) {
    const std::string arg = should_include ? "true" : "false";
    return "{\"type\":\"advance\","
           "\"token\":\"" +
           token + "\",\"remove\":\"" + arg + "\"}";
}

TEST_F(ServerTest, post_token_init_advance_true) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port, servertest::init_postdata);
    const auto token = get_token_from_contents(init_request.get_contents());
    ASSERT_EQ(contents_contains_error(init_request.get_contents()), false);
    const auto postdata = get_advance_request_contents(token, true);
    const auto advance_request =
        util::request(url, servertest::test_port, postdata);
    ASSERT_EQ(contents_contains_error(advance_request.get_contents()), false);
}

TEST_F(ServerTest, post_token_init_advance_false) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port, servertest::init_postdata);
    const auto token = get_token_from_contents(init_request.get_contents());
    ASSERT_EQ(contents_contains_error(init_request.get_contents()), false);
    const auto postdata = get_advance_request_contents(token, false);
    const auto advance_request =
        util::request(url, servertest::test_port, postdata);
    ASSERT_EQ(contents_contains_error(advance_request.get_contents()), false);
}

TEST_F(ServerTest, post_token_init_advance_invalid) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port, servertest::init_postdata);
    const auto token = get_token_from_contents(init_request.get_contents());
    ASSERT_EQ(contents_contains_error(init_request.get_contents()), false);
    const auto postdata =
        get_advance_request_contents(token, false) + "Now invalid json";
    const auto advance_request =
        util::request(url, servertest::test_port, postdata);
    ASSERT_EQ(contents_contains_error(advance_request.get_contents()), true);
}

TEST_F(ServerTest, post_token_init_advance_multiple) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port, servertest::init_postdata);
    const auto token = get_token_from_contents(init_request.get_contents());
    ASSERT_EQ(contents_contains_error(init_request.get_contents()), false);
    for (auto i = 0; i < 5; ++i) {
        const auto postdata = get_advance_request_contents(token, i % 2);
        const auto advance_request =
            util::request(url, servertest::test_port, postdata);
        EXPECT_EQ(contents_contains_error(advance_request.get_contents()),
                  false);
    }
}

static std::string get_info_request_contents(const std::string& token) {
    return "{\"type\":\"info\","
           "\"token\":\"" +
           token + "\"}";
}

static std::string get_results_request_contents(const std::string& token,
        const int begin,
        const int count) {
    return "{\"type\":\"results\","
           "\"token\":\"" +
           token 
           + "\",\"begin\":\""
           + std::to_string(begin)
           + "\",\"count\":\""
           + std::to_string(count)
           + "\"}";
}

TEST_F(ServerTest, post_token_init_info) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port, servertest::init_postdata);
    ASSERT_EQ(contents_contains_error(init_request.get_contents()), false);
    const auto token = get_token_from_contents(init_request.get_contents());
    const auto info_request = util::request(url, servertest::test_port,
                                            get_info_request_contents(token));
    EXPECT_EQ(contents_contains_error(info_request.get_contents()), false);
}

TEST_F(ServerTest, post_token_init_info_invalid) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port, servertest::init_postdata);
    ASSERT_EQ(contents_contains_error(init_request.get_contents()), false);
    const auto token = get_token_from_contents(init_request.get_contents());
    const auto info_request =
        util::request(url, servertest::test_port,
                      get_info_request_contents(token) + "Now invalid json");
    EXPECT_EQ(contents_contains_error(info_request.get_contents()), true);
}

TEST_F(ServerTest, post_token_init_advance_info_multiple) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port, servertest::init_postdata);
    ASSERT_EQ(contents_contains_error(init_request.get_contents()), false);
    const auto token = get_token_from_contents(init_request.get_contents());
    for (auto i = 0; i < 1; ++i) {
        const auto postdata = get_advance_request_contents(token, i % 2);
        const auto advance_request =
            util::request(url, servertest::test_port, postdata);
        EXPECT_EQ(contents_contains_error(advance_request.get_contents()),
                  false);
        const auto info_request = util::request(
            url, servertest::test_port, get_info_request_contents(token));
        EXPECT_EQ(contents_contains_error(info_request.get_contents()), false);
    }
}

TEST_F(ServerTest, post_token_init_results) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port, servertest::init_postdata);
    ASSERT_EQ(contents_contains_error(init_request.get_contents()), false);
    const auto token = get_token_from_contents(init_request.get_contents());
    const auto info_request = util::request(
        url, servertest::test_port, get_results_request_contents(token, 0, 1));
    EXPECT_EQ(contents_contains_error(info_request.get_contents()), false);
}

TEST_F(ServerTest, post_token_init_results_zero_count) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port, servertest::init_postdata);
    ASSERT_EQ(contents_contains_error(init_request.get_contents()), false);
    const auto token = get_token_from_contents(init_request.get_contents());
    const auto info_request = util::request(
        url, servertest::test_port, get_results_request_contents(token, 0, 0));
    EXPECT_EQ(contents_contains_error(info_request.get_contents()), true);
}

TEST_F(ServerTest, post_token_init_results_invalid_begin) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port, servertest::init_postdata);
    ASSERT_EQ(contents_contains_error(init_request.get_contents()), false);
    const auto token = get_token_from_contents(init_request.get_contents());
    const auto info_request = util::request(
        url, servertest::test_port, get_results_request_contents(token, 3, 1));
    EXPECT_EQ(contents_contains_error(info_request.get_contents()), true);
}

TEST_F(ServerTest, post_token_init_results_invalid_end) {
    const auto url = "localhost";
    const auto init_request =
        util::request(url, servertest::test_port, servertest::init_postdata);
    ASSERT_EQ(contents_contains_error(init_request.get_contents()), false);
    const auto token = get_token_from_contents(init_request.get_contents());
    const auto info_request = util::request(
        url, servertest::test_port, get_results_request_contents(token, 0, 3));
    EXPECT_EQ(contents_contains_error(info_request.get_contents()), true);
}
