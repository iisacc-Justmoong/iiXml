if(NOT DEFINED IIXML_SOURCE)
    message(FATAL_ERROR "IIXML_SOURCE is required")
endif()
foreach(_path IN ITEMS CMakeLists.txt cmake/iiXmlConfig.cmake.in
                       src/Input/GetFile.cpp src/Parser/FileParser.cpp)
    file(READ "${IIXML_SOURCE}/${_path}" _contents)
    if(_contents MATCHES "(find_package|find_dependency|target_link_libraries)\\([^\\)]*iiFileProvider"
            OR _contents MATCHES "#include[ \t]*[<\"]iiFileProvider"
            OR _contents MATCHES "iiFileProvider::")
        message(FATAL_ERROR "SDK dependency remains in ${_path}")
    endif()
endforeach()
