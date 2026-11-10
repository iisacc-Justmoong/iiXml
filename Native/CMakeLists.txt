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
foreach(_native_header Xml.h Matrix.h MatrixXml.h MatrixC.h)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/iiXml/${_native_header}"
        CONTENT "#pragma once\n#include \"${CMAKE_CURRENT_SOURCE_DIR}/src/${_native_header}\"\n")
endforeach()
add_library(iiXmlMarkup
    src/Markup/Diagnostics.cpp src/Markup/InlineProperties.cpp
    src/Markup/OpenTag.cpp src/Markup/Doctype.cpp
    src/Markup/TagParser.cpp src/Markup/GetTagInfo.cpp
    src/Markup/DivideBlock.cpp src/Markup/BlockRangeTracker.cpp)
add_library(iiXml::Markup ALIAS iiXmlMarkup)
set_target_properties(iiXmlMarkup PROPERTIES EXPORT_NAME Markup CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON WINDOWS_EXPORT_ALL_SYMBOLS ON)
target_compile_features(iiXmlMarkup PUBLIC cxx_std_23)
target_include_directories(iiXmlMarkup PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative>)
add_library(iiXmlNative src/Xml.cpp src/Matrix.cpp src/MatrixXml.cpp
    src/MatrixArithmetic.cpp src/MatrixAlgebra.cpp)
add_library(iiXml::Native ALIAS iiXmlNative)
set_target_properties(iiXmlNative PROPERTIES EXPORT_NAME Native CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON WINDOWS_EXPORT_ALL_SYMBOLS ON)
target_compile_features(iiXmlNative PUBLIC cxx_std_23)
target_include_directories(iiXmlNative PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative>)
target_link_libraries(iiXmlNative PRIVATE LibXml2::LibXml2)
foreach(_target IN ITEMS iiXmlMarkup iiXmlNative)
    if(BUILD_SHARED_LIBS)
        if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
            set_target_properties(${_target} PROPERTIES FRAMEWORK TRUE FRAMEWORK_VERSION A
                MACOSX_FRAMEWORK_IDENTIFIER "com.iisacc.${_target}")
        elseif(APPLE)
            set_property(TARGET ${_target} APPEND PROPERTY INSTALL_RPATH "@loader_path")
        elseif(UNIX AND NOT EMSCRIPTEN)
            set_property(TARGET ${_target} APPEND PROPERTY INSTALL_RPATH "$ORIGIN")
        endif()
        if(EMSCRIPTEN)
            target_compile_options(${_target} PRIVATE -fPIC -sSIDE_MODULE=1)
            target_link_options(${_target} PRIVATE -sSIDE_MODULE=1)
        endif()
    endif()
endforeach()
set(_matrix_python_default ON)
if(EMSCRIPTEN OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(_matrix_python_default OFF)
endif()
option(IIXML_BUILD_MATRIX_PYTHON "Build the matrix C bridge and Python module" ${_matrix_python_default})
if(IIXML_BUILD_MATRIX_PYTHON)
    add_library(iiXmlMatrixC SHARED src/MatrixC.cpp src/MatrixC.h)
    add_library(iiXml::MatrixC ALIAS iiXmlMatrixC)
    set_target_properties(iiXmlMatrixC PROPERTIES EXPORT_NAME MatrixC)
    target_compile_definitions(iiXmlMatrixC PRIVATE IIXML_MATRIX_C_BUILD)
    target_include_directories(iiXmlMatrixC PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative>)
    target_link_libraries(iiXmlMatrixC PRIVATE iiXml::Native)
    if(APPLE)
        set_property(TARGET iiXmlMatrixC APPEND PROPERTY INSTALL_RPATH "@loader_path")
    elseif(UNIX)
        set_property(TARGET iiXmlMatrixC APPEND PROPERTY INSTALL_RPATH "$ORIGIN")
    endif()
endif()
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
    if(BUILD_SHARED_LIBS AND NOT CMAKE_CROSSCOMPILING)
        foreach(_component IN ITEMS Markup Native)
            add_test(NAME "iiXml.shared_type.${_component}" COMMAND "${CMAKE_COMMAND}"
                "-DTARGET_TYPE=$<TARGET_PROPERTY:iiXml${_component},TYPE>"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/../Test/shared_target_type.cmake")
            add_test(NAME "iiXml.shared_link.${_component}" COMMAND "${CMAKE_COMMAND}"
                "-DIMAGE=$<TARGET_FILE:iiXml${_component}Contracts>"
                "-DEXPECTED=$<TARGET_FILE:iiXml${_component}>"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/../Test/shared_link.cmake")
        endforeach()
    endif()
    add_executable(iiXmlMatrixContracts ../Test/matrix_test.cpp)
    target_link_libraries(iiXmlMatrixContracts PRIVATE iiXml::Native)
    add_test(NAME iiXml.Native.Matrix COMMAND iiXmlMatrixContracts)
    if(IIXML_BUILD_MATRIX_PYTHON)
        add_executable(iiXmlMatrixCContracts ../Test/matrix_c_test.cpp)
        target_link_libraries(iiXmlMatrixCContracts PRIVATE iiXml::MatrixC)
        add_test(NAME iiXml.Native.MatrixC COMMAND iiXmlMatrixCContracts)
    endif()
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(IIXML_BUILD_MATRIX_PYTHON AND Python3_Interpreter_FOUND)
        add_test(NAME iiXml.Native.MatrixPython COMMAND ${CMAKE_COMMAND} -E env
            "IIXML_MATRIX_LIBRARY=$<TARGET_FILE:iiXmlMatrixC>"
            "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/src/Python"
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/../Test/matrix_python_test.py")
        if(NOT IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}" AND NOT IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}"
                AND NOT IS_ABSOLUTE "${CMAKE_INSTALL_DATADIR}" AND NOT IS_ABSOLUTE "${CMAKE_INSTALL_BINDIR}")
            add_test(NAME iiXml.Native.MatrixInstalled COMMAND ${CMAKE_COMMAND}
                "-DIIXML_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/.."
                "-DIIXML_BINARY=${CMAKE_BINARY_DIR}"
                "-DIIXML_PYTHON=${Python3_EXECUTABLE}"
                "-DIIXML_LIBDIR=${CMAKE_INSTALL_LIBDIR}"
                "-DIIXML_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}"
                "-DIIXML_DATADIR=${CMAKE_INSTALL_DATADIR}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/../Test/matrix_install_test.cmake")
            set_tests_properties(iiXml.Native.MatrixInstalled PROPERTIES RUN_SERIAL TRUE)
        endif()
    endif()
endif()
install(TARGETS iiXmlMarkup EXPORT iiXmlMarkupTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY src/Markup/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative/iiXml/Markup FILES_MATCHING PATTERN "*.h")
install(EXPORT iiXmlMarkupTargets NAMESPACE iiXml:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlMarkup)
configure_package_config_file(iiXmlMarkupConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/iiXmlMarkupConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlMarkup)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/iiXmlMarkupConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iiXmlMarkupConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/iiXmlMarkupConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlMarkup)
install(TARGETS iiXmlNative EXPORT iiXmlNativeTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES src/Xml.h src/Matrix.h src/MatrixXml.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative/iiXml)
if(IIXML_BUILD_MATRIX_PYTHON)
    install(TARGETS iiXmlMatrixC EXPORT iiXmlNativeTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES src/MatrixC.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iiXmlNative/iiXml)
    install(FILES src/Python/iixml_matrix.py DESTINATION ${CMAKE_INSTALL_LIBDIR}/iiXmlNative/python)
endif()
install(EXPORT iiXmlNativeTargets NAMESPACE iiXml:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlNative)
set(IIXML_MATRIX_PYTHON_DIR "${CMAKE_INSTALL_LIBDIR}/iiXmlNative/python")
configure_package_config_file(iiXmlNativeConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/iiXmlNativeConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlNative PATH_VARS IIXML_MATRIX_PYTHON_DIR)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/iiXmlNativeConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/iiXmlNativeConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/iiXmlNativeConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iiXmlNative)
install(FILES ../docs/native.md ../docs/Markup.md ../docs/Matrix.md DESTINATION ${CMAKE_INSTALL_DATADIR}/iiXml/docs)
install(FILES ../docs/Markup.agent.json DESTINATION ${CMAKE_INSTALL_DATADIR}/iiXml/docs OPTIONAL)
install(FILES ../docs/Matrix.agent.json DESTINATION ${CMAKE_INSTALL_DATADIR}/iiXml/docs OPTIONAL)
install(FILES ../LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/iiXml)
