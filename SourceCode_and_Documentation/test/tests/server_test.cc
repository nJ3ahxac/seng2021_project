#include "server_test.hh"

#include <exception>

TEST(server_test, visible_web_directory) {
    // The ctor of PageHandler throws a std::runtime_error if it cannot
    // access a requested resource, which is caught by gtest, failing the test.
    PageHandler{};
}
