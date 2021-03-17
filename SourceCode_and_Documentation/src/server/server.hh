#ifndef SERVER_SERVER_HH_
#define SERVER_SERVER_HH_

#include "search.hh"
#include "util/util.hh"

#include <pistache/endpoint.h>
#include <boost/circular_buffer.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <unordered_map>
#include <string>
#include <ranges>

namespace json = rapidjson;

class PageHandler : public Pistache::Http::Handler {
private:
    static constexpr int max_token_storage = 100u;
    // This structure maps a binding to a filename, ie, "/" -> "/main.html"
    // Basically associations of web directory to html files.
    const std::unordered_map<std::string, std::string> bindings;
    // Map of all files, where the key is their directory.
    const std::unordered_map<std::string, std::string> resources;
    // Circular buffer of search tokens
    boost::circular_buffer<search::token> tokens;
public:
    // Caches all web files provided and associates bindings for later lookup.
    // Throws std::runtime_error if a file does not exist, hopefully before the
    // server is started.
    PageHandler();

    // Macro for overriding the clone() member function for standard use.
    HTTP_PROTOTYPE(PageHandler)

    // Called on each request, handles all traffic.
    void onRequest(const Pistache::Http::Request& request,
                   Pistache::Http::ResponseWriter response) override;
    // Handles post requests, if an exception was thrown, the response is bad
    const auto& get_bindings() const noexcept { return bindings; }
    const auto& get_resources() const noexcept { return resources; }
private:
    void handle_post_request(const Pistache::Http::Request& request,
                             Pistache::Http::ResponseWriter& response);
    void handle_get_request(const Pistache::Http::Request& request,
                             Pistache::Http::ResponseWriter& response);

    void handle_token_init(const json::Document& d, Pistache::Http::ResponseWriter& response);
    void handle_token_info(const json::Document& d, Pistache::Http::ResponseWriter& response);
    void handle_token_advance(const json::Document& d, Pistache::Http::ResponseWriter& response);
    void handle_token_results(const json::Document& d, Pistache::Http::ResponseWriter& response);

    search::token& get_token(const std::int64_t id);
    search::token& get_token(const json::Document& d);
};

#endif
