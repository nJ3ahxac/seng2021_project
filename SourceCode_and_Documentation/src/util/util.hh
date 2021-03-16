#ifndef UTIL_UTIL_HH_
#define UTIL_UTIL_HH_

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <exception>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim_all.hpp>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#include <curl/curl.h>

namespace json = rapidjson;

namespace util {
// Curl wrapper class to avoid code duplication.
class request {
private:
    CURLcode result = CURLE_OK;
    std::string contents;
    long code = 0;

public:
    request(const std::string& url, const long& port = 0);
    const CURLcode& get_result() const noexcept { return result; }
    const long& get_code() const noexcept { return code; }
    const std::string& get_contents() const noexcept { return contents; }
    bool is_good() const noexcept { return !this->result; }
};

std::string download_url(const std::string& url);
json::Document download_url_json(const std::string& url);

std::string decompress_gzip(const std::string& data);

std::string read_file(const std::string& dir);
std::ofstream open_file(const std::string& dir);

void write_file_json(const std::string& dir, const json::Document& j);
json::Document read_file_json(const std::string& dir);
} // namespace util

#endif
