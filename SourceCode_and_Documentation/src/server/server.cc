#include "server.hh"

static std::unordered_map<std::string, std::string> create_resources() {
    std::unordered_map<std::string, std::string> resources;
    // Put new files in files, put new bindings in bindings!
    const std::string files[] = {"/404.html",
                                 "/main.html",
                                 "/main.js",
                                 "/about.html",
                                 "/search.html",
                                 "/search.js",
                                 "/results.html",
                                 "/results.js",
                                 "/res/favicon.ico",
                                 "/res/birds.png",
                                 "/res/charizard.png",
                                 "/res/cinema_mask.png",
                                 "/res/cinemascout.png",
                                 "/res/eye.png",
                                 "/res/horse.png",
                                 "/res/mask.png",
                                 "/res/binoculars.png"};

    // Copies the files into a std::map with the key as the directory.
    //
    const auto insert_file = [&resources](const auto& filename) {
        const std::string file_contents = util::read_file("web" + filename);
        resources.insert({filename, file_contents});
    };
    std::ranges::for_each(files, insert_file);
    return resources;
}

ServerData::ServerData(const MovieData& m, const std::uint16_t port)
    : bindings{{"/", "/main.html"},
               {"/search", "/search.html"},
               {"/results", "/results.html"},
               {"/about", "/about.html"},
               {"/favicon.ico", "/res/favicon.ico"}},
      resources(create_resources()), searchdata(m), tokens(max_token_storage),
      movie_data(m) {
    // Set up handlers for post and get requests, calling either
    // handle_get_request or handle_post_request as appropriate.
    const auto error_wrapper = [&]<bool is_get>(const httplib::Request& request,
                                                httplib::Response& response) {
        try {
            util::log(request.remote_addr + ' ' + request.path + '\n');
            if constexpr (is_get) {
                handle_get_request(request, response);
            } else {
                handle_post_request(request, response);
            }
        } catch (const std::runtime_error& e) {
            const std::string msg =
                "{\"error\":\"" + std::string{e.what()} + "\"}";
            response.status = 400; // bad request
            response.set_content(msg, "text/plain");
        } catch (...) {
            const std::string msg = "{\"error\":\"Unknown\"}";
            response.status = 400; // bad request
            response.set_content(msg, "text/plain");
        }
    };

    const auto bind_routes = [error_wrapper, port](auto& srv) {
        const auto all_regex = "/(.*?)";
        srv.Get(all_regex, [&](const httplib::Request& request,
                                  httplib::Response& response) {
            error_wrapper.operator()<true>(request, response);
        });

        srv.Post(all_regex, [&](const httplib::Request& request,
                                   httplib::Response& response) {
            error_wrapper.operator()<false>(request, response);
        });

        srv.listen("0.0.0.0", port);
    };

    server.emplace<httplib::SSLServer>("cert.pem", "key.pem");
    if (std::get<httplib::SSLServer>(server).is_valid()) {
        bind_routes(std::get<httplib::SSLServer>(server));
    } else {
        // fall back to http
        server.emplace<httplib::Server>();
        bind_routes(std::get<httplib::Server>(server));
    }
}

    
static std::string get_json_str(const json::Document& d,
                                const std::string& value) {
    const auto it = d.FindMember(value);
    if (it == d.MemberEnd()) {
        const std::string msg = "No member for expected JSON key \'" + value +
                                "\' when parsing POST request contents.";
        throw std::runtime_error(msg);
    }
    if (!it->value.IsString()) {
        const std::string msg = "Non-string type of JSON key \'" + value +
                                "\' when parsing POST request contents.";
        throw std::runtime_error(msg);
    }
    return {it->value.GetString(), it->value.GetStringLength()};
}

search::token& ServerData::get_token(const std::int64_t id) {
    const auto token_has_identifier = [id](const search::token& token) {
        return token.identifier == id;
    };
    const auto it = std::ranges::find_if(tokens, token_has_identifier);
    if (it < tokens.end()) {
        return *it;
    }
    const std::string msg =
        "Token of id: \'" + std::to_string(id) + "\' could not be found.";
    throw std::runtime_error(msg);
}

search::token& ServerData::get_token(const json::Document& d) {
    const std::string token_str = get_json_str(d, "token");
    try {
        const std::int64_t id = std::stol(token_str);
        return get_token(id);
    } catch (const std::runtime_error& e) {
        throw e;
    } catch (...) {
        const std::string msg = "Failed to convert token string: \'" +
                                token_str + "\' to an int64_t.";
        throw std::runtime_error(msg);
    }
}

void ServerData::handle_token_init(const json::Document& d,
                                   httplib::Response& response) {
    std::int16_t flags = 0;
    static std::string checked = "true";
    if (get_json_str(d, "is_foreign") == checked) {
        flags |= search::MF_FOREIGN;
    }
    if (get_json_str(d, "is_greyscale") == checked) {
        flags |= search::MF_GREYSCALE;
    }
    if (get_json_str(d, "is_silent") == checked) {
        flags |= search::MF_SILENT;
    }
    if (get_json_str(d, "is_pre1980") == checked) {
        flags |= search::MF_PRE1980;
    }
    if (get_json_str(d, "is_adult") == checked) {
        flags |= search::MF_ADULT;
    }
    tokens.push_front(searchdata.create_token(flags));

    const std::string msg =
        "{\"token\":" + std::to_string(tokens.front().identifier) +
        ", \"max\":" + std::to_string(tokens.front().entries.size()) + "}";

    response.status = 200; // ok
    response.set_content(msg, "text/plain");
}

static std::string escape_json_str(std::string str) {
    static const char to_escape[] = {'\b', '\f', '\n', '\r', '\t', '\"', '\\'};
    for (std::size_t i = 0; i < str.length(); ++i) {
        const auto is_char = [&](const char character) {
            return str[i] == character;
        };
        if (std::ranges::any_of(to_escape, is_char)) {
            str.insert(str.begin() + static_cast<signed>(i), '\\');
            ++i; // prevent infinite loop
        }
    }
    return str;
}

std::string ServerData::movie_info_from_imdb(const std::string& movie_imdb,
                                             const bool verbose) {
    const auto it = movie_data.data.FindMember(movie_imdb);
    if (it >= movie_data.data.MemberEnd()) {
        throw std::runtime_error(
            "Failed to find recommended movie in movie_data");
    }
    const auto entry = it->value.GetObject();

    std::string msg = {};
    const auto add_entry_str = [&](const std::string& elem) {
        if (!msg.empty()) msg += ',';
        msg += '\"' + elem + "\":\"" +
               escape_json_str(entry[elem].GetString()) + '\"';
    };

    const auto add_entry_int = [&](const std::string& elem) {
        if (!msg.empty()) msg += ',';
        msg += '\"' + elem + "\":" + std::to_string(entry[elem].GetInt());
    };

    const auto add_entry_float = [&](const std::string& elem) {
        if (!msg.empty()) msg += ',';
        msg += '\"' + elem + "\":" + std::to_string(entry[elem].GetFloat());
    };

    add_entry_str("title");
    add_entry_int("year");
    add_entry_str("poster");
    if (verbose) {
        add_entry_str("plot");
        add_entry_float("rating");
        add_entry_int("votes");
        add_entry_str("director");
        add_entry_str("awards");
        add_entry_str("language");
        add_entry_str("actors");
        add_entry_str("runtime");
        add_entry_str("box_office");
    }
    return msg;
}

void ServerData::handle_token_info(const json::Document& d,
                                   httplib::Response& response) {
    auto& token = get_token(d);
    const std::string movie_imdb = searchdata.get_suggestion_imdb(token);
    const std::string msg =
        "{\"cur\":" + std::to_string(token.entries.size()) +
        ", \"keyword\": \"" + token.keyword + "\", \"is_genre\": \"" +
        std::string{token.is_filtering_genres ? "true" : "false"} + "\", " +
        movie_info_from_imdb(movie_imdb, false) + "}";

    response.status = 200; // ok
    response.set_content(msg, "text/plain");
}

void ServerData::handle_token_advance(const json::Document& d,
                                      httplib::Response& response) {
    auto& token = get_token(d);
    searchdata.advance_token(token, get_json_str(d, "remove") == "true");

    response.status = 200; // ok
    response.set_content("{}", "text/plain");
}

void ServerData::handle_token_results(const json::Document& d,
                                      httplib::Response& response) {
    auto& token = get_token(d);

    const auto begin_str = get_json_str(d, "begin");
    const auto count_str = get_json_str(d, "count");

    const auto [begin, count] = [&]() -> std::pair<long, long> {
        try {
            return {std::stol(begin_str), std::stol(count_str)};
        } catch (...) {
            throw std::runtime_error(
                "Failed to parse begin and count arguments");
        }
    }();

    if (static_cast<std::size_t>(begin) >= token.entries.size()) {
        throw std::runtime_error("Out of range begin index");
    }
    if (count == 0) {
        throw std::runtime_error("Count cannot be zero");
    }
    const auto end =
        std::min(begin + count, static_cast<long>(token.entries.size() - 1));
    // This will still fail if token.entries.size() == 0, despite the clamping
    if (static_cast<std::size_t>(end) >= token.entries.size()) {
        throw std::runtime_error("Out of range end");
    }

    const std::vector<std::string> imdbs =
        searchdata.get_suggestion_imdbs(token);

    std::string result = "{\"movies\":[";
    const auto append_movie_info = [&](const std::string& imdb) {
        result += '{' + movie_info_from_imdb(imdb, true) + "},";
    };

    std::for_each(imdbs.begin() + begin, imdbs.begin() + end,
                  append_movie_info);

    if (result.back() == ',') {
        result.pop_back(); // hack: pop back last comma in json array
    }
    result += "]}";
    response.status = 200; // ok
    response.set_content(result, "text/plain");
}

void ServerData::handle_post_request(const httplib::Request& request,
                                     httplib::Response& response) {
    json::Document d(json::kObjectType);
    d.Parse(request.body);

    const std::string type = get_json_str(d, "type");

    if (type == "init") {
        handle_token_init(d, response);
    } else if (type == "info") {
        handle_token_info(d, response);
    } else if (type == "advance") {
        handle_token_advance(d, response);
    } else if (type == "results") {
        handle_token_results(d, response);
    } else {
        throw std::runtime_error("Unsupported operation: \'" + type + '\'');
    }
}

void ServerData::handle_get_request(const httplib::Request& request,
                                    httplib::Response& response) {
    // There are three cases for providing files here.
    // 1. The page requested is in our bindings, eg "/" -> "main.html"
    if (const auto it = bindings.find(request.path); it != bindings.end()) {
        response.status = 200; // ok
        response.set_content(resources.at(it->second), "");
        return;
    }

    // 2. The page requested is in our std::map of files (usually .js)
    if (const auto it = resources.find(request.path); it != resources.end()) {
        response.status = 200; // ok
        response.set_content(it->second, "");
        return;
    }

    // 3. The page does not exist, in which case we return 404.
    response.status = 404; // not found
    response.set_content(resources.at("/404.html"), "");
}
