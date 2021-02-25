cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(webserver)

include_directories(
    src
)

add_executable(webserver
    src/main.cc
    src/server/server.cc
)

target_compile_options(webserver PRIVATE
    -Wall -Wextra -Wshadow -Wdouble-promotion -Wformat=2 -Wundef -fno-common
    -Wconversion -g3 -O2 -fsanitize=undefined -Wpedantic -std=c++20
    #-Wconversion -g0 -O0 -fsanitize=undefined -Wpedantic -std=c++20
)

target_link_libraries(webserver PRIVATE
    pistache ssl crypto
)

target_link_options(webserver PRIVATE
    -fsanitize=undefined
)
