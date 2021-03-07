#include "search.hh"

using namespace search;

struct movie_entry {
    std::int32_t imdb_index;
    float rating;
    std::unordered_set<std::string> keywords;
};

static std::vector<movie_entry> movie_entries;
static std::unordered_set<std::string> all_keywords;

static const movie_entry& movie_from_index(const std::uint32_t index) {
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

static std::unordered_set<std::string> array_to_set(const json::Value& array) {
    std::unordered_set<std::string> ret{};
    for (const json::Value& word : array.GetArray()) {
        ret.emplace(word.GetString());
        all_keywords.emplace(word.GetString());
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

void search::initialise(const MovieData& movies) {
    const json::Document& data = movies.data;

    movie_entries.reserve(data.MemberCount());

    for (const auto& movie : data.GetObject()) {
        movie_entries.emplace_back(movie_entry{
            .imdb_index = tokenise_imdb_index(movie.name.GetString()),
            .rating = calc_movie_rating(movie.value),
            .keywords = array_to_set(movie.value["genres"])});
    }
}

static std::int64_t generate_token_identifier() {
    return std::random_device{"/dev/random"}();
}

static const std::vector<std::int32_t>& get_top_level_token_entries() {
    static auto top_level = [] {
        std::vector<std::int32_t> ret(movie_entries.size());
        std::ranges::generate(ret, [i = 0]() mutable { return i++; });
        return ret;
    }();

    return top_level;
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

token search::create_token() {
    token ret = {.identifier = generate_token_identifier(),
            .entries = get_top_level_token_entries(),
            .keyword = {},
            .suggestion = std::nullopt};
    static std::string best = rate_token_keywords(ret).front().keyword;
    ret.keyword = best;
    return ret;
}

void search::advance_token(token& token, const bool remove) {
    const auto should_remove = [&](const std::uint32_t index) {
        const auto& keywords = movie_from_index(index).keywords;
        return keywords.contains(token.keyword) ? remove : !remove;
    };
    const auto erase_it = std::remove_if(token.entries.begin(), token.entries.end(), should_remove);
    token.entries.erase(erase_it, token.entries.end());

    const std::vector<keyword_rating> ratings = rate_token_keywords(token);

    if (ratings.front().rating != 0.0f) {
        token.keyword = ratings.front().keyword;
        return;
    }
    // No more possible questions to ask - return best according to rating.
    const auto has_better_rating = [](const std::uint32_t& a, const std::uint32_t& b) {
        return movie_from_index(a).rating > movie_from_index(b).rating;
    };
    std::ranges::sort(token.entries, has_better_rating);

    token.suggestion = movie_from_index(token.entries.front()).imdb_index;
}
