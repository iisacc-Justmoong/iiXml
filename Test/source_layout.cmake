if(NOT SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()
if(IS_DIRECTORY "${SOURCE_ROOT}/Native/src/iiXml")
    message(FATAL_ERROR "The native source tree must not repeat the project name")
endif()
foreach(_file Native/src/Xml.h Native/src/Xml.cpp Native/src/Markup/TagParser.h
        Native/src/Markup/TagParser.cpp Native/src/Markup/BlockRangeTracker.h
        Native/src/Markup/BlockRangeTracker.cpp Native/src/Markup/Diagnostics.h
        Native/src/Markup/Diagnostics.cpp)
    if(NOT EXISTS "${SOURCE_ROOT}/${_file}")
        message(FATAL_ERROR "Missing adjacent native source or header: ${_file}")
    endif()
endforeach()
