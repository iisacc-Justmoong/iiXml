cmake_minimum_required(VERSION 4.1)
project(iiXml LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(iiXml SHARED
    iiXml.cpp
    Src/Elements/DOCTYPE.cpp
    Src/Parser/TagParser.cpp
    Src/Writer/InputValidator.cpp
)

target_include_directories(iiXml PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

enable_testing()

add_executable(parser_test
    Test/parser_test.cpp
)

target_link_libraries(parser_test PRIVATE iiXml)

add_test(NAME parser_test COMMAND parser_test)

add_executable(doctype_test
    Test/doctype_test.cpp
)

target_link_libraries(doctype_test PRIVATE iiXml)

add_test(NAME doctype_test COMMAND doctype_test)

add_executable(input_validator_test
    Test/input_validator_test.cpp
)

target_link_libraries(input_validator_test PRIVATE iiXml)

add_test(NAME input_validator_test COMMAND input_validator_test)

add_executable(project_policy_test
    Test/project_policy_test.cpp
)

target_compile_definitions(project_policy_test PRIVATE
    IIXML_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME project_policy_test COMMAND project_policy_test)
