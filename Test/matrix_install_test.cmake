if(NOT DEFINED IIXML_BINARY OR NOT DEFINED IIXML_SOURCE OR NOT DEFINED IIXML_PYTHON OR NOT DEFINED IIXML_LIBDIR
        OR NOT DEFINED IIXML_INCLUDEDIR OR NOT DEFINED IIXML_DATADIR)
    message(FATAL_ERROR "Missing matrix installation test inputs")
endif()

string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef _id)
set(_prefix "${IIXML_BINARY}/matrix-install-${_id}")
while(EXISTS "${_prefix}")
    string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef _id)
    set(_prefix "${IIXML_BINARY}/matrix-install-${_id}")
endwhile()
execute_process(COMMAND "${CMAKE_COMMAND}" --install "${IIXML_BINARY}" --prefix "${_prefix}"
    RESULT_VARIABLE _install_result OUTPUT_VARIABLE _install_output ERROR_VARIABLE _install_error)
if(NOT _install_result EQUAL 0)
    file(REMOVE_RECURSE "${_prefix}")
    message(FATAL_ERROR "Matrix package installation failed: ${_install_output}\n${_install_error}")
endif()

foreach(_required IN ITEMS
        "${IIXML_INCLUDEDIR}/iiXmlNative/iiXml/Matrix.h"
        "${IIXML_INCLUDEDIR}/iiXmlNative/iiXml/MatrixXml.h"
        "${IIXML_INCLUDEDIR}/iiXmlNative/iiXml/MatrixC.h"
        "${IIXML_LIBDIR}/iiXmlNative/python/iixml_matrix.py"
        "${IIXML_DATADIR}/iiXml/docs/Matrix.md"
        "${IIXML_DATADIR}/iiXml/docs/Matrix.agent.json")
    if(NOT EXISTS "${_prefix}/${_required}")
        file(REMOVE_RECURSE "${_prefix}")
        message(FATAL_ERROR "Installed matrix package is missing ${_required}")
    endif()
endforeach()

file(READ "${_prefix}/${IIXML_LIBDIR}/cmake/iiXmlNative/iiXmlNativeTargets.cmake" _targets)
if(NOT _targets MATCHES "iiXml::MatrixC")
    file(REMOVE_RECURSE "${_prefix}")
    message(FATAL_ERROR "Installed C interface is not exported as iiXml::MatrixC")
endif()
file(READ "${_prefix}/${IIXML_LIBDIR}/cmake/iiXmlNative/iiXmlNativeConfig.cmake" _config)
if(NOT _config MATCHES "iiXmlNative_MATRIX_PYTHON_SOURCE")
    file(REMOVE_RECURSE "${_prefix}")
    message(FATAL_ERROR "Installed iiXml package does not expose its CPython AOT source")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" -E env --unset=IIXML_MATRIX_LIBRARY
    --unset=DYLD_LIBRARY_PATH --unset=DYLD_FALLBACK_LIBRARY_PATH
    "PYTHONPATH=${_prefix}/${IIXML_LIBDIR}/iiXmlNative/python"
    "${IIXML_PYTHON}" -B "${IIXML_SOURCE}/Test/matrix_python_test.py"
    RESULT_VARIABLE _test_result OUTPUT_VARIABLE _test_output ERROR_VARIABLE _test_error)
file(REMOVE_RECURSE "${_prefix}")
if(NOT _test_result EQUAL 0)
    message(FATAL_ERROR "Installed matrix Python test failed: ${_test_output}\n${_test_error}")
endif()
