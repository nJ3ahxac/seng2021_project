#include "server_test.hh"

TEST(server_test, visible_web_directory) {
    static const std::string_view files[] = {"404.html", "main.html", "main.js"};

    for (const auto& file : std::filesystem::directory_iterator("web")) {
        const auto file_is = [&file](const std::string_view& name) {
            return file.path().filename() == name;
        };
        ASSERT_TRUE(std::any_of(std::begin(files), std::end(files), file_is));
    }
}
