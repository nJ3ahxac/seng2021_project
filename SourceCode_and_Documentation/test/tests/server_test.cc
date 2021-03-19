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
    const PageHandler handler{*servertest::test_moviedata};
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
