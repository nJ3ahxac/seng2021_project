#ifndef CLIENT_CLIENT_HH_
#define CLIENT_CLIENT_HH_

#define RAPIDJSON_HAS_STDSTRING 1

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

// Library for making requests.
#include <curl/curl.h>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_set>

#include <mutex>
#include <thread>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim_all.hpp>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

namespace json = rapidjson;

struct MovieData {
    json::Document data;
    
    static std::string download_url(const std::string& url);
    static json::Document download_url_json(const std::string& url);
    static std::string gzip_decompress(const std::string& data);
    static void write_json(const std::string& dir, const json::Document& j);
    static json::Document open_json(const std::string& dir);
    static json::Document gzip_download_to_json(const std::string& url);

    class cache_error : public std::exception {
    public:
        virtual const char* what() const noexcept override { 
            return "Bad cache read";
        }
    };

    enum class construct { with_cache, without_cache };

    // The constructor may be supplied with with or without cache.
    // with_cache loads the data from the local dir "cache", and throws a
    // cache_error if it fails to parse. Without cache regenerates the cache
    // and overwrites the prexisting one.
    // with_cache is preferred over without_cache as it is far slower. It
    // should only be performed between long intervals when updating the movie
    // data is necessary.
    MovieData(const construct&);
};

#endif
