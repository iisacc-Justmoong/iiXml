cmake_minimum_required(VERSION 3.24)
project(iiXml VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(EMSCRIPTEN)
    set(IIXML_BUILD_SHARED_DEFAULT OFF)
else()
    set(IIXML_BUILD_SHARED_DEFAULT ON)
endif()

option(IIXML_BUILD_SHARED "Build iiXml as a shared library." ${IIXML_BUILD_SHARED_DEFAULT})

set(IIXML_LIBRARY_TYPE STATIC)
if(IIXML_BUILD_SHARED)
    set(IIXML_LIBRARY_TYPE SHARED)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(EXISTS "$ENV{HOME}/Qt/6.8.3/macos/lib/cmake/Qt6/Qt6Config.cmake")
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.8.3/macos")
endif()

find_package(Qt6 6.8.3 EXACT REQUIRED COMPONENTS Core)

add_library(iiXml ${IIXML_LIBRARY_TYPE}
    iiXml.cpp
    Src/Elements/ClosedTag.cpp
    Src/Elements/ClosedTag.h
    Src/Elements/Doctype.cpp
    Src/Elements/Doctype.h
    Src/Elements/InlineProperties.cpp
    Src/Elements/InlineProperties.h
    Src/Elements/OpenTag.cpp
    Src/Elements/OpenTag.h
    Src/Parser/FileParser.cpp
    Src/Parser/FileParser.h
    Src/Parser/TagParser.cpp
    Src/Parser/TagParser.h
    Src/Mutation/InlineTagMutation.cpp
    Src/Mutation/InlineTagMutation.h
    Src/Input/GetFile.cpp
    Src/Input/GetFile.h
    Src/Input/GetStringToken.cpp
    Src/Input/GetStringToken.h
    Src/Input/InputValidator.cpp
    Src/Input/InputValidator.h
    Src/Logging/XmlLog.cpp
    Src/Logging/XmlLog.h
)

target_include_directories(iiXml PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(iiXml PUBLIC
    Qt6::Core
)

install(TARGETS iiXml
    EXPORT iiXmlTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES iiXml iiXml.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY Src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Src
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT iiXmlTargets
    FILE iiXmlTargets.cmake
    NAMESPACE iiXml::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXml
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/iiXmlConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/iiXmlConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXml
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/iiXmlConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/iiXmlConfigVersionRoot.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/iiXmlConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/iiXmlConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXml
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

add_executable(open_tag_test
    Test/open_tag_test.cpp
)

target_link_libraries(open_tag_test PRIVATE iiXml)

add_test(NAME open_tag_test COMMAND open_tag_test)

add_executable(closed_tag_test
    Test/closed_tag_test.cpp
)

target_link_libraries(closed_tag_test PRIVATE iiXml)

add_test(NAME closed_tag_test COMMAND closed_tag_test)

add_executable(inline_properties_test
    Test/inline_properties_test.cpp
)

target_link_libraries(inline_properties_test PRIVATE iiXml)

add_test(NAME inline_properties_test COMMAND inline_properties_test)

add_executable(input_validator_test
    Test/input_validator_test.cpp
)

target_link_libraries(input_validator_test PRIVATE iiXml)

add_test(NAME input_validator_test COMMAND input_validator_test)

add_executable(get_file_test
    Test/get_file_test.cpp
)

target_link_libraries(get_file_test PRIVATE iiXml)

add_test(NAME get_file_test COMMAND get_file_test)

add_executable(get_string_token_test
    Test/get_string_token_test.cpp
)

target_link_libraries(get_string_token_test PRIVATE iiXml)

add_test(NAME get_string_token_test COMMAND get_string_token_test)

add_executable(qt_object_signal_test
    Test/qt_object_signal_test.cpp
)

target_link_libraries(qt_object_signal_test PRIVATE iiXml)

add_test(NAME qt_object_signal_test COMMAND qt_object_signal_test)

add_executable(inline_tag_mutation_test
    Test/inline_tag_mutation_test.cpp
)

target_link_libraries(inline_tag_mutation_test PRIVATE iiXml)

add_test(NAME inline_tag_mutation_test COMMAND inline_tag_mutation_test)

add_executable(debug_log_test
    Test/debug_log_test.cpp
)

target_link_libraries(debug_log_test PRIVATE iiXml)

add_test(NAME debug_log_test COMMAND debug_log_test)

add_executable(debug_diagnostics_test
    Test/debug_diagnostics_test.cpp
)

target_link_libraries(debug_diagnostics_test PRIVATE iiXml)

add_test(NAME debug_diagnostics_test COMMAND debug_diagnostics_test)

add_executable(install_layout_test
    Test/install_layout_test.cpp
)

target_compile_definitions(install_layout_test PRIVATE
    IIXML_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME install_layout_test COMMAND install_layout_test)

add_executable(project_policy_test
    Test/project_policy_test.cpp
)

target_compile_definitions(project_policy_test PRIVATE
    IIXML_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME project_policy_test COMMAND project_policy_test)
