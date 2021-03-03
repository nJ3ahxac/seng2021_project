#include "server_test.hh"

TEST_F(ServerTest, visible_web_directory) {
    // The ctor of PageHandler throws a std::runtime_error if it cannot
    // access a requested resource, which is caught by gtest, failing the test.
    PageHandler{};
}

// Returns http code.
static long ping_url(const std::string& url, const int port) {
    CURL* const curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to get curl handle for request");
    }

    // If we don't set this, it prints to cout.
    const auto write_callback = [](char* contents, std::size_t size,
                                   std::size_t nmemb,
                                   void* userp) { return size * nmemb; };
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_PORT, port);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, *write_callback);
    CURLcode result = curl_easy_perform(curl);

    if (result != CURLE_OK) {
        throw std::runtime_error(std::string("Request failed: ") +
                                 curl_easy_strerror(result));
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    return http_code;
}

TEST_F(ServerTest, bindings_code_200) {
    PageHandler handler;
    for (const auto& b : handler.get_bindings()) {
        EXPECT_EQ(ping_url("localhost" + b.first, test_port), 200L);
    }
}

TEST_F(ServerTest, resources_code_200) {
    PageHandler handler;
    for (const auto& r : handler.get_resources()) {
        EXPECT_EQ(ping_url("localhost" + r.first, test_port), 200L);
    }
}

TEST_F(ServerTest, not_existent_code_404) {
    PageHandler handler;
    EXPECT_EQ(ping_url("localhost/_____DOES_NOT_EXIST.wav", test_port), 404L);
}

static std::string download_url_string(const std::string& url, const int port) {
    CURL* const curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Unable to get curl handle for request");
    }

    std::string request_contents;
    const auto write_callback = [](char* contents, std::size_t size,
                                   std::size_t nmemb, std::string* userp) {
        userp->append(contents, size * nmemb);
        return size * nmemb;
    };
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_PORT, port);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, *write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &request_contents);
    CURLcode result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw std::runtime_error(std::string("Request failed: ") +
                                 curl_easy_strerror(result));
    }
    return request_contents;
}

TEST_F(ServerTest, bindings_same_file) {
    PageHandler handler;
    for (const auto& b : handler.get_bindings()) {
        std::ifstream input("web" + b.second);
        if (!input.is_open()) {
            throw std::runtime_error("Failed to open file: web" + b.second);
        }
        std::string file;
        std::copy(std::istreambuf_iterator<char>(input),
                  std::istreambuf_iterator<char>(),
                  std::inserter(file, file.begin()));
        EXPECT_EQ(file, download_url_string("localhost" + b.first, test_port));
    }
}

TEST_F(ServerTest, resources_same_file) {
    PageHandler handler;
    for (const auto& r : handler.get_resources()) {
        std::ifstream input("web" + r.first);
        if (!input.is_open()) {
            throw std::runtime_error("Failed to open file: web" + r.second);
        }
        std::string file;
        std::copy(std::istreambuf_iterator<char>(input),
                  std::istreambuf_iterator<char>(),
                  std::inserter(file, file.begin()));
        EXPECT_EQ(file, download_url_string("localhost" + r.first, test_port));
    }
}
