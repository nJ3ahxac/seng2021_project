#ifndef _SERVER_TEST_HH
#define _SERVER_TEST_HH

#include "gtest/gtest.h"

#include "server.hh"
#include "main.hh"

class ServerTest : public ::testing::Test {
protected:
    // Set up work for all tests (once).
    ServerTest() {
        const Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(PORT_DEFAULT));
        const auto opts = Pistache::Http::Endpoint::options().threads(THREADS_DEFAULT);
        Pistache::Http::Endpoint server(addr);
        server.init(opts);
        server.setHandler(Pistache::Http::make_handler<PageHandler>());
        server.serve();
    }
    // Cleanup for all tests (once).
    ~ServerTest() override {};
    // Set up for each test.
    void SetUp() override {};
    // Cleanup for each test.
    void TearDown() override{};
};

#endif