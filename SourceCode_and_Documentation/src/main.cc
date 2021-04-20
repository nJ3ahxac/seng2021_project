#include "main.hh"

int main(const int argc, const char* argv[]) {
    if (argc > 2) {
        std::cout << "Too many arguments: <port=9080>\n";
        std::exit(EXIT_FAILURE);
    }

    std::uint16_t port = PORT_DEFAULT;

    try {
        if (argc > 1) {
            port = boost::lexical_cast<std::uint16_t>(argv[1]);
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

    try {
        std::cout << "Port: " << port << '\n';
        const ServerData server(*moviedata, port);
    } catch (const std::exception& e) {
        std::cerr << "Uncaught server exception: " << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    }
}
