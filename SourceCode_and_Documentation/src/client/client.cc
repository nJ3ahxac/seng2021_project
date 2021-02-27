#include "client.hh"

std::string MovieData::download_url(const std::string& url) const {
    static CURL* curl = curl_easy_init();

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

json MovieData::download_url_json(const std::string& url) const {
    return json::parse(download_url(url));
}

MovieData::MovieData(const construct& c) {
    // Do not update cache if constructed with_cache, just read from files.
    if (c == construct::with_cache) {
        try {
            throw cache_error();
        } catch (...) {
            throw cache_error();
        }
        return;
    }

    // construct::without_cache
    // This url contains a gzipped tsv of all movies in imdb, most notably their
    // identifier. This identifier is used later when grabbing more info.
    const auto url_titles = "https://datasets.imdbws.com/title.basics.tsv.gz";
    const std::string title_basics_gz = download_url(url_titles);
    std::cout << title_basics_gz << '\n';
}
