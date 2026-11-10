#include "MatrixAlgebra.h"
#include "MatrixArithmetic.h"

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

namespace ii::xml {
namespace {

auto fail(MatrixErrorCode code, std::string message) {
    return std::unexpected(MatrixError{code, std::move(message)});
}

MatrixResult<std::vector<std::size_t>> normalize_axes(std::span<const std::ptrdiff_t> axes,
                                                       std::size_t rank) {
    if (rank > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()))
        return fail(MatrixErrorCode::overflow, "Matrix rank exceeds axis range");
    const auto signed_rank = static_cast<std::ptrdiff_t>(rank);
    std::vector<bool> used(rank, false);
    std::vector<std::size_t> result;
    result.reserve(axes.size());
    for (auto axis : axes) {
        if (axis < 0) {
            if (axis < -signed_rank) return fail(MatrixErrorCode::out_of_bounds, "Matrix axis is out of bounds");
            axis += signed_rank;
        }
        if (axis < 0 || axis >= signed_rank)
            return fail(MatrixErrorCode::out_of_bounds, "Matrix axis is out of bounds");
        const auto index = static_cast<std::size_t>(axis);
        if (used[index]) return fail(MatrixErrorCode::invalid_shape, "Matrix axes must not repeat");
        used[index] = true;
        result.push_back(index);
    }
    return result;
}

std::vector<std::size_t> coordinates(std::size_t flat, std::span<const std::size_t> shape) {
    std::vector<std::size_t> result(shape.size());
    for (std::size_t i = shape.size(); i-- > 0;) {
        result[i] = flat % shape[i];
        flat /= shape[i];
    }
    return result;
}

std::size_t offset(std::span<const std::size_t> index, std::span<const std::size_t> shape) {
    std::size_t result = 0;
    for (std::size_t i = 0; i < shape.size(); ++i) result = result * shape[i] + index[i];
    return result;
}

MatrixResult<Number> accumulate_product(Number total, const Number& left, const Number& right) {
    auto product = detail::apply(left, right, detail::Operation::multiply);
    if (!product) return std::unexpected(product.error());
    return detail::apply(total, *product, detail::Operation::add);
}

} // namespace

MatrixResult<Matrix> Matrix::matmul(const Matrix& other) const {
    if (!valid() || !other.valid()) return fail(MatrixErrorCode::invalid_state, "Matrix is in a moved-from state");
    const auto left_rank = shape_.size();
    const auto right_rank = other.shape_.size();
    if (left_rank == 0 || right_rank == 0)
        return fail(MatrixErrorCode::shape_mismatch, "Matrix multiplication does not accept scalars");
    const bool left_vector = left_rank == 1;
    const bool right_vector = right_rank == 1;
    const std::size_t rows = left_vector ? 1 : shape_[left_rank - 2];
    const std::size_t inner = shape_[left_rank - 1];
    const std::size_t right_inner = right_vector ? other.shape_[0] : other.shape_[right_rank - 2];
    const std::size_t columns = right_vector ? 1 : other.shape_[right_rank - 1];
    if (inner != right_inner) return fail(MatrixErrorCode::shape_mismatch, "Matrix inner dimensions do not match");

    const auto left_batch_rank = left_vector ? 0 : left_rank - 2;
    const auto right_batch_rank = right_vector ? 0 : right_rank - 2;
    const auto batch_rank = std::max(left_batch_rank, right_batch_rank);
    std::vector<std::size_t> batch_shape(batch_rank);
    for (std::size_t i = 0; i < batch_rank; ++i) {
        const auto left = i < batch_rank - left_batch_rank ? 1 : shape_[i - (batch_rank - left_batch_rank)];
        const auto right = i < batch_rank - right_batch_rank ? 1 : other.shape_[i - (batch_rank - right_batch_rank)];
        if (left != right && left != 1 && right != 1)
            return fail(MatrixErrorCode::shape_mismatch, "Matrix batch dimensions cannot be broadcast");
        batch_shape[i] = left == 1 ? right : left;
    }
    auto output_shape = batch_shape;
    if (!left_vector) output_shape.push_back(rows);
    if (!right_vector) output_shape.push_back(columns);
    auto output_count = detail::element_count(output_shape);
    if (!output_count) return std::unexpected(output_count.error());
    if (*output_count == 0) return create(std::move(output_shape), {});
    if (inner == 0)
        return create(std::move(output_shape), std::vector<Number>(*output_count, Number{std::int64_t{0}}));
    auto batch_count = detail::element_count(batch_shape);
    if (!batch_count) return std::unexpected(batch_count.error());

    std::vector<Number> output;
    output.reserve(*output_count);
    for (std::size_t batch = 0; batch < *batch_count; ++batch) {
        const auto index = coordinates(batch, batch_shape);
        std::size_t left_batch = 0;
        for (std::size_t i = 0; i < left_batch_rank; ++i)
            left_batch = left_batch * shape_[i] + (shape_[i] == 1 ? 0 : index[batch_rank - left_batch_rank + i]);
        std::size_t right_batch = 0;
        for (std::size_t i = 0; i < right_batch_rank; ++i)
            right_batch = right_batch * other.shape_[i] +
                (other.shape_[i] == 1 ? 0 : index[batch_rank - right_batch_rank + i]);
        for (std::size_t row = 0; row < rows; ++row) {
            for (std::size_t column = 0; column < columns; ++column) {
                Number total{std::int64_t{0}};
                for (std::size_t i = 0; i < inner; ++i) {
                    const auto left = (left_batch * rows + row) * inner + i;
                    const auto right = (right_batch * inner + i) * columns + column;
                    auto next = accumulate_product(total, values_[left], other.values_[right]);
                    if (!next) return std::unexpected(next.error());
                    total = *next;
                }
                output.push_back(total);
            }
        }
    }
    return create(std::move(output_shape), std::move(output));
}

MatrixResult<Matrix> Matrix::tensordot(const Matrix& other, std::span<const std::ptrdiff_t> left_axes,
                                        std::span<const std::ptrdiff_t> right_axes) const {
    if (!valid() || !other.valid()) return fail(MatrixErrorCode::invalid_state, "Matrix is in a moved-from state");
    if (left_axes.size() != right_axes.size())
        return fail(MatrixErrorCode::shape_mismatch, "Tensor contraction axis counts do not match");
    auto left = normalize_axes(left_axes, shape_.size());
    if (!left) return std::unexpected(left.error());
    auto right = normalize_axes(right_axes, other.shape_.size());
    if (!right) return std::unexpected(right.error());
    std::vector<bool> left_used(shape_.size(), false), right_used(other.shape_.size(), false);
    std::vector<std::size_t> contracted_shape;
    for (std::size_t i = 0; i < left->size(); ++i) {
        if (shape_[(*left)[i]] != other.shape_[(*right)[i]])
            return fail(MatrixErrorCode::shape_mismatch, "Tensor contraction dimensions do not match");
        left_used[(*left)[i]] = true;
        right_used[(*right)[i]] = true;
        contracted_shape.push_back(shape_[(*left)[i]]);
    }
    std::vector<std::size_t> output_shape;
    for (std::size_t i = 0; i < shape_.size(); ++i) if (!left_used[i]) output_shape.push_back(shape_[i]);
    for (std::size_t i = 0; i < other.shape_.size(); ++i) if (!right_used[i]) output_shape.push_back(other.shape_[i]);
    auto output_count = detail::element_count(output_shape);
    if (!output_count) return std::unexpected(output_count.error());
    if (*output_count == 0) return create(std::move(output_shape), {});
    auto contracted_count = detail::element_count(contracted_shape);
    if (!contracted_count) return std::unexpected(contracted_count.error());
    if (*contracted_count == 0)
        return create(std::move(output_shape), std::vector<Number>(*output_count, Number{std::int64_t{0}}));

    std::vector<Number> output;
    output.reserve(*output_count);
    std::vector<std::size_t> left_index(shape_.size()), right_index(other.shape_.size());
    for (std::size_t cell = 0; cell < *output_count; ++cell) {
        const auto output_index = coordinates(cell, output_shape);
        std::size_t position = 0;
        for (std::size_t i = 0; i < shape_.size(); ++i) if (!left_used[i]) left_index[i] = output_index[position++];
        for (std::size_t i = 0; i < other.shape_.size(); ++i) if (!right_used[i]) right_index[i] = output_index[position++];
        Number total{std::int64_t{0}};
        for (std::size_t i = 0; i < *contracted_count; ++i) {
            auto remaining = i;
            for (std::size_t axis = left->size(); axis-- > 0;) {
                const auto index = remaining % contracted_shape[axis];
                remaining /= contracted_shape[axis];
                left_index[(*left)[axis]] = index;
                right_index[(*right)[axis]] = index;
            }
            auto next = accumulate_product(total, values_[offset(left_index, shape_)],
                                           other.values_[offset(right_index, other.shape_)]);
            if (!next) return std::unexpected(next.error());
            total = *next;
        }
        output.push_back(total);
    }
    return create(std::move(output_shape), std::move(output));
}

namespace detail {
MatrixResult<Matrix> reduce_axes(const Matrix& matrix, std::span<const std::ptrdiff_t> axes,
                                 bool keep_dimensions, bool mean) {
    if (!matrix.valid()) return fail(MatrixErrorCode::invalid_state, "Matrix is in a moved-from state");
    auto normalized = normalize_axes(axes, matrix.shape().size());
    if (!normalized) return std::unexpected(normalized.error());
    if (normalized->empty()) return Matrix::create(matrix.shape(), matrix.values());
    std::vector<bool> reduced(matrix.shape().size(), false);
    std::vector<std::size_t> reduction_shape;
    for (const auto axis : *normalized) {
        reduced[axis] = true;
        reduction_shape.push_back(matrix.shape()[axis]);
    }
    std::vector<std::size_t> output_shape;
    for (std::size_t i = 0; i < matrix.shape().size(); ++i) {
        if (!reduced[i]) output_shape.push_back(matrix.shape()[i]);
        else if (keep_dimensions) output_shape.push_back(1);
    }
    auto output_count = element_count(output_shape);
    if (!output_count) return std::unexpected(output_count.error());
    if (*output_count == 0) return Matrix::create(std::move(output_shape), {});
    auto reduction_count = element_count(reduction_shape);
    if (!reduction_count) return std::unexpected(reduction_count.error());
    if (mean && *reduction_count == 0) return fail(MatrixErrorCode::empty, "An empty reduction has no mean");
    if (mean && *reduction_count > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        return fail(MatrixErrorCode::overflow, "Reduction element count exceeds integer range");

    std::vector<Number> output(*output_count, Number{std::int64_t{0}});
    std::vector<std::size_t> strides(matrix.shape().size(), 0);
    std::size_t stride = 1;
    for (std::size_t i = matrix.shape().size(); i-- > 0;) {
        if (!reduced[i]) {
            strides[i] = stride;
            stride *= matrix.shape()[i];
        }
    }
    for (std::size_t flat = 0; flat < matrix.values().size(); ++flat) {
        auto remaining = flat;
        std::size_t output_offset = 0;
        for (std::size_t i = matrix.shape().size(); i-- > 0;) {
            const auto index = remaining % matrix.shape()[i];
            remaining /= matrix.shape()[i];
            output_offset += index * strides[i];
        }
        auto total = apply(output[output_offset], matrix.values()[flat], Operation::add);
        if (!total) return std::unexpected(total.error());
        output[output_offset] = *total;
    }
    if (mean) {
        for (auto& value : output) {
            auto average = apply(value, Number{static_cast<std::int64_t>(*reduction_count)}, Operation::divide);
            if (!average) return std::unexpected(average.error());
            value = *average;
        }
    }
    return Matrix::create(std::move(output_shape), std::move(output));
}
} // namespace detail

MatrixResult<Matrix> Matrix::sum(std::span<const std::ptrdiff_t> axes, bool keep_dimensions) const {
    return detail::reduce_axes(*this, axes, keep_dimensions, false);
}
MatrixResult<Matrix> Matrix::sum(std::ptrdiff_t axis, bool keep_dimensions) const {
    return sum(std::span<const std::ptrdiff_t>(&axis, 1), keep_dimensions);
}
MatrixResult<Matrix> Matrix::mean(std::span<const std::ptrdiff_t> axes, bool keep_dimensions) const {
    return detail::reduce_axes(*this, axes, keep_dimensions, true);
}
MatrixResult<Matrix> Matrix::mean(std::ptrdiff_t axis, bool keep_dimensions) const {
    return mean(std::span<const std::ptrdiff_t>(&axis, 1), keep_dimensions);
}

} // namespace ii::xml
