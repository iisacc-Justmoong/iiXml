#include "MatrixC.h"
#include "Matrix.h"
#include "MatrixXml.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using ii::xml::Matrix;
using ii::xml::MatrixResult;
using ii::xml::Number;
thread_local std::string error_message;

template<class Fn> int guarded(Fn&& action) noexcept {
    try {
        error_message.clear();
        action();
        return 0;
    } catch (const std::exception& error) {
        error_message = error.what();
    } catch (...) {
        error_message = "Unknown matrix error";
    }
    return 1;
}

template<class T> T unwrap(MatrixResult<T> result) {
    if (!result) throw std::runtime_error(result.error().message);
    return std::move(*result);
}

const Matrix& get(const void* handle) {
    if (!handle) throw std::invalid_argument("Matrix handle is null");
    return *static_cast<const Matrix*>(handle);
}

std::vector<size_t> dimensions(const size_t* shape, size_t rank) {
    if (rank && !shape) throw std::invalid_argument("Matrix shape is null");
    return rank ? std::vector<size_t>(shape, shape + rank) : std::vector<size_t>{};
}

iiXmlMatrixNumber to_c(const Number& number) {
    if (const auto* integer = std::get_if<std::int64_t>(&number)) return {0, *integer, 0};
    return {1, 0, std::get<double>(number)};
}

void save(MatrixResult<Matrix> result, void** output) {
    if (!output) throw std::invalid_argument("Output handle is null");
    *output = new Matrix(unwrap(std::move(result)));
}
} // namespace

extern "C" {
const char* iixml_matrix_error(void) { return error_message.c_str(); }

int iixml_matrix_create(const size_t* shape, size_t rank, const iiXmlMatrixNumber* numbers,
                        size_t count, void** output) {
    return guarded([&] {
        if (!output || (count && !numbers)) throw std::invalid_argument("Matrix output or values are null");
        *output = nullptr;
        std::vector<Number> values;
        values.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            if (numbers[i].kind == 0) values.emplace_back(numbers[i].integer);
            else if (numbers[i].kind == 1) values.emplace_back(numbers[i].real);
            else throw std::invalid_argument("Unknown matrix number type");
        }
        save(Matrix::create(dimensions(shape, rank), std::move(values)), output);
    });
}
void iixml_matrix_destroy(void* handle) { delete static_cast<Matrix*>(handle); }
size_t iixml_matrix_rank(const void* handle) { return handle ? static_cast<const Matrix*>(handle)->shape().size() : 0; }
size_t iixml_matrix_size(const void* handle) { return handle ? static_cast<const Matrix*>(handle)->values().size() : 0; }
int iixml_matrix_dimension(const void* handle, size_t index, size_t* output) {
    return guarded([&] {
        if (!output) throw std::invalid_argument("Dimension output is null");
        const auto& shape = get(handle).shape();
        if (index >= shape.size()) throw std::out_of_range("Dimension index is out of bounds");
        *output = shape[index];
    });
}
int iixml_matrix_value(const void* handle, size_t index, iiXmlMatrixNumber* output) {
    return guarded([&] {
        if (!output) throw std::invalid_argument("Value output is null");
        const auto& values = get(handle).values();
        if (index >= values.size()) throw std::out_of_range("Value index is out of bounds");
        *output = to_c(values[index]);
    });
}
int iixml_matrix_at(const void* handle, const size_t* index, size_t rank, iiXmlMatrixNumber* output) {
    return guarded([&] {
        if (!output || (rank && !index)) throw std::invalid_argument("Matrix index or output is null");
        *output = to_c(unwrap(get(handle).at(std::span<const size_t>(index, rank))));
    });
}
int iixml_matrix_reshape(const void* handle, const size_t* shape, size_t rank, void** output) {
    return guarded([&] {
        if (!output) throw std::invalid_argument("Output handle is null");
        *output = nullptr;
        save(get(handle).reshape(dimensions(shape, rank)), output);
    });
}
int iixml_matrix_binary(const void* left, const void* right, int operation, void** output) {
    return guarded([&] {
        if (!output) throw std::invalid_argument("Output handle is null");
        *output = nullptr;
        const auto& a = get(left);
        const auto& b = get(right);
        switch (operation) {
        case 0: save(a.add(b), output); break;
        case 1: save(a.subtract(b), output); break;
        case 2: save(a.multiply(b), output); break;
        case 3: save(a.divide(b), output); break;
        case 4: save(a.matmul(b), output); break;
        default: throw std::invalid_argument("Unknown matrix operation");
        }
    });
}
int iixml_matrix_reduce(const void* handle, int operation, iiXmlMatrixNumber* output) {
    return guarded([&] {
        if (!output) throw std::invalid_argument("Reduction output is null");
        if (operation == 0) *output = to_c(unwrap(get(handle).sum()));
        else if (operation == 1) *output = to_c(unwrap(get(handle).mean()));
        else throw std::invalid_argument("Unknown matrix reduction");
    });
}
int iixml_matrix_reduce_axes(const void* handle, int operation, const ptrdiff_t* axes,
                             size_t count, int keep_dimensions, void** output) {
    return guarded([&] {
        if (!output || (count && !axes)) throw std::invalid_argument("Reduction axes or output are null");
        *output = nullptr;
        const auto selected = std::span<const ptrdiff_t>(axes, count);
        if (operation == 0) save(get(handle).sum(selected, keep_dimensions != 0), output);
        else if (operation == 1) save(get(handle).mean(selected, keep_dimensions != 0), output);
        else throw std::invalid_argument("Unknown matrix reduction");
    });
}
int iixml_matrix_tensordot(const void* left, const void* right, const ptrdiff_t* left_axes,
                           size_t left_count, const ptrdiff_t* right_axes, size_t right_count, void** output) {
    return guarded([&] {
        if (!output || (left_count && !left_axes) || (right_count && !right_axes))
            throw std::invalid_argument("Contraction axes or output are null");
        *output = nullptr;
        save(get(left).tensordot(get(right), std::span<const ptrdiff_t>(left_axes, left_count),
                                std::span<const ptrdiff_t>(right_axes, right_count)), output);
    });
}
int iixml_matrix_from_xml(const char* input, size_t size, void** output) {
    return guarded([&] {
        if (!output || (size && !input)) throw std::invalid_argument("XML input or output is null");
        *output = nullptr;
        save(ii::xml::matrix_from_xml(size ? std::string(input, size) : std::string{}), output);
    });
}
int iixml_matrix_to_xml(const void* handle, char** output, size_t* size) {
    return guarded([&] {
        if (!output || !size) throw std::invalid_argument("XML output is null");
        *output = nullptr;
        *size = 0;
        const auto text = unwrap(ii::xml::matrix_to_xml(get(handle)));
        auto* copy = static_cast<char*>(std::malloc(text.size() + 1));
        if (!copy) throw std::bad_alloc();
        std::memcpy(copy, text.c_str(), text.size() + 1);
        *output = copy;
        *size = text.size();
    });
}
void iixml_matrix_free_string(char* text) { std::free(text); }
} // extern "C"
