cmake_minimum_required(VERSION 4.1)
project(iiXml LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(EXISTS "$ENV{HOME}/Qt/6.8.3/macos/lib/cmake/Qt6/Qt6Config.cmake")
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.8.3/macos")
endif()

find_package(Qt6 6.8.3 EXACT REQUIRED COMPONENTS Core)

add_library(iiXml SHARED
    iiXml.cpp
    Src/Elements/DOCTYPE.cpp
    Src/Elements/DOCTYPE.h
    Src/Parser/FileParser.cpp
    Src/Parser/FileParser.h
    Src/Parser/TagParser.cpp
    Src/Parser/TagParser.h
    Src/Writer/InputValidator.cpp
    Src/Writer/InputValidator.h
        Src/Writer/GetFile.cpp
        Src/Writer/GetFile.h
        Src/Writer/GetStringToken.cpp
        Src/Writer/GetStringToken.h
)

target_include_directories(iiXml PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(iiXml PUBLIC
    Qt6::Core
)

enable_testing()

add_executable(parser_test
    Test/parser_test.cpp
)

target_link_libraries(parser_test PRIVATE iiXml)

add_test(NAME parser_test COMMAND parser_test)

add_executable(file_parser_test
    Test/file_parser_test.cpp
)

target_link_libraries(file_parser_test PRIVATE iiXml)

add_test(NAME file_parser_test COMMAND file_parser_test)

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

add_executable(qt_object_signal_test
    Test/qt_object_signal_test.cpp
)

target_link_libraries(qt_object_signal_test PRIVATE iiXml)

add_test(NAME qt_object_signal_test COMMAND qt_object_signal_test)

add_executable(project_policy_test
    Test/project_policy_test.cpp
)

target_compile_definitions(project_policy_test PRIVATE
    IIXML_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME project_policy_test COMMAND project_policy_test)
