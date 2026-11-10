#include "Matrix.h"
#include "MatrixArithmetic.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace ii::xml {
namespace {

auto fail(MatrixErrorCode code, std::string message) {
    return std::unexpected(MatrixError{code, std::move(message)});
}

using detail::Operation;
using detail::apply;
using detail::element_count;

MatrixResult<Matrix> elementwise(const Matrix& lhs, const Matrix& rhs, Operation operation) {
    if (!lhs.valid() || !rhs.valid()) return fail(MatrixErrorCode::invalid_state, "Matrix is in a moved-from state");
    const bool left_scalar = lhs.shape().empty();
    const bool right_scalar = rhs.shape().empty();
    if (!left_scalar && !right_scalar && lhs.shape() != rhs.shape())
        return fail(MatrixErrorCode::shape_mismatch, "Elementwise matrix shapes do not match");
    const auto& shape = left_scalar ? rhs.shape() : lhs.shape();
    const auto count = left_scalar ? rhs.values().size() : lhs.values().size();
    std::vector<Number> values;
    values.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        auto value = apply(lhs.values()[left_scalar ? 0 : i], rhs.values()[right_scalar ? 0 : i], operation);
        if (!value) return std::unexpected(value.error());
        values.push_back(*value);
    }
    return Matrix::create(shape, std::move(values));
}

} // namespace

Matrix::Matrix(std::vector<std::size_t> shape, std::vector<Number> values)
    : shape_(std::move(shape)), values_(std::move(values)) {}

MatrixResult<Matrix> Matrix::create(std::vector<std::size_t> shape, std::vector<Number> values) {
    auto count = element_count(shape);
    if (!count) return std::unexpected(count.error());
    if (*count != values.size()) return fail(MatrixErrorCode::invalid_shape, "Shape does not match the number of values");
    for (const auto& value : values) {
        if (const auto* real = std::get_if<double>(&value); real && !std::isfinite(*real))
            return fail(MatrixErrorCode::invalid_number, "Matrix values must be finite");
    }
    return Matrix(std::move(shape), std::move(values));
}

bool Matrix::valid() const noexcept {
    if (std::ranges::find(shape_, 0) != shape_.end()) return values_.empty();
    std::size_t count = 1;
    for (const auto dimension : shape_) {
        if (dimension > std::numeric_limits<std::size_t>::max() / count) return false;
        count *= dimension;
    }
    return count == values_.size();
}

MatrixResult<Number> Matrix::at(std::span<const std::size_t> index) const {
    if (!valid()) return fail(MatrixErrorCode::invalid_state, "Matrix is in a moved-from state");
    if (index.size() != shape_.size()) return fail(MatrixErrorCode::out_of_bounds, "Index rank does not match matrix rank");
    for (std::size_t i = 0; i < index.size(); ++i) {
        if (index[i] >= shape_[i]) return fail(MatrixErrorCode::out_of_bounds, "Matrix index is out of bounds");
    }
    std::size_t offset = 0;
    for (std::size_t i = 0; i < index.size(); ++i) offset = offset * shape_[i] + index[i];
    return values_[offset];
}

MatrixResult<Matrix> Matrix::reshape(std::vector<std::size_t> shape) const {
    if (!valid()) return fail(MatrixErrorCode::invalid_state, "Matrix is in a moved-from state");
    return create(std::move(shape), values_);
}
MatrixResult<Matrix> Matrix::add(const Matrix& other) const { return elementwise(*this, other, Operation::add); }
MatrixResult<Matrix> Matrix::subtract(const Matrix& other) const { return elementwise(*this, other, Operation::subtract); }
MatrixResult<Matrix> Matrix::multiply(const Matrix& other) const { return elementwise(*this, other, Operation::multiply); }
MatrixResult<Matrix> Matrix::divide(const Matrix& other) const { return elementwise(*this, other, Operation::divide); }

MatrixResult<Number> Matrix::sum() const {
    if (!valid()) return fail(MatrixErrorCode::invalid_state, "Matrix is in a moved-from state");
    Number total{std::int64_t{0}};
    for (const auto& value : values_) {
        auto next = apply(total, value, Operation::add);
        if (!next) return std::unexpected(next.error());
        total = *next;
    }
    return total;
}

MatrixResult<Number> Matrix::mean() const {
    if (!valid()) return fail(MatrixErrorCode::invalid_state, "Matrix is in a moved-from state");
    if (values_.empty()) return fail(MatrixErrorCode::empty, "An empty matrix has no mean");
    if (values_.size() > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        return fail(MatrixErrorCode::overflow, "Matrix element count exceeds integer range");
    auto total = sum();
    if (!total) return std::unexpected(total.error());
    return apply(*total, Number{static_cast<std::int64_t>(values_.size())}, Operation::divide);
}

} // namespace ii::xml
