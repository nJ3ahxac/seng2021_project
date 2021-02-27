set (BINARY ${CMAKE_PROJECT_NAME})

add_executable(${BINARY}
    main.cc
    server/server.cc
    #client/client.cc
)

add_library(${BINARY}_lib STATIC
    main.cc
    server/server.cc
    client/client.cc
)

target_compile_options(webserver PRIVATE
    -Wall -Wextra -Wshadow -Wdouble-promotion -Wformat=2 -Wundef -fno-common
    -Wconversion -g3 -O2 -fsanitize=undefined -Wpedantic -std=c++20
    #-Wconversion -g0 -O0 -fsanitize=undefined -Wpedantic -std=c++20
)

target_link_libraries(webserver PRIVATE
    pistache ssl crypto curl
)

target_link_options(webserver PRIVATE
    -fsanitize=undefined
)
