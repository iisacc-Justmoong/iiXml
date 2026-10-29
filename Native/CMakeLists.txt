include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
find_package(LibXml2 2.9 REQUIRED)
# Source-preserving markup is independent of Qt, libxml and document modules.
file(GLOB _markup_public_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/Markup/*.h")
foreach(_header IN LISTS _markup_public_headers)
    get_filename_component(_name "${_header}" NAME)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/iiXml/Markup/${_name}"
        CONTENT "#pragma once\n#include \"${_header}\"\n")
endforeach()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/iiXml/Xml.h"
    CONTENT "#pragma once\n#include \"${CMAKE_CURRENT_SOURCE_DIR}/src/Xml.h\"\n")
add_library(iiXmlMarkup STATIC
    src/Markup/Diagnostics.cpp src/Markup/InlineProperties.cpp
    src/Markup/OpenTag.cpp src/Markup/Doctype.cpp
    src/Markup/TagParser.cpp src/Markup/GetTagInfo.cpp
    src/Markup/DivideBlock.cpp src/Markup/BlockRangeTracker.cpp)
add_library(iiXml::Markup ALIAS iiXmlMarkup)
set_target_properties(iiXmlMarkup PROPERTIES EXPORT_NAME Markup CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
target_compile_features(iiXmlMarkup PUBLIC cxx_std_23)
target_include_directories(iiXmlMarkup PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative>)
add_library(iiXmlNative STATIC src/Xml.cpp)
add_library(iiXml::Native ALIAS iiXmlNative)
set_target_properties(iiXmlNative PROPERTIES EXPORT_NAME Native CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
target_compile_features(iiXmlNative PUBLIC cxx_std_23)
target_include_directories(iiXmlNative PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative>)
target_link_libraries(iiXmlNative PRIVATE LibXml2::LibXml2)
if(BUILD_TESTING)
    add_test(NAME iiXml.Native.SourceLayout COMMAND ${CMAKE_COMMAND}
        "-DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/.." -P "${CMAKE_CURRENT_SOURCE_DIR}/../Test/source_layout.cmake")
    add_executable(iiXmlMarkupContracts ../Test/markup_contract_test.cpp)
    target_link_libraries(iiXmlMarkupContracts PRIVATE iiXml::Markup)
    if(MSVC)
        target_compile_options(iiXmlMarkupContracts PRIVATE /utf-8)
    endif()
    add_test(NAME iiXml.Markup.Contracts COMMAND iiXmlMarkupContracts)
    add_executable(iiXmlNativeContracts ../Test/native_test.cpp)
    target_link_libraries(iiXmlNativeContracts PRIVATE iiXml::Native)
    add_test(NAME iiXml.Native.Contracts COMMAND iiXmlNativeContracts)
endif()
install(TARGETS iiXmlMarkup EXPORT iiXmlMarkupTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY src/Markup/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative/iiXml/Markup FILES_MATCHING PATTERN "*.h")
install(EXPORT iiXmlMarkupTargets NAMESPACE iiXml:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlMarkup)
configure_package_config_file(iiXmlMarkupConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/iiXmlMarkupConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlMarkup)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/iiXmlMarkupConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iiXmlMarkupConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/iiXmlMarkupConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlMarkup)
install(TARGETS iiXmlNative EXPORT iiXmlNativeTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES src/Xml.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative/iiXml)
install(EXPORT iiXmlNativeTargets NAMESPACE iiXml:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlNative)
configure_package_config_file(iiXmlNativeConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/iiXmlNativeConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlNative)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/iiXmlNativeConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iiXmlNativeConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/iiXmlNativeConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlNative)
install(FILES ../docs/native.md ../docs/Markup.md DESTINATION ${CMAKE_INSTALL_DATADIR}/iiXml/docs)
install(FILES ../docs/Markup.agent.json DESTINATION ${CMAKE_INSTALL_DATADIR}/iiXml/docs OPTIONAL)
install(FILES ../LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/iiXml)
