#include "search.hh"

using namespace search;

struct movie_entry {
    std::int32_t imdb_index;
    std::unordered_set<std::string> keywords;
};

static std::vector<movie_entry> movie_entries;
static std::unordered_set<std::string> all_keywords;

// Expects an imdb string with the leading "ff" characters, i.e. "ff00015132"
static std::int32_t tokenise_imdb_index(const std::string& imdb) {
    const auto should_skip_char = [](const char value) {
        return value == 'f' || value == '0';
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

void search::initialise(const MovieData& movies) {
    const json::Document& data = movies.data;

    movie_entries.reserve(data.MemberCount());

    for (const auto& movie : data.GetObject()) {
        movie_entries.emplace_back(movie_entry{
            .imdb_index = tokenise_imdb_index(movie.value["imdb"].GetString()),
            .keywords = array_to_set(movie.value["keywords"])});
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

token search::create_token() {
    return {generate_token_identifier(), get_top_level_token_entries()};
}

// return a float where a greater number means a keyword is more controversial.
static float get_keyword_rating(token& token, const std::string& keyword) {
    const auto entry_has_keyword = [&](const std::int32_t id) {
        return movie_entries[id].keywords.contains(keyword);
    };
    const auto it = std::partition(token.entries.begin(), token.entries.end(),
                                   entry_has_keyword);

    const auto matches = std::distance(token.entries.begin(), it);
    const float threshold = static_cast<float>(matches) / token.entries.size();
    return threshold > 0.5f ? 1.0f - threshold : threshold;
}

void search::advance_token(token& token) {
    const std::size_t partition_index = 1;
}
