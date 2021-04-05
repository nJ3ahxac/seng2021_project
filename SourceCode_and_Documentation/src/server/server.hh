#ifndef SERVER_SERVER_HH_
#define SERVER_SERVER_HH_

#include "httplib.h"

#include "server/search.hh"
#include "client/client.hh"
#include "util/util.hh"

#include <boost/circular_buffer.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <unordered_map>
#include <string>
#include <ranges>

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
    // Circular buffer of search tokens
    boost::circular_buffer<search::token> tokens;
    // Move data
    MovieData movie_data;
    
    const auto& get_bindings() const noexcept { return bindings; }
    const auto& get_resources() const noexcept { return resources; }
public:
    ServerData(const MovieData& m);

    void handle_post_request(const httplib::Request& request,
                    httplib::Response& response);
    void handle_get_request(const httplib::Request& request,
                             httplib::Response& response);
private:
    std::string movie_info_from_imdb(const std::string& imdb, const bool verbose);

    void handle_token_init(const json::Document& d, httplib::Response& response);
    void handle_token_info(const json::Document& d, httplib::Response& response);
    void handle_token_advance(const json::Document& d, httplib::Response& response);
    void handle_token_results(const json::Document& d, httplib::Response& response);

    search::token& get_token(const std::int64_t id);
    search::token& get_token(const json::Document& d);
};

#endif
