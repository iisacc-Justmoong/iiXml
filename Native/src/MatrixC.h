#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#  if defined(IIXML_MATRIX_C_BUILD)
#    define IIXML_MATRIX_API __declspec(dllexport)
#  else
#    define IIXML_MATRIX_API __declspec(dllimport)
#  endif
#else
#  define IIXML_MATRIX_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iiXmlMatrixNumber {
    int kind; /* 0: signed integer, 1: double */
    int64_t integer;
    double real;
} iiXmlMatrixNumber;

/* Functions return zero on success. The error text is thread-local. */
IIXML_MATRIX_API const char* iixml_matrix_error(void);
IIXML_MATRIX_API int iixml_matrix_create(const size_t*, size_t, const iiXmlMatrixNumber*, size_t, void**);
IIXML_MATRIX_API void iixml_matrix_destroy(void*);
IIXML_MATRIX_API size_t iixml_matrix_rank(const void*);
IIXML_MATRIX_API size_t iixml_matrix_size(const void*);
IIXML_MATRIX_API int iixml_matrix_dimension(const void*, size_t, size_t*);
IIXML_MATRIX_API int iixml_matrix_value(const void*, size_t, iiXmlMatrixNumber*);
IIXML_MATRIX_API int iixml_matrix_at(const void*, const size_t*, size_t, iiXmlMatrixNumber*);
IIXML_MATRIX_API int iixml_matrix_reshape(const void*, const size_t*, size_t, void**);
/* Operations: 0 add, 1 subtract, 2 elementwise multiply, 3 divide, 4 matmul. */
IIXML_MATRIX_API int iixml_matrix_binary(const void*, const void*, int, void**);
/* Reductions: 0 sum, 1 mean. */
IIXML_MATRIX_API int iixml_matrix_reduce(const void*, int, iiXmlMatrixNumber*);
/* Negative axes count from the end; zero axes give identity/outer product. */
IIXML_MATRIX_API int iixml_matrix_reduce_axes(const void*, int, const ptrdiff_t*, size_t, int, void**);
IIXML_MATRIX_API int iixml_matrix_tensordot(const void*, const void*,
    const ptrdiff_t*, size_t, const ptrdiff_t*, size_t, void**);
IIXML_MATRIX_API int iixml_matrix_from_xml(const char*, size_t, void**);
IIXML_MATRIX_API int iixml_matrix_to_xml(const void*, char**, size_t*);
IIXML_MATRIX_API void iixml_matrix_free_string(char*);

#ifdef __cplusplus
}
#endif
