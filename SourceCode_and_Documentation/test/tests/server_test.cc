#include "server_test.hh"

TEST_F(ServerTest, visible_web_directory) {
    // The ctor of PageHandler throws a std::runtime_error if it cannot
    // access a requested resource, which is caught by gtest, failing the test.
    PageHandler{};
}

TEST_F(ServerTest, bindings_code_200) {
    PageHandler handler;
    for (const auto& b : handler.get_bindings()) {
        const auto url = "localhost" + b.first;
        EXPECT_EQ(util::request(url, test_port).get_code(), 200L);
    }
}

TEST_F(ServerTest, resources_code_200) {
    PageHandler handler;
    for (const auto& r : handler.get_resources()) {
        const auto url = "localhost" + r.first;
        EXPECT_EQ(util::request(url, test_port).get_code(), 200L);
    }
}

TEST_F(ServerTest, not_existent_code_404) {
    PageHandler handler;
    const auto url = "localhost/_____DOES_NOT_EXIST.wav";
    EXPECT_EQ(util::request(url, test_port).get_code(), 404L);
}

TEST_F(ServerTest, bindings_same_file) {
    PageHandler handler;
    for (const auto& b : handler.get_bindings()) {
        std::ifstream input("web" + b.second);
        if (!input.is_open()) {
            throw std::runtime_error("Failed to open file: web" + b.second);
        }
        std::string file;
        std::copy(std::istreambuf_iterator<char>(input),
                  std::istreambuf_iterator<char>(),
                  std::inserter(file, file.begin()));
        const auto url = "localhost" + b.first;
        EXPECT_EQ(file, util::request(url, test_port).get_contents());
    }
}

TEST_F(ServerTest, resources_same_file) {
    PageHandler handler;
    for (const auto& r : handler.get_resources()) {
        std::ifstream input("web" + r.first);
        if (!input.is_open()) {
            throw std::runtime_error("Failed to open file: web" + r.second);
        }
        std::string file;
        std::copy(std::istreambuf_iterator<char>(input),
                  std::istreambuf_iterator<char>(),
                  std::inserter(file, file.begin()));
        const auto url = "localhost" + r.first;
        EXPECT_EQ(file, util::request(url, test_port).get_contents());
    }
}
