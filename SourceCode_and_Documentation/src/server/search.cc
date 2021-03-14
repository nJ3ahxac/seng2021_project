#include "search.hh"

using namespace search;

struct movie_entry {
    std::int32_t imdb_index;
    float rating;
    std::int16_t year, flags;
    std::unordered_set<std::string> genres, keywords;
};

static std::vector<movie_entry> movie_entries;
static std::unordered_set<std::string> all_keywords;
static std::unordered_set<std::string> all_genres;

static const movie_entry& movie_from_index(const std::int32_t index) {
    return movie_entries[static_cast<std::size_t>(index)];
}

// Expects an imdb string with the leading "tt" characters, i.e. "tt00015132"
static std::int32_t tokenise_imdb_index(const std::string& imdb) {
    const auto should_skip_char = [](const char value) {
        return value == 't' || value == '0';
    };
    // find first non zero character
    const auto it = std::ranges::find_if_not(imdb, should_skip_char);
    return std::stoi(std::string{it, imdb.end()});
}

static std::unordered_set<std::string> array_to_set(const json::Value& array, auto& all_container) {
    std::unordered_set<std::string> ret{};
    for (const json::Value& word : array.GetArray()) {
        ret.emplace(word.GetString());
        all_container.emplace(word.GetString());
    }
    return ret;
}

static float calc_movie_rating(const json::Value& entry) {
    constexpr float error_value = 0.0f;
    const json::Value& rating = entry["rating"];
    if (!rating.IsFloat()) {
        return error_value;
    }
    const json::Value& votes = entry["votes"];
    if (!votes.IsInt()) {
        return error_value;
    }
    
    // Prefer higher rated movies but don't recommend movies with few votes.
    const float vote_count = static_cast<float>(votes.GetInt());
    return std::pow(rating.GetFloat() / 10.0f, 10.0f) * vote_count;
}

static std::int16_t calc_movie_flags(const movie_entry& movie, const std::string& languages) {
    std::int16_t flags = 0;
    if (languages.find("English") == std::string::npos) {
        flags |= MF_FOREIGN;
    }
    if (movie.genres.contains("Adult")) {
        flags |= MF_ADULT;
    }
    if (movie.year < 1930) {
        flags |= MF_SILENT;
    }
    if (movie.year < 1950) {
        flags |= MF_GREYSCALE;
    }
    if (movie.year < 1980) {
        flags |= MF_PRE1980;
    }
    return flags;
}

void search::initialise(const MovieData& movies) {
    const json::Document& data = movies.data;

    movie_entries.reserve(data.MemberCount());

    for (const auto& movie : data.GetObject()) {
        auto& entry = movie_entries.emplace_back(movie_entry{
            .imdb_index = tokenise_imdb_index(movie.name.GetString()),
            .rating = calc_movie_rating(movie.value),
            .year = static_cast<std::int16_t>(movie.value["year"].GetInt()),
            .genres = array_to_set(movie.value["genres"], all_genres),
            .keywords = array_to_set(movie.value["keywords"], all_keywords)});
        entry.flags = calc_movie_flags(entry, movie.value["language"].GetString());
    }
}

static std::int64_t generate_token_identifier() {
    return std::random_device{"/dev/random"}();
}

static bool movie_matches_flags(const movie_entry& movie, const std::int16_t flags) {
    // TODO: This can be done with a single line, im sure
    if (movie.flags & MF_FOREIGN && !(flags & MF_FOREIGN)) {
        return false;
    }
    if (movie.flags & MF_GREYSCALE && !(flags & MF_GREYSCALE)) {
        return false;
    }
    if (movie.flags & MF_SILENT && !(flags & MF_SILENT)) {
        return false;
    }
    if (movie.flags & MF_PRE1980 && !(flags & MF_PRE1980)) {
        return false;
    }
    if (movie.flags & MF_ADULT && !(flags & MF_ADULT)) {
        return false;
    }
    return true;
}

static std::vector<std::int32_t> get_top_level_token_entries(const std::int16_t flags) {
    std::vector<std::int32_t> ret;
    ret.reserve(movie_entries.size());

    for (std::size_t i = 0; i < movie_entries.size(); ++i) {
        const auto& movie = movie_entries[i];
        if (movie_matches_flags(movie, flags)) {
            ret.push_back(static_cast<std::int32_t>(i));
        }
    }

    return ret;
}

// return a float where a greater number means a genre is more controversial.
static float get_genre_rating(const token& token, const std::string& genre) {
    const auto entry_has_genre = [&](const std::int32_t id) {
        return movie_entries[static_cast<std::size_t>(id)].genres.contains(genre);
    };
    const auto matches = std::ranges::count_if(token.entries, entry_has_genre);
    const float fraction = static_cast<float>(matches) / static_cast<float>(token.entries.size());
    return fraction > 0.5f ? 1.0f - fraction : fraction;
}

struct genre_rating {
    float rating;
    std::string keyword;
};

// return a std::vector of populated genre_ratings sorted by descending rating
static std::vector<genre_rating> rate_token_genres(const token& token) {
    std::vector<genre_rating> ret;
    ret.reserve(all_genres.size());

    for (const auto& word : all_genres) {
        ret.push_back({get_genre_rating(token, word), word});
    }

    const auto has_better_rating = [](const auto& a, const auto& b) {
        return a.rating > b.rating;
    };
    std::ranges::sort(ret, has_better_rating);
    return ret;
}

// return a float where a greater number means a keyword is more controversial.
static float get_keyword_rating(const token& token, const std::string& keyword) {
    const auto entry_has_keyword = [&](const std::int32_t id) {
        return movie_entries[static_cast<std::size_t>(id)].keywords.contains(keyword);
    };
    const auto matches = std::ranges::count_if(token.entries, entry_has_keyword);
    const float fraction = static_cast<float>(matches) / static_cast<float>(token.entries.size());
    return fraction > 0.5f ? 1.0f - fraction : fraction;
}

struct keyword_rating {
    float rating;
    std::string keyword;
};

// return a std::vector of populated keyword_ratings sorted by descending rating
static std::vector<keyword_rating> rate_token_keywords(const token& token) {
    std::vector<keyword_rating> ret;
    ret.reserve(all_keywords.size());

    for (const auto& word : all_keywords) {
        ret.push_back({get_keyword_rating(token, word), word});
    }

    const auto has_better_rating = [](const auto& a, const auto& b) {
        return a.rating > b.rating;
    };
    std::ranges::sort(ret, has_better_rating);
    return ret;
}

token search::create_token(const std::int16_t flags) {
    token ret = {.identifier = generate_token_identifier(),
            .entries = get_top_level_token_entries(flags),
            .keyword = {},
            .is_filtering_genres = true,
            .suggestion = std::nullopt};
    ret.keyword = rate_token_genres(ret).front().keyword;
    return ret;
}

void search::advance_token(token& token, const bool remove) {
    // Did the previous question filter genres or keywords?
    if (token.is_filtering_genres) {
        const auto should_remove_genre = [&](const std::int32_t index) {
            const auto& genres = movie_from_index(index).genres;
            return genres.contains(token.keyword) ? remove : !remove;
        };
        const auto erase_it = std::remove_if(token.entries.begin(),
                token.entries.end(),
                should_remove_genre);
        token.entries.erase(erase_it, token.entries.end());
    } else {
       const auto should_remove_keyword = [&](const std::int32_t index) {
            const auto& keywords = movie_from_index(index).keywords;
            return keywords.contains(token.keyword) ? remove : !remove;
        };
        const auto erase_it = std::remove_if(token.entries.begin(),
                token.entries.end(),
                should_remove_keyword);
        token.entries.erase(erase_it, token.entries.end());
    }

    if (token.is_filtering_genres) {
        const std::vector<genre_rating> ratings = rate_token_genres(token);
        if (ratings.front().rating != 0.0f) {
            token.keyword = ratings.front().keyword;
            return;
        }
    }

    const std::vector<keyword_rating> ratings = rate_token_keywords(token);
    if (ratings.front().rating != 0.0f) {
        token.keyword = ratings.front().keyword;
        token.is_filtering_genres = false;
        return;
    }

    // No more possible questions to ask - return best according to rating.
    const auto has_better_rating = [](const std::int32_t a, const std::int32_t b) {
        return movie_from_index(a).rating > movie_from_index(b).rating;
    };
    std::ranges::sort(token.entries, has_better_rating);

    token.suggestion = movie_from_index(token.entries.front()).imdb_index;
}