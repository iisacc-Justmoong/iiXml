#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace ii::xml {

// The deleted overload prevents C++ bool from silently becoming integer 0/1.
class Number final : public std::variant<std::int64_t, double> {
public:
    using std::variant<std::int64_t, double>::variant;
    Number(bool) = delete;
    friend bool operator==(const Number&, const Number&) = default;
};

enum class MatrixErrorCode {
    invalid_shape,
    invalid_number,
    precision_loss,
    overflow,
    division_by_zero,
    shape_mismatch,
    out_of_bounds,
    empty,
    invalid_state,
    xml_error
};

struct MatrixError final {
    MatrixErrorCode code;
    std::string message;
};

template<class T> using MatrixResult = std::expected<T, MatrixError>;

// Immutable, row-major matrix. An empty shape is a single scalar.
class Matrix final {
public:
    [[nodiscard]] static MatrixResult<Matrix> create(std::vector<std::size_t> shape,
                                                     std::vector<Number> values);
    [[nodiscard]] const std::vector<std::size_t>& shape() const noexcept { return shape_; }
    [[nodiscard]] const std::vector<Number>& values() const noexcept { return values_; }
    // A moved-from matrix can be inspected for validity before reuse.
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] MatrixResult<Number> at(std::span<const std::size_t> index) const;
    [[nodiscard]] MatrixResult<Matrix> reshape(std::vector<std::size_t> shape) const;

    // Equal shapes or a rank-zero scalar; multiply is elementwise.
    [[nodiscard]] MatrixResult<Matrix> add(const Matrix& other) const;
    [[nodiscard]] MatrixResult<Matrix> subtract(const Matrix& other) const;
    [[nodiscard]] MatrixResult<Matrix> multiply(const Matrix& other) const;
    [[nodiscard]] MatrixResult<Matrix> divide(const Matrix& other) const;
    [[nodiscard]] MatrixResult<Matrix> matmul(const Matrix& other) const;
    // Paired axes are contracted in order; negative axes count from the end.
    [[nodiscard]] MatrixResult<Matrix> tensordot(const Matrix& other,
        std::span<const std::ptrdiff_t> left_axes, std::span<const std::ptrdiff_t> right_axes) const;
    [[nodiscard]] MatrixResult<Number> sum() const;
    [[nodiscard]] MatrixResult<Number> mean() const;
    [[nodiscard]] MatrixResult<Matrix> sum(std::span<const std::ptrdiff_t> axes, bool keep_dimensions = false) const;
    [[nodiscard]] MatrixResult<Matrix> sum(std::ptrdiff_t axis, bool keep_dimensions = false) const;
    [[nodiscard]] MatrixResult<Matrix> mean(std::span<const std::ptrdiff_t> axes, bool keep_dimensions = false) const;
    [[nodiscard]] MatrixResult<Matrix> mean(std::ptrdiff_t axis, bool keep_dimensions = false) const;

private:
    Matrix(std::vector<std::size_t> shape, std::vector<Number> values);
    std::vector<std::size_t> shape_;
    std::vector<Number> values_;
};

} // namespace ii::xml
