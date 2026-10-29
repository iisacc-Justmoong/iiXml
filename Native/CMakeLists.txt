include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
find_package(LibXml2 2.9 REQUIRED)
# Source-preserving markup is independent of Qt, libxml and document modules.
add_library(iiXmlMarkup STATIC
    src/iiXml/Markup/Diagnostics.cpp src/iiXml/Markup/InlineProperties.cpp
    src/iiXml/Markup/OpenTag.cpp src/iiXml/Markup/Doctype.cpp
    src/iiXml/Markup/TagParser.cpp src/iiXml/Markup/GetTagInfo.cpp
    src/iiXml/Markup/DivideBlock.cpp src/iiXml/Markup/BlockRangeTracker.cpp)
add_library(iiXml::Markup ALIAS iiXmlMarkup)
set_target_properties(iiXmlMarkup PROPERTIES EXPORT_NAME Markup CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
target_compile_features(iiXmlMarkup PUBLIC cxx_std_23)
target_include_directories(iiXmlMarkup PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative>)
add_library(iiXmlNative STATIC src/iiXml/Xml.cpp)
add_library(iiXml::Native ALIAS iiXmlNative)
set_target_properties(iiXmlNative PROPERTIES EXPORT_NAME Native CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
target_compile_features(iiXmlNative PUBLIC cxx_std_23)
target_include_directories(iiXmlNative PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative>)
target_link_libraries(iiXmlNative PRIVATE LibXml2::LibXml2)
if(BUILD_TESTING)
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
install(DIRECTORY src/iiXml/Markup/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative/iiXml/Markup FILES_MATCHING PATTERN "*.h")
install(EXPORT iiXmlMarkupTargets NAMESPACE iiXml:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlMarkup)
configure_package_config_file(iiXmlMarkupConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/iiXmlMarkupConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlMarkup)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/iiXmlMarkupConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iiXmlMarkupConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/iiXmlMarkupConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlMarkup)
install(TARGETS iiXmlNative EXPORT iiXmlNativeTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES src/iiXml/Xml.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative/iiXml)
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
