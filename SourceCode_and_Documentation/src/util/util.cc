#include "util.hh"

// Request methods.
util::request::request(const std::string& url, const long& port,
                       const std::string& postfields) {
    CURL* const curl = curl_easy_init();

    if (!curl) {
        throw std::runtime_error("Unable to get curl handle for request");
    }

    std::string request_contents;
    const auto write_callback = [](char* data, std::size_t size,
                                   std::size_t nmemb, std::string* userp) {
        userp->append(data, size * nmemb);
        return size * nmemb;
    };

    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 2);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 5);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, *write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &request_contents);
    if (port) {
        curl_easy_setopt(curl, CURLOPT_PORT, port);
    }
    if (postfields != std::string{}) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());
    }

    this->result = curl_easy_perform(curl);

    // Do not write to our members if the request failed for whatever reason.
    if (result != CURLE_OK) {
        return;
    }

    this->contents = request_contents;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &this->code);
    curl_easy_cleanup(curl);
}

// Utility functions.
std::string util::download_url(const std::string& url) {
    auto request = util::request(url);
    while (!request.is_good()) {
        request = util::request(url);
    }
    return request.get_contents();
}

json::Document util::download_url_json(const std::string& url) {
    json::Document d;
    d.Parse(download_url(url).c_str());
    return d;
}

std::string util::decompress_gzip(const std::string& data) {
    std::stringstream input;
    input << data;

    // Use boost to handle gzip decompression.
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(boost::iostreams::gzip_decompressor());
    in.push(input);

    std::stringstream ret;
    boost::iostreams::copy(in, ret);

    return ret.str();
}

std::string util::read_file(const std::string& dir) {
    std::ifstream input(dir);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open readable file: " + dir);
    }
    std::string file_contents;
    std::copy(std::istreambuf_iterator<char>(input),
              std::istreambuf_iterator<char>(),
              std::inserter(file_contents, file_contents.begin()));
    return file_contents;
}

[[maybe_unused]] std::string util::read_file_min(const std::string& dir) {
    auto contents = read_file(dir);
    // For each newline, erase itself and whitespace chars after it.
    auto i = contents.find_first_of('\n');
    while (i != std::string::npos) {
        auto start_it = contents.begin() + static_cast<long>(i);
        auto end_it = std::find_if_not(start_it, contents.end(),
                                       [](auto c) { return std::isspace(c); });
        contents.erase(start_it, end_it);
        i = contents.find_first_of('\n');
    }
    return contents;
}

std::ofstream util::open_file(const std::string& dir) {
    std::ofstream ret(dir);
    if (!ret.is_open()) {
        throw std::runtime_error("Failed to open writable file: " + dir);
    }
    return ret;
}

void util::write_file_json(const std::string& dir, const json::Document& j) {
    auto output = util::open_file(dir);

    json::StringBuffer sb;
    json::Writer<json::StringBuffer> writer(sb);
    j.Accept(writer);
    output << sb.GetString();
}

json::Document util::read_file_json(const std::string& dir) {
    const std::string file_contents = util::read_file(dir);
    json::Document d;
    d.Parse(file_contents.c_str());
    return d;
}
