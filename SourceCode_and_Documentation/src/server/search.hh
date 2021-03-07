#ifndef SERVER_SEARCH_HH_
#define SERVER_SEARCH_HH_

#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <ranges>
#include <random>

#include "client/client.hh" // MovieData

namespace search {

enum movie_flags {
    MF_FOREIGN = (1 << 0),
    MF_GREYSCALE = (1 << 1),
    MF_SILENT = (1 << 2),
    MF_PRE1980 = (1 << 3),
    MF_ADULT = (1 << 4),
};

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

void initialise(const MovieData& data);

token create_token(const std::int16_t flags = 0);

void advance_token(token& token, const bool remove);


}

#endif
