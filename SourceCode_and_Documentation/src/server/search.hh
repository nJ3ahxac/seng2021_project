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
    token(const std::int64_t i, const std::vector<std::int32_t>& e)
        : identifier(i)
        , entries(e) {}
    token(token& other) = delete;

    std::int64_t identifier;
    std::vector<std::int32_t> entries;
};

void initialise(const MovieData& data);

token create_token();

void advance_token(token& token);


}

#endif
