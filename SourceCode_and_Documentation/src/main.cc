#include "main.hh"

int main(const int argc, const char* argv[]) {
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

    std::cout << "Database size: "
        << movies->data.MemberCount()
        << " films.\n";

    const Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(port));
    const auto opts = Pistache::Http::Endpoint::options().threads(threads);
    Pistache::Http::Endpoint server(addr);
    server.init(opts);

    try {
        server.setHandler(Pistache::Http::make_handler<PageHandler>());
    } catch (const std::exception& e) {
        std::cerr << "Bad handler set: " << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    }
    // TODO handle exceptions to do nothing after this point

    std::cout << "Port: " << port << "\nThreads: " << threads << '\n';
    server.serve();
}
