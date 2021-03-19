#ifndef CLIENT_CLIENT_HH_
#define CLIENT_CLIENT_HH_

#define RAPIDJSON_HAS_STDSTRING 1

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// Library for making requests.
#include <curl/curl.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

#include <mutex>
#include <thread>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim_all.hpp>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#include "util/util.hh"

namespace json = rapidjson;

struct MovieData {
    json::Document data;

    class cache_error : public std::runtime_error {
    public:
        cache_error(const std::string& m) : std::runtime_error(m) {}
        virtual const char* what() const noexcept override {
            return std::runtime_error::what();
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
    MovieData(const construct&, const std::string& cache_dir = "cache/");
    MovieData(const MovieData& md);
    MovieData operator=(const MovieData& md);
};

#endif
