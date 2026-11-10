#pragma once

#include "Matrix.h"

namespace ii::xml::detail {

enum class Operation { add, subtract, multiply, divide };
[[nodiscard]] MatrixResult<Number> apply(Number lhs, Number rhs, Operation operation);
[[nodiscard]] MatrixResult<std::size_t> element_count(std::span<const std::size_t> shape);

} // namespace ii::xml::detail
