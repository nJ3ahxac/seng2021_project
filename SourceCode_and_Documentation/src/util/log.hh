#ifndef UTIL_LOG_HH_
#define UTIL_LOG_HH_

#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>

#include "util.hh"

namespace util {
// very basic threadsafe logging, includes time by default
class log {
private:
    static inline std::mutex lock;

public:
    log(const std::string& message, const bool print_time = true) {
        const std::lock_guard<std::mutex> guard(this->lock);

        auto file = util::open_file("/tmp/webserver.log", std::ios_base::app);

        if (print_time) {
            const auto epoch = std::time(nullptr);
            struct tm* local = localtime(&epoch);
            file << std::put_time(local, "[%F %T] ");
        }

        file << message;
    }
};

} // namespace util

#endif
