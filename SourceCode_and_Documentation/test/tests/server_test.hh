#ifndef _SERVER_TEST_HH
#define _SERVER_TEST_HH

#include <algorithm>
#include <atomic>
#include <exception>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include <curl/curl.h>

#include <pistache/endpoint.h>

#include "main.hh"
#include "server.hh"

static int test_port = PORT_DEFAULT + 1; // avoid same port bind at runtime

// Since googletest inits a new fixtures each test, we will have to work around
// failing to bind to the same port within short periods by incrementing
// test_port each time we construct ServerTest.
class ServerTest : public ::testing::Test {
private:
    std::thread server_thread;

protected:
    ServerTest() {
        // Blocking until the server starts.
        std::mutex init_lock;
        const auto start_server = [&, this]() {
            const Pistache::Address addr(Pistache::Ipv4::any(),
                                         Pistache::Port(++test_port));
            const auto opts =
                Pistache::Http::Endpoint::options().threads(THREADS_DEFAULT);
            Pistache::Http::Endpoint server(addr);

            server.init(opts);
            server.setHandler(Pistache::Http::make_handler<PageHandler>());
            init_lock.unlock();
            server.serve();
        };

        init_lock.lock();
        this->server_thread = std::thread(start_server);
        init_lock.lock();
    }
    ~ServerTest() override { server_thread.detach(); };
};

#endif
