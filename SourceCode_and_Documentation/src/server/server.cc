#include "server.hh"

PageHandler::PageHandler()
    : Pistache::Http::Handler(), bindings{{"/", "/main.html"}}, resources() {
    // Put new files in files, put new bindings in bindings!
    const std::vector<std::string> files = {"/404.html", "/main.html",
                                            "/main.js"};

    // Copies the files into a std::map with the key as the directory.
    for (const auto& filename : files) {
        std::ifstream input("web" + filename);

        if (!input.good()) {
            throw std::runtime_error("Failed to open file: web" + filename);
        }

        std::string file_contents;
        std::copy(std::istreambuf_iterator<char>(input),
                  std::istreambuf_iterator<char>(),
                  std::inserter(file_contents, file_contents.begin()));

        resources.insert({filename, file_contents});
    }
};

void PageHandler::onRequest(const Pistache::Http::Request& request,
                            Pistache::Http::ResponseWriter response) {
    std::cout << "Got request with resource: " << request.resource() << '\n';

    // TODO: handle more than get requests
    if (request.method() != Pistache::Http::Method::Get) {
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

    // 3. The page does not exist, in which case we return 404..
    response.send(Pistache::Http::Code::Not_Found, resources["/404.html"]);
}
