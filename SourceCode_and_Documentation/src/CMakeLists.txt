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
    -Wconversion -g3 -O2 -Wpedantic -std=c++20
    #-fsanitize=undefined
)

target_link_libraries(webserver PRIVATE
    pthread pistache ssl crypto curl boost_iostreams
)

target_link_options(webserver PRIVATE
    #-fsanitize=undefined
)
