#ifndef TEST_TESTS_UTIL_TEST_HH
#define TEST_TESTS_UTIL_TEST_HH

#include <filesystem>
#include <fstream>

#include "gtest/gtest.h"

#include "util/util.hh"

namespace utiltest {
static const std::string test_filename = "utiltest.tmp";
static const std::string test_url = "example.com";
static bool has_internet = true;
} // namespace utiltest

// This class creates a tmp file for each test and also inits a has_internet
// bool which allows us to early out in tests which require internet.
class UtilTest : public ::testing::Test {
private:
    std::fstream tmp_file;

    // Creates and/or clears a file.
    void wipe_tmpfile(const std::string& dir) {
        using std::ofstream;
        constexpr auto flags = ofstream::out | ofstream::in | ofstream::trunc;
        this->tmp_file = std::fstream(utiltest::test_filename, flags);
    }

protected:
    UtilTest() {
        wipe_tmpfile(utiltest::test_filename);
        utiltest::has_internet = util::request{utiltest::test_url}.is_good();
    }

    void SetUp() override {
        if (!utiltest::has_internet) {
            std::cerr << "example.com is unreachable, test will auto pass\n";
        }
    }

    void TearDown() override { wipe_tmpfile(utiltest::test_filename); }

    ~UtilTest() override { std::filesystem::remove(utiltest::test_filename); };
};

#endif
