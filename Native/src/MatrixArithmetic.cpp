#include "MatrixArithmetic.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <utility>

namespace ii::xml::detail {
namespace {

auto fail(MatrixErrorCode code, std::string message) {
    return std::unexpected(MatrixError{code, std::move(message)});
}

MatrixResult<double> exact_double(std::int64_t value) {
    const auto bits = static_cast<std::uint64_t>(value);
    const std::uint64_t magnitude = value < 0 ? std::uint64_t{0} - bits : bits;
    const auto width = std::bit_width(magnitude);
    constexpr auto precision = std::numeric_limits<double>::digits;
    if (width > precision) {
        const auto lost = width - precision;
        if ((magnitude & ((std::uint64_t{1} << lost) - 1)) != 0)
            return fail(MatrixErrorCode::precision_loss, "Integer cannot be represented exactly as a double");
    }
    return static_cast<double>(value);
}
} // namespace

MatrixResult<std::size_t> element_count(std::span<const std::size_t> shape) {
    if (std::ranges::find(shape, 0) != shape.end()) return 0;
    std::size_t count = 1;
    for (const auto dimension : shape) {
        if (dimension > std::numeric_limits<std::size_t>::max() / count)
            return fail(MatrixErrorCode::overflow, "Matrix dimension product overflows size_t");
        count *= dimension;
    }
    return count;
}

MatrixResult<Number> apply(Number lhs, Number rhs, Operation operation) {
    const auto* left_integer = std::get_if<std::int64_t>(&lhs);
    const auto* right_integer = std::get_if<std::int64_t>(&rhs);
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    constexpr auto min = std::numeric_limits<std::int64_t>::min();
    if (left_integer && right_integer) {
        const auto a = *left_integer;
        const auto b = *right_integer;
        switch (operation) {
        case Operation::add:
            if ((b > 0 && a > max - b) || (b < 0 && a < min - b))
                return fail(MatrixErrorCode::overflow, "Integer addition overflow");
            return Number{a + b};
        case Operation::subtract:
            if ((b < 0 && a > max + b) || (b > 0 && a < min + b))
                return fail(MatrixErrorCode::overflow, "Integer subtraction overflow");
            return Number{a - b};
        case Operation::multiply:
            if (a != 0 && b != 0 &&
                ((a > 0 && b > 0 && a > max / b) ||
                 (a > 0 && b < 0 && b < min / a) ||
                 (a < 0 && b > 0 && a < min / b) ||
                 (a < 0 && b < 0 && a < max / b)))
                return fail(MatrixErrorCode::overflow, "Integer multiplication overflow");
            return Number{a * b};
        case Operation::divide:
            if (b == 0) return fail(MatrixErrorCode::division_by_zero, "Division by zero");
            if (a == min && b == -1) return fail(MatrixErrorCode::overflow, "Integer division overflow");
            if (a % b == 0) return Number{a / b};
            break;
        }
    }
    if (operation == Operation::divide &&
        ((right_integer && *right_integer == 0) ||
         (!right_integer && std::get<double>(rhs) == 0.0)))
        return fail(MatrixErrorCode::division_by_zero, "Division by zero");
    auto left = left_integer ? exact_double(*left_integer) : MatrixResult<double>{std::get<double>(lhs)};
    if (!left) return std::unexpected(left.error());
    auto right = right_integer ? exact_double(*right_integer) : MatrixResult<double>{std::get<double>(rhs)};
    if (!right) return std::unexpected(right.error());
    double result = 0;
    switch (operation) {
    case Operation::add: result = *left + *right; break;
    case Operation::subtract: result = *left - *right; break;
    case Operation::multiply: result = *left * *right; break;
    case Operation::divide: result = *left / *right; break;
    }
    if (!std::isfinite(result)) return fail(MatrixErrorCode::overflow, "Floating-point result is not finite");
    return Number{result};
}

} // namespace ii::xml::detail
