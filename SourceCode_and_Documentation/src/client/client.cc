#include "client.hh"

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

static json::Document gzip_download_to_json(const std::string& url) {
    const std::string title_basics =
        util::decompress_gzip(util::download_url(url));
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

    const std::string url_base =
        "http://www.omdbapi.com/?plot=full&apikey=" + get_api_key() + "&i=";

    const auto backup_movie = [&]() -> bool {
        const std::string index = get_next_movie_index();
        if (index.empty()) {
            return false;
        }

        const std::string url = url_base + index;
        const std::string result = util::download_url(url);

        auto stream = std::ofstream("backup/" + index);
        if (!stream.is_open()) {
            throw std::runtime_error("could not open \"backup/" + result +
                                     '\"');
        }
        stream << result;

        return true;
    };

    const auto backup_movie_thread = [&]() {
        while (backup_movie()) {
        }
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
    const auto is_nonalpha = [](const char c) { return !std::isalpha(c); };
    std::replace_if(in.begin(), in.end(), is_nonalpha, ' ');

    // remove consecutive spacebars
    boost::algorithm::trim_all(in);

    // insert each spacebar separated word into the unordered set
    std::unordered_set<std::string> words;
    boost::split(words, in, boost::is_any_of(" "));
    return words;
}

static std::string get_keyword_list() {
    try {
        return util::read_file("keywords.txt");
    } catch (...) {
        const std::string url =
            "http://www.desiquintans.com/downloads/nounlist/nounlist.txt";
        return util::download_url(url);
    }
}

static std::unordered_set<std::string> get_keywords() {
    std::unordered_set<std::string> keywords;
    boost::split(keywords, get_keyword_list(), boost::is_any_of("\n"));
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

    const std::string url_base =
        "http://www.omdbapi.com/?plot=full&apikey=" + get_api_key() + "&i=";
    const std::unordered_set<std::string> keywords = get_keywords();

    const auto download_movie = [&]() -> bool {
        const auto movie_it_opt = get_next_movie_it();
        if (!movie_it_opt.has_value()) {
            return false;
        }
        const auto& movie_it = *movie_it_opt;

        const std::string index = movie_it->name.GetString();

        const json::Document result = util::download_url_json(url_base + index);

        const std::unique_lock<std::mutex> guard(lock);
        const auto result_has_good = [&](const std::string& str) {
            return result.HasMember(str) && result[str].IsString();
        };

        static const std::string elements[] = {
            "Plot",    "imdbRating", "imdbVotes", "Language",
            "Year",    "Director",   "Awards",    "Actors",
            "Runtime", "BoxOffice",  "Poster"};

        if (!result.IsObject() ||
            !std::ranges::all_of(elements, result_has_good)) {
            return true;
        }

        json::Value keyword_value(json::kArrayType);
        for (const std::string& word :
             string_words(result["Plot"].GetString())) {
            if (!keywords.contains(word)) {
                continue;
            }

            json::Value entry(word.c_str(), doc.GetAllocator());
            keyword_value.PushBack(entry.Move(), doc.GetAllocator());
        }
        json::Value keyword_key{"keywords"};
        movie_it->value.AddMember(keyword_key, keyword_value,
                                  doc.GetAllocator());

        const float rating = [&] {
            try {
                return std::stof(result["imdbRating"].GetString());
            } catch (...) {
                return 0.0f;
            }
        }();
        json::Value rating_key{"rating"};
        movie_it->value.AddMember(rating_key, rating, doc.GetAllocator());

        const int votes = [&] {
            try {
                std::string votes = result["imdbVotes"].GetString();
                // remove commas returned by api
                boost::erase_all(votes, ",");
                return std::stoi(votes);
            } catch (...) {
                return 0;
            }
        }();
        json::Value votes_key{"votes"};
        movie_it->value.AddMember(votes_key, votes, doc.GetAllocator());

        const int year = [&] {
            try {
                // Can't call GetInt with rapidjson since the API returns
                // {"Year", "1985"} instead of {"Year", 1985} for example.
                return std::stoi(result["Year"].GetString());
            } catch (...) {
                return 1970;
            }
        }();
        json::Value year_key{"year"};
        movie_it->value.AddMember(year_key, year, doc.GetAllocator());

        json::Value language_key{"language"};
        json::Value language_value{result["Language"].GetString(),
                                   doc.GetAllocator()};
        movie_it->value.AddMember(language_key, language_value,
                                  doc.GetAllocator());

        json::Value plot_key{"plot"};
        json::Value plot_value{result["Plot"].GetString(), doc.GetAllocator()};
        movie_it->value.AddMember(plot_key, plot_value, doc.GetAllocator());

        json::Value director_key{"director"};
        json::Value director_value{result["Director"].GetString(),
                                   doc.GetAllocator()};
        movie_it->value.AddMember(director_key, director_value,
                                  doc.GetAllocator());

        json::Value awards_key{"awards"};
        json::Value awards_value{result["Awards"].GetString(),
                                 doc.GetAllocator()};
        movie_it->value.AddMember(awards_key, awards_value, doc.GetAllocator());

        json::Value actors_key{"actors"};
        json::Value actors_value{result["Actors"].GetString(),
                                 doc.GetAllocator()};
        movie_it->value.AddMember(actors_key, actors_value, doc.GetAllocator());

        json::Value runtime_key{"runtime"};
        json::Value runtime_value{result["Runtime"].GetString(),
                                  doc.GetAllocator()};
        movie_it->value.AddMember(runtime_key, runtime_value,
                                  doc.GetAllocator());

        json::Value box_office_key{"box_office"};
        json::Value box_office_value{result["BoxOffice"].GetString(),
                                     doc.GetAllocator()};
        movie_it->value.AddMember(box_office_key, box_office_value,
                                  doc.GetAllocator());

        json::Value poster_key{"poster"};
        json::Value poster_value{result["Poster"].GetString(),
                                 doc.GetAllocator()};
        movie_it->value.AddMember(poster_key, poster_value, doc.GetAllocator());

        return true;
    };

    const std::size_t likely_max =
        doc.GetObject().MemberCount() / max_concurrent;

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
            std::cout << std::clamp(percent, 0ul, 100ul) << "%\n";
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
    for (auto it = object.begin(); it < object.end();) {
        auto& entry = it->value;
        bool remove = false;
        // No plot entry in OMDB/no keywords?
        if (!entry.HasMember("keywords") ||
            entry["keywords"].GetArray().Size() < 1) {
            remove = true;
        }
        // Insufficient rating?
        if (!entry.HasMember("rating") || entry["rating"].GetFloat() == 0.0f) {
            remove = true;
        }
        // No rating votes?
        if (!entry.HasMember("votes") || entry["votes"].GetInt() == 0) {
            remove = true;
        }

        // No poster?
        if (!entry.HasMember("poster") ||
            std::string{entry["poster"].GetString()} == "N/A") {
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

MovieData::MovieData(const construct& c, const std::string& cache_dir) {
    const std::string cache_file = "title_basics.json";
    // Do not update cache if constructed with_cache, just read from files.
    if (c == construct::with_cache) {
        try {
            this->data = util::read_file_json(cache_dir + cache_file);
        } catch (std::exception& e) {
            throw cache_error(e.what());
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
    // Download all movie plots from omdb and store keywords to be searched
    // with.
    update_movie_omdb_data(this->data);
    // Remove entries with little data or relevance (i.e. no keywords or low
    // rating)
    prune_movie_data(this->data);

    std::filesystem::create_directory(cache_dir);
    util::write_file_json(cache_dir + cache_file, this->data);
}

MovieData::MovieData(const MovieData& md) {
    this->data.CopyFrom(md.data, this->data.GetAllocator());
}

MovieData MovieData::operator=(const MovieData& md) {
    this->data.CopyFrom(md.data, this->data.GetAllocator());
    return *this;
}
