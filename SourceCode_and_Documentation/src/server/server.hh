#ifndef SERVER_SERVER_HH_
#define SERVER_SERVER_HH_

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>

#include <pistache/endpoint.h>

class PageHandler : public Pistache::Http::Handler {
private:
    // This structure maps a binding to a filename, ie, "/" -> "/main.html"
    // Basically associations of web directory to html files.
    std::map<std::string, std::string> bindings;
    // Map of all files, where the key is their directory.
    std::map<std::string, std::string> resources;

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

    const std::map<std::string, std::string>& get_bindings() const noexcept {
        return bindings;
    }
    const std::map<std::string, std::string>& get_resources() const noexcept {
        return resources;
    }
};

#endif
