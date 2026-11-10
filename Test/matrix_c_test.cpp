#include <iiXml/MatrixC.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x "\n"; std::exit(1); } } while (false)

int main() {
    const size_t shape[] = {2};
    const iiXmlMatrixNumber values[] = {{0, 3, 0.0}, {1, 0, 2.5}};
    void* matrix = nullptr;
    CHECK(iixml_matrix_create(shape, 1, values, 2, &matrix) == 0 && matrix);
    CHECK(iixml_matrix_rank(matrix) == 1 && iixml_matrix_size(matrix) == 2);
    iiXmlMatrixNumber total{};
    CHECK(iixml_matrix_reduce(matrix, 0, &total) == 0 && total.kind == 1 && total.real == 5.5);
    char* xml = nullptr;
    size_t length = 0;
    CHECK(iixml_matrix_to_xml(matrix, &xml, &length) == 0 && xml && length == std::strlen(xml));
    void* restored = nullptr;
    CHECK(iixml_matrix_from_xml(xml, length, &restored) == 0 && restored);
    iixml_matrix_free_string(xml);
    CHECK(iixml_matrix_value(restored, 1, &total) == 0 && total.kind == 1 && total.real == 2.5);
    CHECK(iixml_matrix_value(restored, 2, &total) != 0 && std::strlen(iixml_matrix_error()) != 0);
    const ptrdiff_t axis[] = {-1};
    void* reduced = nullptr;
    CHECK(iixml_matrix_reduce_axes(matrix, 0, axis, 1, 1, &reduced) == 0 && reduced);
    CHECK(iixml_matrix_rank(reduced) == 1 && iixml_matrix_value(reduced, 0, &total) == 0 && total.real == 5.5);
    void* dot = nullptr;
    CHECK(iixml_matrix_tensordot(matrix, matrix, axis, 1, axis, 1, &dot) == 0 && dot);
    CHECK(iixml_matrix_rank(dot) == 0 && iixml_matrix_value(dot, 0, &total) == 0 && total.real == 15.25);
    void* identity = nullptr;
    CHECK(iixml_matrix_reduce_axes(matrix, 1, nullptr, 0, 0, &identity) == 0 && identity);
    CHECK(iixml_matrix_rank(identity) == 1 && iixml_matrix_size(identity) == 2);
    void* outer = nullptr;
    CHECK(iixml_matrix_tensordot(matrix, matrix, nullptr, 0, nullptr, 0, &outer) == 0 && outer);
    CHECK(iixml_matrix_rank(outer) == 2 && iixml_matrix_size(outer) == 4);
    iixml_matrix_destroy(outer);
    iixml_matrix_destroy(identity);
    iixml_matrix_destroy(dot);
    iixml_matrix_destroy(reduced);
    iixml_matrix_destroy(restored);
    iixml_matrix_destroy(matrix);
}
