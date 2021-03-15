#include "main.hh"

// Sets the file limit, if 0 sets to max available.
[[maybe_unused]] static void set_file_limit(const int n) {
    const auto read_limits = []() {
        rlimit ret;
        getrlimit(RLIMIT_NOFILE, &ret);
        return ret;
    };
    const auto set_limits = [&](const rlimit& r) {
        setrlimit(RLIMIT_NOFILE, &r);
    };
    auto limits = read_limits();
    limits.rlim_cur = n == 0 ? limits.rlim_max : n;
    set_limits(limits);
}

int main(int argc, char* argv[]) {
    set_file_limit(0);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
