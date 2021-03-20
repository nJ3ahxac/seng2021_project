#ifndef TEST_TESTS_SERVER_TEST_HH
#define TEST_TESTS_SERVER_TEST_HH

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <thread>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include <curl/curl.h>

#include <pistache/endpoint.h>

#include "client/client.hh"
#include "main.hh"
#include "server/server.hh"
#include "util/util.hh"

namespace servertest {
static int test_port = PORT_DEFAULT;
static std::string test_dir = "test/tmp/";
static std::optional<MovieData> test_moviedata;

static std::string init_postdata = "{\"type\":\"init\""
                                   ",\"is_foreign\":\"true\""
                                   ",\"is_greyscale\":\"true\""
                                   ",\"is_silent\":\"true\""
                                   ",\"is_pre1980\":\"true\""
                                   ",\"is_adult\":\"true\"}";
} // namespace servertest

// Since googletest inits a new fixtures each test, we will have to work around
// failing to bind to the same port within short periods by incrementing
// test_port each time we construct ServerTest.
class ServerTest : public ::testing::Test {
private:
    std::thread server_thread;

    // Creates a small cache so that we can test our operations on a less
    // encompassing and expensive dataset. Cleaned up in destructor.
    void create_test_moviedata_cache() {
        std::filesystem::create_directory(servertest::test_dir);
        auto out = util::open_file(servertest::test_dir + "title_basics.json");
        // Guaranteed valid json example dataset.
        const std::string contents_json =
            "{\"tt0815138\":{\"title\":\"Take\",\"genres\":[\"Crime\","
            "\"Drama\",\"Thriller\"],\"keywords\":[\"gambling\",\"meet\","
            "\"mother\",\"tragedy\",\"pass\"],\"rating\":5.900000095367432,"
            "\"votes\":1503,\"year\":2007,\"language\":\"English\"},"
            "\"tt0815140\":{\"title\":\"Dale\",\"genres\":[\"Documentary\","
            "\"Sport\"],\"keywords\":[\"documentary\",\"race\",\"death\","
            "\"racing\",\"footage\",\"home\",\"legend\",\"family\"],\"rating\":"
            "8.699999809265137,\"votes\":271,\"year\":2007,\"language\":"
            "\"English\"}}";
        out << contents_json;
    }

protected:
    ServerTest() {
        const auto start_server = [&]() {
            const Pistache::Address addr(Pistache::Ipv4::any(),
                                         Pistache::Port(servertest::test_port));
            const auto opts =
                Pistache::Http::Endpoint::options().threads(THREADS_DEFAULT);
            Pistache::Http::Endpoint server(addr);

            MovieData moviedata{MovieData::construct::with_cache,
                                servertest::test_dir};
            servertest::test_moviedata = moviedata;

            server.init(opts);
            server.setHandler(
                Pistache::Http::make_handler<PageHandler>(moviedata));
            server.serve();
        };
        ++servertest::test_port;
        create_test_moviedata_cache();
        this->server_thread = std::thread(start_server);

        // To avoid a race condition, we must wait for the server to init on the
        // other thread before running any tests.
        const auto start = std::chrono::steady_clock::now();
        while (util::request("localhost", servertest::test_port).get_code() !=
               200) {
            const auto now = std::chrono::steady_clock::now();
            if (now > start + std::chrono::seconds(1)) {
                throw std::runtime_error("Fixture server did not respond");
            }
        }
    }
    virtual ~ServerTest() {
        server_thread.detach();
        std::filesystem::remove_all(servertest::test_dir);
    };
};

#endif
