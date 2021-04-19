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

#include "client/client.hh"
#include "main.hh"
#include "server/server.hh"
#include "util/util.hh"

namespace servertest {
static constexpr int test_port = PORT_DEFAULT + 1;
static const std::string test_dir = "test/tmp/";
static std::optional<MovieData> test_moviedata;
static std::optional<ServerData> test_serverdata;

static const std::string init_postdata = "{\"type\":\"init\""
                                         ",\"is_foreign\":\"true\""
                                         ",\"is_greyscale\":\"true\""
                                         ",\"is_silent\":\"true\""
                                         ",\"is_pre1980\":\"true\""
                                         ",\"is_adult\":\"true\"}";
} // namespace servertest

// cpphttplib seems to have thread joining issues, making starting/stopping
// the server in quick succession impossible.
// As a workaround, we will only start the server once.
namespace {
inline std::thread server_thread;
inline std::once_flag server_flag;
} // namespace

class ServerTest : public ::testing::Test {
private:
    // Creates a small cache so that we can test our operations on a less
    // encompassing and expensive dataset. Cleaned up in destructor.
    void create_test_moviedata_cache() {
        std::filesystem::create_directory(servertest::test_dir);
        auto out = util::open_file(servertest::test_dir + "title_basics.json");
        // Guaranteed valid json example dataset.
        const std::string contents_json =
            "{\"tt0851515\":{\"title\":\"Silver "
            "Medalist\",\"genres\":[\"Action\",\"Adventure\",\"Comedy\"],"
            "\"keywords\":[\"cross\",\"bicycle\",\"racer\",\"body\"],"
            "\"rating\":7.300000190734863,\"votes\":2054,\"year\":2009,"
            "\"language\":\"Mandarin, Hokkien\",\"plot\":\"Assassins, "
            "scammers, gangsters, cops, a washed-up bicycle racer, and a body "
            "continually cross paths; usually with negative "
            "outcomes.\",\"director\":\"Hao Ning\",\"awards\":\"5 wins & 10 "
            "nominations.\",\"actors\":\"Bo Huang, Kung-Wei Lu, Morris Hsiang "
            "Jung, Jack Kao\",\"runtime\":\"99 "
            "min\",\"box_office\":\"N/A\",\"poster\":\"https://"
            "m.media-amazon.com/images/M/"
            "MV5BYTdkZmY4ODgtOTAzZi00OTA5LWIzMmQtNWEyYTVkMGM2YjY0XkEyXkFqcGdeQX"
            "VyMjg0MTI5NzQ@._V1_SX300.jpg\"},\"tt0851530\":{\"title\":\"The "
            "Lodger\",\"genres\":[\"Crime\",\"Mystery\",\"Thriller\"],"
            "\"keywords\":[\"mouse\",\"cat\",\"second\",\"tale\",\"killer\","
            "\"game\",\"serial\",\"detective\",\"first\",\"plot\","
            "\"relationship\"],\"rating\":5.699999809265137,\"votes\":4680,"
            "\"year\":2009,\"language\":\"English\",\"plot\":\"The tale of a "
            "serial killer in West Hollywood has two converging plot lines. "
            "The first involves an uneasy relationship between a "
            "psychologically unstable landlady and her enigmatic lodger; the "
            "second is about a troubled detective engaged in a cat-and-mouse "
            "game with the elusive killer, who is imitating the crimes of Jack "
            "the Ripper.\",\"director\":\"David Ondaatje\",\"awards\":\"1 "
            "nomination.\",\"actors\":\"Alfred Molina, Hope Davis, Shane West, "
            "Donal Logue\",\"runtime\":\"95 "
            "min\",\"box_office\":\"N/A\",\"poster\":\"https://"
            "m.media-amazon.com/images/M/"
            "MV5BMjA3NDMwNzgxM15BMl5BanBnXkFtZTcwNTY1MzUyMg@@._V1_SX300.jpg\"}"
            "}";
        out << contents_json;
    }

protected:
    ServerTest() {
        const auto start_server = [&]() {
            MovieData moviedata{MovieData::construct::with_cache,
                                servertest::test_dir};
            servertest::test_moviedata = moviedata;
            servertest::test_serverdata.emplace(moviedata,
                                                servertest::test_port, false);
        };

        create_test_moviedata_cache();
        std::call_once(::server_flag,
                       [&]() { ::server_thread = std::thread(start_server); });

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
        std::filesystem::remove_all(servertest::test_dir);
    };
};

#endif
