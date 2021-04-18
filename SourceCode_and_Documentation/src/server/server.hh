#ifndef SERVER_SERVER_HH_
#define SERVER_SERVER_HH_

#include "../../lib/cpp-httplib/httplib.h"

#include "client/client.hh"
#include "server/search.hh"
#include "util/util.hh"

#include <boost/circular_buffer.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <ranges>
#include <string>
#include <unordered_map>

namespace json = rapidjson;

class ServerData {
private:
    static constexpr int max_token_storage = 100u;
    // This structure maps a binding to a filename, ie, "/" -> "/main.html"
    // Basically associations of web directory to html files.
    const std::unordered_map<std::string, std::string> bindings;
    // Map of all files, where the key is their directory.
    const std::unordered_map<std::string, std::string> resources;
    // SearchData object for making operations to tokens.
    SearchData searchdata;
    // Circular buffer of search tokens.
    boost::circular_buffer<search::token> tokens;
    // Movie data, utilised during webserver queries.
    MovieData movie_data;
    // Manages everything http related.
    httplib::Server server;

    const auto& get_bindings() const noexcept { return bindings; }
    const auto& get_resources() const noexcept { return resources; }

public:
    // Caches all provided web files and associates bindings for later lookup.
    // Throws a std::runtime_error if a requested file does not exist.
    // Begins hosting the server on localhost@port once initialisation finishes.
    // If exit_on_delete is true, the server will exit if it receives any DELETE
    // http request (useful for tests).
    ServerData(const MovieData& m, const std::uint16_t port,
               bool exit_on_delete = false);

    void handle_post_request(const httplib::Request& request,
                             httplib::Response& response);
    void handle_get_request(const httplib::Request& request,
                            httplib::Response& response);

private:
    std::string movie_info_from_imdb(const std::string& imdb,
                                     const bool verbose);

    void handle_token_init(const json::Document& d,
                           httplib::Response& response);
    void handle_token_info(const json::Document& d,
                           httplib::Response& response);
    void handle_token_advance(const json::Document& d,
                              httplib::Response& response);
    void handle_token_results(const json::Document& d,
                              httplib::Response& response);

    search::token& get_token(const std::int64_t id);
    search::token& get_token(const json::Document& d);
};

#endif
