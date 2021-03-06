set (BINARY ${CMAKE_PROJECT_NAME}_test)

add_executable(${BINARY}
    main.cc
    tests/server_test.cc
)

add_test(NAME ${BINARY} COMMAND ${BINARY})

target_link_libraries(${BINARY} PUBLIC
    ${CMAKE_PROJECT_NAME}_lib
    gtest
    pthread
    pistache
    ssl
    crypto
    curl
    boost_iostreams
)

target_compile_options(${BINARY} PUBLIC
    -Wno-ambiguous-reversed-operator -O2
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME}_test POST_BUILD
    COMMAND webserver_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../
    COMMENT "Running tests"
)
