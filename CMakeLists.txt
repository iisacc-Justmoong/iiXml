cmake_minimum_required(VERSION 4.1)
project(iixml LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(iixml SHARED
    iiXml.cpp
    Parser/tag_parser.cpp
)

target_include_directories(iixml PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

enable_testing()

add_executable(parser_test
    tests/parser_test.cpp
)

target_link_libraries(parser_test PRIVATE iixml)

add_test(NAME parser_test COMMAND parser_test)
