cmake_minimum_required(VERSION 3.24)
project(iiXml VERSION 0.1.1 LANGUAGES CXX)

# The native package is selectable without discovering any Qt package. Existing
# source consumers retain the legacy API unless they opt out explicitly.
option(IIXML_BUILD_QT_API "Build the legacy QObject XML API" ON)
option(IIXML_BUILD_HTML_BLOCK "Build the integrated HTML block component" ON)
if(PROJECT_IS_TOP_LEVEL AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(_sdk_home "$ENV{HOME}")
    if(_sdk_home STREQUAL "")
        set(_sdk_home "$ENV{USERPROFILE}")
    endif()
    set(CMAKE_INSTALL_PREFIX "${_sdk_home}/.local/SDK/iiXml" CACHE PATH
        "Installation prefix for iiXml." FORCE)
    unset(_sdk_home)
endif()
include(CTest)
add_subdirectory(Native)
if(NOT IIXML_BUILD_QT_API)
    return()
endif()

set(CMAKE_CXX_STANDARD 23)
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

if(EXISTS "/Volumes/Storage/Qt/6.8.3/macos/lib/cmake/Qt6/Qt6Config.cmake")
    list(PREPEND CMAKE_PREFIX_PATH "/Volumes/Storage/Qt/6.8.3/macos")
endif()

find_package(Qt6 6.8.3 EXACT REQUIRED COMPONENTS Core)

add_library(iiXml ${IIXML_LIBRARY_TYPE}
    src/iiXml.cpp
    src/Elements/ClosedTag.cpp
    src/Elements/ClosedTag.h
    src/Elements/Doctype.cpp
    src/Elements/Doctype.h
    src/Elements/InlineProperties.cpp
    src/Elements/InlineProperties.h
    src/Elements/OpenTag.cpp
    src/Elements/OpenTag.h
    src/Parser/FileParser.cpp
    src/Parser/FileParser.h
    src/Parser/TagParser.cpp
    src/Parser/TagParser.h
    src/Mutation/InlineTagMutation.cpp
    src/Mutation/InlineTagMutation.h
    src/Input/GetFile.cpp
    src/Input/GetFile.h
    src/Input/FileReader.h
    src/Input/GetStringToken.cpp
    src/Input/GetStringToken.h
    src/Input/InputValidator.cpp
    src/Input/InputValidator.h
    src/Logging/XmlLog.cpp
    src/Logging/XmlLog.h
)

# Preserve historical <Src/...> consumers with forwarding headers. All installed
# spellings resolve to one canonical header, including on case-sensitive hosts.
file(GLOB_RECURSE _iixml_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
foreach(_header IN LISTS _iixml_headers)
    file(RELATIVE_PATH _relative "${CMAKE_CURRENT_SOURCE_DIR}/src" "${_header}")
    if(_relative MATCHES "^Html/" AND NOT IIXML_BUILD_HTML_BLOCK)
        continue()
    endif()
    set(_canonical "${_relative}")
    set(_compat_kind xml)
    if(_relative MATCHES "^Html/(.*)$")
        set(_relative "${CMAKE_MATCH_1}")
        set(_compat_kind html)
    endif()
    get_filename_component(_directory "${CMAKE_CURRENT_BINARY_DIR}/compat/Src/${_relative}" DIRECTORY)
    file(MAKE_DIRECTORY "${_directory}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/compat/Src/${_relative}"
        "#pragma once\n#include <src/${_canonical}>\n")
    foreach(_output "src/${_canonical}" "Src/${_relative}")
        get_filename_component(_directory "${CMAKE_CURRENT_BINARY_DIR}/install-compat/${_compat_kind}/${_output}" DIRECTORY)
        file(MAKE_DIRECTORY "${_directory}")
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/install-compat/${_compat_kind}/${_output}"
            "#pragma once\n#include <iiXmlDetail/${_canonical}>\n")
    endforeach()
endforeach()
foreach(_name iiXml iiHtmlBlock)
    if(_name STREQUAL "iiHtmlBlock")
        set(_canonical "Html/iiHtmlBlock.h")
    else()
        set(_canonical "iiXml.h")
    endif()
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/install-compat/roots")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/install-compat/roots/${_name}.h"
        "#pragma once\n#include <iiXmlDetail/${_canonical}>\n")
endforeach()

target_include_directories(iiXml PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/compat>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(iiXml PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE IMPORTED_NO_SYSTEM TRUE)
target_link_libraries(iiXml PUBLIC
    Qt6::Core
)

if(IIXML_BUILD_HTML_BLOCK)
    file(GLOB_RECURSE _html_sources CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Html/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Html/*.h")
    add_library(iiXmlHtmlBlock ${IIXML_LIBRARY_TYPE} ${_html_sources})
    add_library(iiXml::HtmlBlock ALIAS iiXmlHtmlBlock)
    add_library(iiHtmlBlock::iiHtmlBlock ALIAS iiXmlHtmlBlock)
    set_target_properties(iiXmlHtmlBlock PROPERTIES EXPORT_NAME HtmlBlock INSTALL_RPATH_USE_LINK_PATH TRUE IMPORTED_NO_SYSTEM TRUE)
    target_compile_features(iiXmlHtmlBlock PUBLIC cxx_std_23)
    target_include_directories(iiXmlHtmlBlock PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Html>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/compat>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(iiXmlHtmlBlock PUBLIC iiXml Qt6::Core)
    install(TARGETS iiXmlHtmlBlock EXPORT iiXmlTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES src/Html/iiHtmlBlock "${CMAKE_CURRENT_BINARY_DIR}/install-compat/roots/iiHtmlBlock.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(DIRECTORY src/Html/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iiXmlDetail/Html FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/install-compat/html/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
    configure_package_config_file(cmake/iiHtmlBlockConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/iiHtmlBlockConfig.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiHtmlBlock)
    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/iiHtmlBlockConfigVersion.cmake VERSION 0.1.1 COMPATIBILITY SameMajorVersion)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/iiHtmlBlockConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/iiHtmlBlockConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiHtmlBlock)
endif()

install(TARGETS iiXml
    EXPORT iiXmlTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES src/iiXml "${CMAKE_CURRENT_BINARY_DIR}/install-compat/roots/iiXml.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iiXmlDetail
    FILES_MATCHING PATTERN "*.h" PATTERN "Html" EXCLUDE
)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/install-compat/xml/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
install(FILES README.md AGENT.md AGENTS.md LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/iiXml)
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DATADIR}/iiXml/docs
    FILES_MATCHING PATTERN "*.md" PATTERN "*.json" PATTERN "*.iisacc.json" EXCLUDE
    PATTERN ".*.iisacc-preview" EXCLUDE PATTERN ".iisacc-temporary.lock" EXCLUDE)

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
if(BUILD_TESTING)
    add_executable(iiXmlMarkupQtCoexistence Test/markup_qt_coexistence_test.cpp)
    target_link_libraries(iiXmlMarkupQtCoexistence PRIVATE iiXml iiXml::Markup)
    add_test(NAME iiXml.Markup.QtCoexistence COMMAND iiXmlMarkupQtCoexistence)
endif()

if(IIXML_BUILD_HTML_BLOCK)
    add_executable(HtmlBlockMerge_test tests/HtmlBlockMerge.cpp)
    target_link_libraries(HtmlBlockMerge_test PRIVATE iiXml::HtmlBlock)
    add_test(NAME iiXml.HtmlBlockMerge COMMAND HtmlBlockMerge_test)
    foreach(_case GetTagInfo GetHTML iiXmlToHTML DivideBlock BlockRangeTracker
            BlockHTMLSerializer DeleteBlock CombineBlock FlattenBlock umbrella_header)
        add_executable(iiXmlHtml_${_case}_test tests/html/${_case}_test.cpp)
        target_link_libraries(iiXmlHtml_${_case}_test PRIVATE iiXml::HtmlBlock)
        if(MSVC)
            target_compile_options(iiXmlHtml_${_case}_test PRIVATE /UNDEBUG)
        else()
            target_compile_options(iiXmlHtml_${_case}_test PRIVATE -UNDEBUG)
        endif()
        add_test(NAME iiXml.HtmlBlock.${_case} COMMAND iiXmlHtml_${_case}_test)
    endforeach()
    add_executable(iiXmlHtmlExample example/HtmlBlock/ParsedXmlHtmlExample.cpp)
    target_link_libraries(iiXmlHtmlExample PRIVATE iiXml::HtmlBlock)
    if(MSVC)
        target_compile_options(iiXmlHtmlExample PRIVATE /UNDEBUG)
    else()
        target_compile_options(iiXmlHtmlExample PRIVATE -UNDEBUG)
    endif()
    add_test(NAME iiXml.HtmlBlock.example COMMAND iiXmlHtmlExample)
    add_test(NAME iiXml.HtmlBlock.installed_consumer
        COMMAND ${CMAKE_COMMAND} "-DIIXML_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}"
            "-DIIXML_BINARY=${CMAKE_CURRENT_BINARY_DIR}" "-DQT6_DIR=${Qt6_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/html/installed_consumer.cmake")
    set_tests_properties(iiXml.HtmlBlock.installed_consumer PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE)
endif()

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

add_test(NAME iiXml.sdk_boundary COMMAND ${CMAKE_COMMAND}
    "-DIIXML_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}" -P "${CMAKE_CURRENT_SOURCE_DIR}/Test/sdk_boundary_test.cmake")

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
