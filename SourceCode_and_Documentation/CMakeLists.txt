cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(webserver)

include_directories(
    src
    $(PROJECT_SOURCE_DIR)/src/server
    $(PROJECT_SOURCE_DIR)/lib/cpp-httplib
)

add_subdirectory(src)
add_subdirectory(lib/googletest)
add_subdirectory(test)
