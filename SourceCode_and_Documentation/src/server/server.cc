#include "server.hh"

PageHandler::PageHandler()
    : Pistache::Http::Handler(), bindings{{"/", "/main.html"},
                                          {"/search", "/search.html"},
                                          {"/about", "/about.html"}},
      resources() {
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
        const std::string file_contents = util::open_file("web" + filename);
        resources.insert({filename, file_contents});
    }
};

void PageHandler::handle_post_request(
    const Pistache::Http::Request& request,
    Pistache::Http::ResponseWriter& response) {
    // TODO: Handle post request here
}

void PageHandler::onRequest(const Pistache::Http::Request& request,
                            Pistache::Http::ResponseWriter response) {
    // std::cout << "Got request with resource: " << request.resource() <<
    // '\n';
    const auto method = request.method();
    if (method != Pistache::Http::Method::Post &&
        method != Pistache::Http::Method::Get) {
        return;
    }

    if (method == Pistache::Http::Method::Post) {
        try {
            handle_post_request(request, response);
        } catch (const std::exception& e) {
            // Return a simple json error message if we can't parse the post.
            std::string resp = "{error: " + std::string(e.what()) + "}";
            response.send(Pistache::Http::Code::Bad_Request, resp);
        }
        return;
    }

    // There are three cases for providing files here.
    // 1. The page requested is in our bindings, eg "/" -> "main.html"
    if (const auto it = bindings.find(request.resource());
        it != bindings.end()) {

        response.send(Pistache::Http::Code::Ok, resources[it->second]);
        return;
    }

    // 2. The page requested is in our std::map of files (usually .js)
    if (const auto it = resources.find(request.resource());
        it != resources.end()) {

        response.send(Pistache::Http::Code::Ok, it->second);
        return;
    }

    // 3. The page does not exist, in which case we return 404.
    response.send(Pistache::Http::Code::Not_Found, resources["/404.html"]);
}
