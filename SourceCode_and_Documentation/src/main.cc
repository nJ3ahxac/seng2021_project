#include "main.hh"

[[maybe_unused]] static void test_textmode_client() {
    std::optional<MovieData> movies;
    try {
        std::cout << "Loading pre-existing movie data cache.\n";
        movies = MovieData(MovieData::construct::with_cache);
    } catch (const MovieData::cache_error& e) {
        std::cout << "Failed to load movie data from pre-existing cache.\n"
                     "Regenerating cache. This may take a while.\n";
        movies = MovieData(MovieData::construct::without_cache);
        std::cout << "Movie data cache regenerated.\n";
    } catch (const std::exception& e) {
        std::cerr << "Bad movie data initialisation: " << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    }

    std::cout << "Initialising search database with "
              << movies->data.MemberCount() << " films.\n";

    SearchData searchdata{*movies};

    std::cout << "Search initialized, creating token\n";

    search::token t = searchdata.create_token();
    while (!t.suggestion.has_value()) {
        std::cout << t.entries.size() << " movies left, best keyword is "
                  << t.keyword << '\n';
        std::cout << "(r)emove or (k)eep keyword? ";

        char answer;
        std::cin >> answer;
        if (answer != 'r' && answer != 'k') {
            continue;
        }

        searchdata.advance_token(t, answer == 'r');
    }

    std::cout << "Your movie suggestion is IMDB index " << t.suggestion.value()
              << '\n';
    if (t.entries.size() > 1) {
        std::cout << "However, there are " << t.entries.size() - 1
                  << " alternatives\n";
    }

    std::exit(EXIT_SUCCESS);
}

int main(const int argc, const char* argv[]) {
    // test_textmode_client();
    if (argc > 3) {
        std::cout << "Too many arguments: <port=9080> <threads=1>\n";
        std::exit(EXIT_FAILURE);
    }

    std::uint16_t port = PORT_DEFAULT;
    int threads = THREADS_DEFAULT;

    try {
        if (argc > 1) {
            port = boost::lexical_cast<std::uint16_t>(argv[1]);
        }
        if (argc > 2) {
            threads = boost::lexical_cast<int>(argv[2]);
            if (threads == 0) {
                threads = static_cast<int>(std::thread::hardware_concurrency());
            }
        }
    } catch (const boost::bad_lexical_cast& e) {
        std::cerr << "Bad argument conversion: " << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    }

    // Attempt to initialise MovieData via our cache. If it doesn't exist,
    // we will redownload the cache (lest our server serves no data).
    std::optional<MovieData> moviedata;
    try {
        std::cout << "Loading pre-existing movie data cache.\n";
        moviedata = MovieData(MovieData::construct::with_cache);
    } catch (const MovieData::cache_error& e) {
        std::cout << "Failed to load movie data from pre-existing cache.\n"
                     "Regenerating cache. This may take a while.\n";
        moviedata = MovieData(MovieData::construct::without_cache);
        std::cout << "Movie data cache regenerated.\n";
    } catch (const std::exception& e) {
        std::cerr << "Bad movie data initialisation: " << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    }

    std::cout << "Initialising search database with: "
              << moviedata->data.MemberCount() << " films.\n";


    httplib::Server server;

/*void ServerData::onRequest(const httplib::Request& request,
                            httplib::Response response) {
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
        response.status = 400; // bad request
        response.set_content(msg, "text/plain");
    } catch (...) {
        const std::string msg = "{\"error\":\"Unknown\"}";
        response.status = 400; // bad request
        response.set_content(msg, "text/plain");
    }
}*/
    ServerData server_data(*moviedata);

    const auto error_wrapper = [&](const bool is_get,
            const httplib::Request& request,
            httplib::Response& response) {
        try {
            if (is_get) {
                server_data.handle_get_request(request, response);
            } else {
                server_data.handle_post_request(request, response);
            }
        } catch (const std::runtime_error& e) {
            const std::string msg = "{\"error\":\"" + std::string{e.what()} + "\"}";
            response.status = 400; // bad request
            response.set_content(msg, "text/plain");
        } catch (...) {
            const std::string msg = "{\"error\":\"Unknown\"}";
            response.status = 400; // bad request
            response.set_content(msg, "text/plain");
        }
    };

    const auto get_fn = [&](const httplib::Request& request, httplib::Response& response) {
        error_wrapper(true, request, response);
    };
    
    const auto post_fn = [&](const httplib::Request& request, httplib::Response& response) {
        error_wrapper(false, request, response);
    };

    const auto all = "/(.*?)";
    server.Get(all, get_fn);
    server.Post(all, post_fn);

    try {
        std::cout << "Port: " << port << "\nThreads: " << threads << '\n';
        server.listen("0.0.0.0", port);
    }
    catch (const std::exception& e) {
        std::cerr << "Uncaught server exception: " << e.what() << '\n';
    }
}
