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
    // Currently suggested movie (IMDB index)
    std::optional<std::int32_t> suggestion;
};

void initialise(const MovieData& data);

token create_token();

void advance_token(token& token, const bool remove);


}

#endif
