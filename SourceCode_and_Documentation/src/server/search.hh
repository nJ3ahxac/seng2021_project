#ifndef SERVER_SEARCH_HH_
#define SERVER_SEARCH_HH_

#include <algorithm>
#include <random>
#include <ranges>
#include <string>
#include <unordered_set>
#include <vector>

#include "client/client.hh" // MovieData

namespace search {
struct token {
    // For session identification between requests
    std::int64_t identifier;
    // Movies still considered for watching
    // (internal use only, NOT imdb index)
    std::vector<std::int32_t> entries;
    // Current keyword to send to the client for filtering
    // If keyword.empty(), no more filtering is possible and
    // it is likely that the suggestion optional has a value
    std::string keyword;
    // Are we filtering genres or keywords?
    bool is_filtering_genres;
    // Currently suggested movie (IMDB index)
    std::optional<std::int32_t> suggestion;
};

struct movie_entry {
    std::int32_t imdb_index;
    float rating;
    std::int16_t year, flags;
    std::unordered_set<std::string> genres, keywords;
};

enum movie_flags {
    MF_FOREIGN = (1 << 0),
    MF_GREYSCALE = (1 << 1),
    MF_SILENT = (1 << 2),
    MF_PRE1980 = (1 << 3),
    MF_ADULT = (1 << 4),
};

struct genre_rating {
    float rating;
    std::string keyword;
};
} // namespace search

using namespace search;

class SearchData {
private:
    std::vector<movie_entry> movie_entries;
    std::unordered_set<std::string> all_keywords;
    std::unordered_set<std::string> all_genres;

    struct keyword_rating {
        float rating;
        std::string keyword;
    };

    const movie_entry& movie_from_index(const std::int32_t index);

    std::vector<keyword_rating> rate_token_keywords(const token& token);
    std::vector<genre_rating> rate_token_genres(const token& token);

    float get_keyword_rating(const token& token, const std::string& keyword);
    float get_genre_rating(const token& token, const std::string& genre);
    std::vector<std::int32_t> get_top_level_token_entries(const std::int16_t flags);

public:
    SearchData(const MovieData& data);
    token create_token(const std::int16_t flags = 0);
    void advance_token(token& token, const bool remove);
};

#endif
