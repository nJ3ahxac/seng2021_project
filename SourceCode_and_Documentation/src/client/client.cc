#include "client.hh"

std::string MovieData::download_url(const std::string& url) {
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, *write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &request_contents);

    // If less than 5 bytes during 2 seconds, timeout
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 2);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 5);
    CURLcode result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw std::runtime_error(std::string("Request failed: ") +
                                 curl_easy_strerror(result));
    }

    return request_contents;
}

json::Document MovieData::download_url_json(const std::string& url) {
    json::Document d;
    d.Parse(download_url(url).c_str());
    return d;
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

void MovieData::write_json(const std::string& dir, const json::Document& j) {
    std::ofstream output;
    output.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    output.open(dir);

    json::StringBuffer sb;
    json::Writer<json::StringBuffer> writer(sb);
    j.Accept(writer);
    output << sb.GetString();
}

json::Document MovieData::open_json(const std::string& dir) {
    std::ifstream input;
    input.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    input.open(dir);
    std::string file_contents;
    std::copy(std::istreambuf_iterator<char>(input),
              std::istreambuf_iterator<char>(),
              std::inserter(file_contents, file_contents.begin()));
    json::Document d;
    d.Parse(file_contents.c_str());
    return d;
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

json::Document MovieData::gzip_download_to_json(const std::string& url) {
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

    json::Document data(json::kObjectType);

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
        
        json::Value entry_value(json::kObjectType);

        // store off the primaryTitle in the entry as "title"
        {
            json::Value title_key{"title"};
            json::Value title_value{parts[title_index], data.GetAllocator()};
            entry_value.AddMember(title_key, title_value, data.GetAllocator());
        }

        // store off each comma separated genre in the entry as "genres"
        {
            std::vector<std::string> genres;
            boost::split(genres, parts[genre_index], boost::is_any_of(","));

            json::Value genre_value(json::kArrayType);
            for (const auto& genre : genres) {
                json::Value entry{genre.c_str(), data.GetAllocator()};
                genre_value.PushBack(entry.Move(), data.GetAllocator());
            }
            json::Value genre_key{"genres"};
            entry_value.AddMember(genre_key, genre_value, data.GetAllocator());
        }

        // Use the imdb index (i.e. "ff000451") as the JSON key
        json::Value entry_key{parts[imdb_index], data.GetAllocator()};
        data.AddMember(entry_key, entry_value, data.GetAllocator());
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
