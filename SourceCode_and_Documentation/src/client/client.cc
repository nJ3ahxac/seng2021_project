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

    //curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 2);
    //curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 5);
    CURLcode result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw std::runtime_error('\"'
                + url
                + "\" request failed: "
                + curl_easy_strerror(result));
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

static std::string get_api_key() {
    const std::string file = "key.txt";
    std::ifstream key_file(file);
    if (!key_file.is_open()) {
        throw std::runtime_error("Failed to read \"" + file + '\"');
    }

    std::string api_key;
    key_file >> api_key;

    if (api_key.length() != 8) {
        throw std::runtime_error("Likely bad api key in \"" + file + '\"');
    }

    return api_key;
}

constexpr auto max_concurrent = 100ul;

[[maybe_unused]] static void backup_all_movies(const json::Document& doc) {
    std::filesystem::create_directory("backup");

    std::vector<std::thread> threads;
    threads.reserve(max_concurrent);

    std::mutex lock;

    auto it = doc.GetObject().MemberBegin();
    const auto end = doc.GetObject().MemberEnd();

    const auto get_next_movie_index = [&]() -> std::string {
        const std::unique_lock<std::mutex> guard(lock);
        if (it >= end) {
            return {};
        }
        return (it++)->name.GetString();
    };

    const std::string url_base = "http://www.omdbapi.com/?plot=full&apikey="
        + get_api_key()
        + "&i=";

    const auto backup_movie = [&]() -> bool {
        const std::string index = get_next_movie_index();
        if (index.empty()) {
            return false;
        }

        const std::string url = url_base + index;
        const std::string result = MovieData::download_url(url);

        auto stream = std::ofstream("backup/" + index);
        if (!stream.is_open()) {
            throw std::runtime_error("could not open \"backup/" + result + '\"');
        }
        stream << result;
        
        return true;
    };

    const auto backup_movie_thread = [&]() {
        while (backup_movie()) {}
    };

    while (threads.size() < max_concurrent) {
        threads.emplace_back(backup_movie_thread);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

static std::unordered_set<std::string> string_words(std::string in) {
    // replace nonalphanumeric characters with spacebars
    const auto is_nonalpha = [](const char c) {
        return !std::isalpha(c);
    };
    std::replace_if(in.begin(), in.end(), is_nonalpha, ' ');

    // remove consecutive spacebars
    boost::algorithm::trim_all(in);

    // insert each spacebar separated word into the unordered set
    std::unordered_set<std::string> words;
    boost::split(words, in, boost::is_any_of(" "));
    return words;
}

static std::unordered_set<std::string> get_keywords() {
    const std::string url = "http://www.desiquintans.com/downloads/nounlist/nounlist.txt";
    const std::string nouns = MovieData::download_url(url);

    std::unordered_set<std::string> keywords;
    boost::split(keywords, nouns, boost::is_any_of("\n"));

    return keywords;
}

static void update_movie_omdb_data(json::Document& doc) {
    std::vector<std::thread> threads;
    threads.reserve(max_concurrent);

    std::mutex lock;

    auto it = doc.GetObject().MemberBegin();
    const auto end = doc.GetObject().MemberEnd();

    const auto get_next_movie_it = [&]() -> std::optional<decltype(it)> {
        const std::unique_lock<std::mutex> guard(lock);
        if (it >= end) {
            return std::nullopt;
        }
        return it++;
    };

    const std::string url_base = "http://www.omdbapi.com/?plot=full&apikey="
        + get_api_key()
        + "&i=";
    const std::unordered_set<std::string> keywords = get_keywords();

    const auto download_movie = [&]() -> bool {
        const auto movie_it_opt = get_next_movie_it();
        if (!movie_it_opt.has_value()) {
            return false;
        }
        const auto& movie_it = *movie_it_opt;

        const std::string index = movie_it->name.GetString();

        std::optional<json::Document> result_opt;
        while (!result_opt.has_value()) {
            try {
                result_opt = MovieData::download_url_json(url_base + index);
            } catch (...) {
                continue;
            }

            const auto result_has_good = [&](const std::string& str) {
                return result_opt->HasMember(str) && (*result_opt)[str].IsString();
            };

            if (!result_opt->IsObject()
                    || !result_has_good("Plot")
                    || !result_has_good("imdbRating")
                    || !result_has_good("imdbVotes")
                    || !result_has_good("Language")) {
                return true;
            }
        }
        const auto& result = *result_opt;

        json::Value keyword_value(json::kArrayType);
        for (const std::string& word : string_words(result["Plot"].GetString())) {
            if (!keywords.contains(word)) {
                continue;
            }

            json::Value entry(word.c_str(), doc.GetAllocator());
            keyword_value.PushBack(entry.Move(), doc.GetAllocator());
        }
        json::Value keyword_key{"keywords"};
        movie_it->value.AddMember(keyword_key, keyword_value, doc.GetAllocator());

        const float rating = [&] {
            try {
                const float rating = std::stof(result["imdbRating"].GetString());

                // remove commas returned by api
                std::string votes = result["imdbVotes"].GetString();
                boost::erase_all(votes, ",");

                return rating * static_cast<float>(std::stoi(votes));
            } catch (...) {
                return 0.0f;
            }
        }();

        json::Value rating_key{"rating"};
        movie_it->value.AddMember(rating_key, rating, doc.GetAllocator());

        json::Value language_key{"language"};
        json::Value language_value{result["Language"].GetString(), doc.GetAllocator()};
        movie_it->value.AddMember(language_key, language_value, doc.GetAllocator());

        return true;
    };

    const std::size_t likely_max = doc.GetObject().MemberCount() / max_concurrent;

    const auto download_movie_thread = [&](const bool report) {
        for (std::size_t count = 0; download_movie(); ++count) {
            if (!report) {
                continue;
            }
            const std::size_t last_percent = ((count - 1) * 100) / likely_max;
            const std::size_t percent = (count * 100) / likely_max;
            if (last_percent == percent) {
                continue;
            }
            std::cout << percent << "%\n";
        }
    };

    while (threads.size() < max_concurrent) {
        threads.emplace_back(download_movie_thread, threads.empty());
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

static void prune_movie_data(json::Document& doc) {
    const auto& object = doc.GetObject();
    for (auto it = object.begin(); it < object.end(); ) {
        auto& entry = it->value;
        bool remove = false;
        // No plot entry in OMDB/no keywords?
        if (!entry.HasMember("keywords") || entry["keywords"].GetArray().Size() < 1) {
            remove = true;
        }
        // Insufficient rating?
        if (!entry.HasMember("rating") || entry["rating"].GetFloat() == 0.0f) {
            remove = true;
        }

        // See rapidjson docs "Modify Object" for why this works
        if (remove) {
            doc.RemoveMember(it);
        } else {
            ++it;
        }
    }
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

    constexpr auto do_backup = false;
    if constexpr (do_backup) {
        backup_all_movies(this->data);
    }

    // construct::without_cache
    // This url contains a gzipped tsv of all movies in imdb, most notably their
    // identifier and their media type. Their identifier is used later, but for
    // now it is possible to filter out all non-movie entries from our dataset.
    const auto url_titles = "https://datasets.imdbws.com/title.basics.tsv.gz";
    this->data = gzip_download_to_json(url_titles);
    // Download all movie plots from omdb and store keywords to be searched with.
    update_movie_omdb_data(this->data);
    // Remove entries with little data or relevance (i.e. no keywords or low rating)
    prune_movie_data(this->data);

    std::filesystem::create_directory("cache");
    write_json("cache/title_basics.json", this->data);
}
