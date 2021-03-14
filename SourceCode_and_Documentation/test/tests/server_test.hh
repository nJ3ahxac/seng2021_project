#ifndef TEST_TESTS_SERVER_TEST_HH
#define TEST_TESTS_SERVER_TEST_HH

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include <curl/curl.h>

#include <pistache/endpoint.h>

#include "main.hh"
#include "server/server.hh"
#include "util/util.hh"

static int test_port = PORT_DEFAULT;

// Since googletest inits a new fixtures each test, we will have to work around
// failing to bind to the same port within short periods by incrementing
// test_port each time we construct ServerTest.
class ServerTest : public ::testing::Test {
private:
    std::thread server_thread;

protected:
    ServerTest() {
        // Blocking until the server starts.
        const auto start_server = []() {
            const Pistache::Address addr(Pistache::Ipv4::any(),
                                         Pistache::Port(++test_port));
            const auto opts =
                Pistache::Http::Endpoint::options().threads(THREADS_DEFAULT);
            Pistache::Http::Endpoint server(addr);

            server.init(opts);
            server.setHandler(Pistache::Http::make_handler<PageHandler>());
            server.serve();
        };
        this->server_thread = std::thread(start_server);
    }
    // To avoid a race condition, we must wait for the server to init on the
    // other thread before running any tests.
    void SetUp() override {
        const auto start = std::chrono::steady_clock::now();
        while (util::request("localhost", test_port).get_code() != 200) {
            const auto now = std::chrono::steady_clock::now();
            if (now > start + std::chrono::seconds(1)) {
                throw std::runtime_error("Fixture server did not respond");
            }
        }
    }

    ~ServerTest() override { server_thread.detach(); };
};

#endif
