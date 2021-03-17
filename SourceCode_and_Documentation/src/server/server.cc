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

PageHandler::PageHandler()
    : Pistache::Http::Handler(), bindings{{"/", "/main.html"},
                                          {"/search", "/search.html"},
                                          {"/about", "/about.html"}},
        resources(create_resources()),
        tokens(max_token_storage) {
}

static std::string get_json_str(const json::Document& d, const std::string& value) {
    const auto it = d.FindMember(value);
    if (it == d.MemberEnd()) {
        const std::string msg = "No member for expected JSON key \'"
            + value
            + "\' when parsing POST request contents.";
        throw std::runtime_error(msg);
    }
    if (!it->value.IsString()) {
        const std::string msg = "Non-string type of JSON key \'"
            + value
            + "\' when parsing POST request contents.";
        throw std::runtime_error(msg);
    }
    return {it->value.GetString(), it->value.GetStringLength()};
}

search::token& PageHandler::get_token(const std::int64_t id) {
    const auto token_has_identifier = [id](const search::token& token) {
        return token.identifier == id;
    };
    const auto it = std::ranges::find_if(tokens, token_has_identifier);
    if (it < tokens.end()) {
        return *it;
    }
    const std::string msg = "Token of id: \'"
        + std::to_string(id)
        + "\' could not be found.";
    throw std::runtime_error(msg);
}

search::token& PageHandler::get_token(const json::Document& d) {
    const std::string token_str = get_json_str(d, "token");
    try {
        const std::int64_t id = std::stol(token_str);
        return get_token(id);
    } catch (const std::runtime_error& e) {
        throw e;
    } catch (...) {
        const std::string msg = "Failed to convert token string: \'"
            + token_str
            + "\' to an int64_t.";
        throw std::runtime_error(msg);
    }
}

void PageHandler::handle_token_init(const json::Document& d,
        Pistache::Http::ResponseWriter& response) {
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
    tokens.push_front(search::create_token(flags));
    
    const std::string msg = "{\"token\":"
        + std::to_string(tokens.front().identifier)
        + ", \"max\":"
        + std::to_string(tokens.front().entries.size())
        + "}";
    response.send(Pistache::Http::Code::Ok, msg);
}

void PageHandler::handle_token_info(const json::Document& d,
        Pistache::Http::ResponseWriter& response) {
    auto& token = get_token(d);
    const std::string msg = "{\"cur\":"
        + std::to_string(token.entries.size())
        + ", \"keyword\": \""
        + token.keyword
        + "\", \"is_genre\": \""
        + std::string{token.is_filtering_genres ? "true" : "false"}
        + "\"}";
    response.send(Pistache::Http::Code::Ok, msg);
}

void PageHandler::handle_token_advance(const json::Document& d,
        Pistache::Http::ResponseWriter& response) {
    auto& token = get_token(d);
    search::advance_token(token, get_json_str(d, "remove") == "true");
    response.send(Pistache::Http::Code::Ok, "{}");
}

void PageHandler::handle_token_results(const json::Document& d,
        Pistache::Http::ResponseWriter& response) {
    throw std::runtime_error("Unimplemented operation");
}

void PageHandler::handle_post_request(
        const Pistache::Http::Request& request,
        Pistache::Http::ResponseWriter& response) {
    response.headers().add<Pistache::Http::Header::ContentType>(MIME(Application, Json));
    json::Document d(json::kObjectType);
    d.Parse(request.body());

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

void PageHandler::handle_get_request(
        const Pistache::Http::Request& request,
        Pistache::Http::ResponseWriter& response) {
    // There are three cases for providing files here.
    // 1. The page requested is in our bindings, eg "/" -> "main.html"
    if (const auto it = bindings.find(request.resource());
        it != bindings.end()) {
        response.send(Pistache::Http::Code::Ok, resources.at(it->second));
        return;
    }

    // 2. The page requested is in our std::map of files (usually .js)
    if (const auto it = resources.find(request.resource());
        it != resources.end()) {
        response.send(Pistache::Http::Code::Ok, it->second);
        return;
    }

    // 3. The page does not exist, in which case we return 404.
    response.send(Pistache::Http::Code::Not_Found, resources.at("/404.html"));
}

void PageHandler::onRequest(const Pistache::Http::Request& request,
                            Pistache::Http::ResponseWriter response) {
    try { 
        switch (request.method()) {
        case Pistache::Http::Method::Post:
            handle_post_request(request, response);
            break;
        case Pistache::Http::Method::Get:
            handle_get_request(request, response);
            break;
        default:
            break;
        }
    } catch (const std::runtime_error& e) {
        const std::string msg = "{\"error\":\"" + std::string{e.what()} + "\"}";
        response.send(Pistache::Http::Code::Bad_Request, msg);
    } catch (...) {
        const std::string msg = "{\"error\":\"Unknown\"}";
        response.send(Pistache::Http::Code::Bad_Request, msg);
    }
}
