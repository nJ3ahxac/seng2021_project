#include "server.hh"

static std::unordered_map<std::string, std::string> create_resources() {
    std::unordered_map<std::string, std::string> resources;
    // Put new files in files, put new bindings in bindings!
    const std::vector<std::string> files = {"/404.html",
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
    for (const auto& filename : files) {
        const std::string file_contents = util::read_file("web" + filename);
        resources.insert({filename, file_contents});
    }
    return resources;
}

PageHandler::PageHandler()
    : Pistache::Http::Handler(), bindings{{"/", "/main.html"},
                                          {"/search", "/search.html"},
                                          {"/about", "/about.html"}},
        resources(create_resources()),
        tokens(max_token_storage) {
};

void PageHandler::handle_post_request(
        const Pistache::Http::Request& request,
        Pistache::Http::ResponseWriter& response) {
    
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
}
