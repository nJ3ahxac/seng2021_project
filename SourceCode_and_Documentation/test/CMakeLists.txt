set (BINARY ${CMAKE_PROJECT_NAME}_test)

add_executable(${BINARY}
    main.cc
    tests/server_test.cc
)

add_test(NAME ${BINARY} COMMAND ${BINARY})

target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib gtest)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME}_test POST_BUILD
    COMMAND webserver_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../
    COMMENT "Running tests"
)
