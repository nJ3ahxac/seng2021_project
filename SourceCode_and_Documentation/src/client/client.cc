#include "client.hh"

std::string MovieData::download_url(const std::string& url) {
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

json MovieData::download_url_json(const std::string& url) {
    return json::parse(download_url(url));
}

std::string MovieData::gzip_decompress(const std::string& data) {
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

void MovieData::write_json(const std::string& dir, const json& j) {
    std::ofstream output;
    output.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    output.open(dir);
    output << j;
}

json MovieData::open_json(const std::string& dir) {
    std::ifstream input;
    input.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    input.open(dir);
    std::string file_contents;
    std::copy(std::istreambuf_iterator<char>(input),
              std::istreambuf_iterator<char>(),
              std::inserter(file_contents, file_contents.begin()));
    return json::parse(file_contents);
}

static std::size_t get_part_index_of(const std::vector<std::string>& keys,
        const std::string& name) {
    const auto key_has_name = [&name](const std::string& key) {
        return key == name;
    };
    const auto it = std::find_if(keys.begin(), keys.end(), key_has_name);
    if (it == keys.end()) {
        throw std::runtime_error("no key named " + name);
    }
    return static_cast<std::size_t>(std::distance(keys.begin(), it));
}

json MovieData::gzip_download_to_json(const std::string& url) {
    const std::string title_basics = gzip_decompress(download_url(url));
    std::stringstream ss{title_basics};

    std::string entries;
    std::getline(ss, entries, '\n');

    std::vector<std::string> keys;
    boost::split(keys, entries, boost::is_any_of("\t"));

    const std::size_t imdb_index = get_part_index_of(keys, "tconst");
    const std::size_t type_index = get_part_index_of(keys, "titleType");
    const std::size_t genre_index = get_part_index_of(keys, "genres");
    const std::size_t title_index = get_part_index_of(keys, "primaryTitle");

    json data;

    for (std::string line; std::getline(ss, line, '\n');) {
        std::vector<std::string> parts;
        boost::split(parts, line, boost::is_any_of("\t"));

        // Filter out non-movie entries
        if (parts[type_index] != "movie") {
            continue;
        }
        // Filter out movies with no genres
        if (parts[genre_index] == "\\N") {
            continue;
        }
        // Use the imdb index (i.e. "ff000451") as the JSON key
        auto& entry = data[parts[imdb_index]];

        // store off the primaryTitle in the entry as "title"
        entry["title"] = parts[title_index];

        // store off each comma separated genre in the entry as "genres"
        std::vector<std::string> genres;
        boost::split(genres, parts[genre_index], boost::is_any_of(","));
        entry["genres"] = genres;
    }

    return data;
}

MovieData::MovieData(const construct& c) {
    // Do not update cache if constructed with_cache, just read from files.
    if (c == construct::with_cache) {
        try {
            this->data = open_json("cache/title_basics.json");
        } catch (...) {
            throw cache_error();
        }
        return;
    }

    // construct::without_cache
    // This url contains a gzipped tsv of all movies in imdb, most notably their
    // identifier. This identifier is used later when grabbing more info.
    const auto url_titles = "https://datasets.imdbws.com/title.basics.tsv.gz";
    this->data = gzip_download_to_json(url_titles);

    std::filesystem::create_directory("cache");
    write_json("cache/title_basics.json", this->data);
}
