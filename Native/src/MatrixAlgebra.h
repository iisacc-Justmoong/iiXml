#pragma once

#include "Matrix.h"

namespace ii::xml::detail {

[[nodiscard]] MatrixResult<Matrix> reduce_axes(const Matrix& matrix,
    std::span<const std::ptrdiff_t> axes, bool keep_dimensions, bool mean);

} // namespace ii::xml::detail
