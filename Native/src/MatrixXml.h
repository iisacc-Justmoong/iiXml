#pragma once

#include "Matrix.h"
#include "Xml.h"

namespace ii::xml {

[[nodiscard]] MatrixResult<Matrix> matrix_from_xml(std::string utf8, Limits limits = {});
[[nodiscard]] MatrixResult<std::string> matrix_to_xml(const Matrix& matrix, Limits limits = {});

} // namespace ii::xml
