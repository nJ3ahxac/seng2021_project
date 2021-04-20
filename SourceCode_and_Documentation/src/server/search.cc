#include "search.hh"

using namespace search;

// Expects an imdb string with the leading "tt" characters, i.e. "tt00015132"
static std::int32_t tokenise_imdb_index(const std::string& imdb) {
    const auto should_skip_char = [](const char value) {
        return value == 't' || value == '0';
    };
    // find first non zero character
    const auto it = std::ranges::find_if_not(imdb, should_skip_char);
    return std::stoi(std::string{it, imdb.end()});
}

static std::unordered_set<std::string> array_to_set(const json::Value& array,
                                                    auto& all_container) {
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

static std::int16_t calc_movie_flags(const movie_entry& movie,
                                     const std::string& languages) {
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

static std::int64_t generate_token_identifier() {
    return std::random_device{"/dev/urandom"}();
}

static bool movie_matches_flags(const movie_entry& movie,
                                const std::int16_t flags) {
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

// SearchData methods.

// return a std::vector of populated genre_ratings sorted by descending rating
std::vector<genre_rating> SearchData::rate_token_genres(const token& token) {
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

const movie_entry& SearchData::movie_from_index(const std::int32_t index) {
    return this->movie_entries[static_cast<std::size_t>(index)];
}

// return a std::vector of populated keyword_ratings sorted by descending rating
std::vector<SearchData::keyword_rating>
SearchData::rate_token_keywords(const token& token) {
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

// return a float where a greater number means a keyword is more controversial.
float SearchData::get_keyword_rating(const token& token,
                                     const std::string& keyword) {
    const auto entry_has_keyword = [&](const std::int32_t id) {
        return movie_entries[static_cast<std::size_t>(id)].keywords.contains(
            keyword);
    };
    const auto matches =
        std::ranges::count_if(token.entries, entry_has_keyword);
    const float fraction =
        static_cast<float>(matches) / static_cast<float>(token.entries.size());
    return fraction > 0.5f ? 1.0f - fraction : fraction;
}

std::vector<std::int32_t>
SearchData::get_top_level_token_entries(const std::int16_t flags) {
    std::vector<std::int32_t> ret;
    ret.reserve(this->movie_entries.size());

    for (std::size_t i = 0; i < this->movie_entries.size(); ++i) {
        const auto& movie = this->movie_entries[i];
        if (movie_matches_flags(movie, flags)) {
            ret.push_back(static_cast<std::int32_t>(i));
        }
    }

    return ret;
}

// return a float where a greater number means a genre is more controversial.
float SearchData::get_genre_rating(const token& token,
                                   const std::string& genre) {
    const auto entry_has_genre = [&](const std::int32_t id) {
        return this->movie_entries[static_cast<std::size_t>(id)]
            .genres.contains(genre);
    };
    const auto matches = std::ranges::count_if(token.entries, entry_has_genre);
    const float fraction =
        static_cast<float>(matches) / static_cast<float>(token.entries.size());
    return fraction > 0.5f ? 1.0f - fraction : fraction;
}

SearchData::SearchData(const MovieData& movies) {
    const json::Document& data = movies.data;

    this->movie_entries.reserve(data.MemberCount());

    for (const auto& movie : data.GetObject()) {
        auto& entry = movie_entries.emplace_back(movie_entry{
            .imdb_index = tokenise_imdb_index(movie.name.GetString()),
            .rating = calc_movie_rating(movie.value),
            .year = static_cast<std::int16_t>(movie.value["year"].GetInt()),
            .genres = array_to_set(movie.value["genres"], all_genres),
            .keywords = array_to_set(movie.value["keywords"], all_keywords)});
        entry.flags =
            calc_movie_flags(entry, movie.value["language"].GetString());
    }

    // Prevent recommending a genre when it appears as a keyword
    for (std::string genre : all_genres) {
        boost::algorithm::to_lower(genre);
        all_keywords.erase(genre);
    }
}

token SearchData::create_token(const std::int16_t flags) {
    token ret = {.identifier = generate_token_identifier(),
                 .entries = get_top_level_token_entries(flags),
                 .keyword = {},
                 .is_filtering_genres = true,
                 .suggestion_needs_update = true};
    ret.keyword = rate_token_genres(ret).front().keyword;
    return ret;
}

void SearchData::advance_token(token& token, const bool remove) {
    // Did the previous question filter genres or keywords?
    if (token.is_filtering_genres) {
        const auto should_remove_genre = [&](const std::int32_t index) {
            const auto& genres = movie_from_index(index).genres;
            return genres.contains(token.keyword) ? remove : !remove;
        };
        const auto erase_it = std::remove_if(
            token.entries.begin(), token.entries.end(), should_remove_genre);
        token.entries.erase(erase_it, token.entries.end());
    } else {
        const auto should_remove_keyword = [&](const std::int32_t index) {
            const auto& keywords = movie_from_index(index).keywords;
            return keywords.contains(token.keyword) ? remove : !remove;
        };
        const auto erase_it = std::remove_if(
            token.entries.begin(), token.entries.end(), should_remove_keyword);
        token.entries.erase(erase_it, token.entries.end());
    }

    token.suggestion_needs_update = true;

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
}

std::size_t SearchData::get_suggestion_size(const token& token) {
    return token.entries.size();
}

std::string SearchData::imdb_str_from_entry(const std::int32_t entry) {
    const std::int32_t imdb_index = movie_from_index(entry).imdb_index;
    const std::string entry_str = std::to_string(imdb_index);
    std::string zero_pad = {};
    if (entry_str.length() < 7) {
        zero_pad = std::string(7ul - entry_str.length(), '0');
    }
    return "tt" + zero_pad + entry_str;
}

void SearchData::sort_entries_by_rating(token& token) {
    const auto has_better_rating = [this](const std::int32_t a,
                                          const std::int32_t b) {
        return movie_from_index(a).rating > movie_from_index(b).rating;
    };
    std::ranges::sort(token.entries, has_better_rating);
}

std::string SearchData::get_suggestion_imdb(token& token) {
    if (token.entries.empty()) {
        throw std::runtime_error("No more possible suggestions to return");
    }

    if (!token.suggestion_needs_update && !token.suggested.empty()) {
        return imdb_str_from_entry(token.suggested.back());
    }
    token.suggestion_needs_update = false;
    sort_entries_by_rating(token);

    const auto movie_was_suggested = [&](const std::int32_t imdb) {
        return std::ranges::find(token.suggested, imdb) < token.suggested.end();
    };
    const auto movie_has_keyword = [&](const std::int32_t imdb) {
        const auto& movie = movie_from_index(imdb);
        if (token.is_filtering_genres) {
            return movie.genres.contains(token.keyword);
        }
        return movie.keywords.contains(token.keyword);
    };

    const auto should_suggest = [&](const std::int32_t imdb) {
        return !movie_was_suggested(imdb) && movie_has_keyword(imdb);
    };
    const auto it = std::ranges::find_if(token.entries, should_suggest);
    if (it < token.entries.end()) {
        token.suggested.emplace_back(*it);
        return imdb_str_from_entry(*it);
    }

    // fall back to old method
    const std::size_t index =
        std::min(token.suggested.size(), token.entries.size() - 1);
    const std::int32_t suggest_imdb = token.entries[index];
    token.suggested.emplace_back(suggest_imdb);
    return imdb_str_from_entry(suggest_imdb);
}

std::vector<std::string> SearchData::get_suggestion_imdbs(token& token) {
    sort_entries_by_rating(token);
    std::vector<std::string> ret;
    ret.reserve(token.entries.size());
    for (const std::int32_t entry : token.entries) {
        ret.emplace_back(imdb_str_from_entry(entry));
    }
    return ret;
}
