# Numeric matrices and XML

`iiXml::Native` provides an immutable, row-major numeric matrix and a separate
XML codec. `<iiXml/Matrix.h>` requires C++23; `<iiXml/MatrixXml.h>` adds XML
conversion. Link to `iiXml::Native` with `find_package(iiXmlNative CONFIG
REQUIRED)`. No Qt dependency is introduced.

## Shape and values

`ii::xml::Matrix::create(shape, values)` accepts a vector of dimensions and a
flat vector of `Number`, a variant of `int64_t` and `double`. The product of
dimensions must equal the value count. An empty shape is a scalar containing
one value; any zero dimension denotes an empty matrix. Dimension-product
overflow, non-finite doubles and invalid counts are errors. `reshape` preserves
the flat order and value types, returning a new matrix. `at` requires exactly
one in-range index per dimension, with the last dimension changing fastest.
The C++ `Number` constructor rejects `bool` at compile time.
After a move, `valid()` reports whether the source retains a usable matrix;
operations on an invalid moved-from source return an `invalid_state` error.

```cpp
#include <iiXml/Matrix.h>
#include <iiXml/MatrixXml.h>
#include <array>

using namespace ii::xml;
auto matrix = Matrix::create({2, 2}, {Number{std::int64_t{1}}, Number{2.5},
                                      Number{std::int64_t{3}}, Number{4.0}});
if (matrix) {
    auto reshaped = matrix->reshape({4});
    auto xml = matrix_to_xml(*matrix);
    auto row_sums = matrix->sum(-1);
    auto product = matrix->tensordot(*matrix, std::array<std::ptrdiff_t, 1>{1},
                                    std::array<std::ptrdiff_t, 1>{0});
    // Check each MatrixResult before using it; failures contain a code and message.
}
```

## Arithmetic

`add`, `subtract`, `multiply`, and `divide` apply element by element to equal
shapes; an empty-shape scalar can also be combined with any matrix. Operations
return new values and leave their operands unchanged.

`matmul` multiplies the final two dimensions and broadcasts leading batch
dimensions when they are equal or either is one. Rank-one operands are treated
as a row or column vector respectively, with the temporary dimension removed
from the result; two vectors produce a rank-zero matrix. A rank-zero operand or
incompatible inner/batch dimensions is an error.

`tensordot` contracts explicitly paired axes: the axis lists must have equal
length and each paired dimension must match. The result dimensions are the
uncontracted left dimensions followed by the uncontracted right dimensions,
each in their original order. Empty axis lists compute an outer product. A
zero-sized contracted dimension produces integer zero in every output cell.

`sum()` and `mean()` without arguments retain their existing scalar return
values and reduce all entries. Axis overloads accept one or more signed axes;
negative axes count from the end. Duplicate or out-of-range axes are errors.
The returned matrix drops reduced dimensions unless `keep_dimensions` is true,
in which case each reduced dimension is one. An empty axis list returns an
unchanged matrix. For an empty reduction, `sum` produces zeros and `mean` is
an error when at least one output cell exists; an empty output is returned
without an error. The sum of a wholly empty matrix is zero, and its whole-matrix
mean is an error.

Integer-only operations produce checked signed 64-bit integers, except an
inexact integer division produces a double. Floating-point operations use
binary64 and reject non-finite inputs and results. Converting an integer to
double for mixed arithmetic or fractional division is allowed only when that
integer is exactly representable; otherwise the operation reports an error.
Integer overflow, division by zero, incompatible shapes and invalid indices
are errors. A reduction or matrix multiplication reports overflow as soon as an
intermediate operation overflows. Floating-point rounding and underflow follow
the platform's double arithmetic.

## C interface

When the Python bridge is enabled, `<iiXml/MatrixC.h>` and the exported CMake
target `iiXml::MatrixC` provide the same operations through opaque handles and
tagged integer/real values. Check each nonzero return code and read the
thread-local `iixml_matrix_error()` immediately. Destroy handles with
`iixml_matrix_destroy` and free generated XML with `iixml_matrix_free_string`.
Axis reductions use `iixml_matrix_reduce_axes`; tensor contraction uses
`iixml_matrix_tensordot` with a separate signed axis array for each input.

## XML representation

`matrix_from_xml` and `matrix_to_xml` use the existing bounded `Reader` and
`Writer`. The format is a `<matrix>` root with a mandatory `shape` attribute,
containing zero or more ordered `<value>` children:

```xml
<matrix shape="2 2"><value type="integer">1</value><value type="real">2.5</value><value>3</value><value>4</value></matrix>
```

Dimensions are unsigned decimal integers separated by XML whitespace; the
empty attribute represents a scalar. Values are finite decimal integers or
reals, including decimal exponents. Surrounding XML whitespace is ignored.
An omitted `type` infers an integer for integer syntax and a real for decimal
or exponent syntax; `type="integer"` and `type="real"` select explicitly.
The writer emits these explicit types and enough digits for a binary64
round-trip. Unexpected elements, attributes, namespaces, nonnumeric values,
count mismatches and unsupported type names are rejected. XML comments and
processing instructions are ignored; nested value elements are not allowed.
Input/output and parser limits are the `ii::xml::Limits` supplied by the caller.

## Python

The standard-library-only `iixml_matrix` module uses the `iiXmlMatrixC` shared
library built by the project. It exposes `Matrix(shape, values)`, `shape`,
`values`, `at`, `reshape`, `from_xml`, `to_xml`, scalar and matrix arithmetic,
`matmul`, `tensordot`, `sum`, and `mean`. `sum(axis=..., keepdims=...)` and
`mean(axis=..., keepdims=...)` accept one integer axis or a sequence of axes;
omitting the axis preserves the scalar-returning behavior. When `keepdims=True`
with no axis, the result is a matrix with every original axis retained at size
one. `tensordot(other, left_axes, right_axes)` accepts one axis or a sequence
for each input. Python booleans are rejected, as are integers
outside signed 64-bit range. Errors are surfaced as `MatrixError` (with Python
type/range errors raised before calling native code where applicable). Add the
installed `lib/iiXmlNative/python` directory to `PYTHONPATH`; the module finds
the installed library relative to that path, or from `IIXML_MATRIX_LIBRARY`.
`IIXML_BUILD_MATRIX_PYTHON` controls this bridge and defaults off for iOS and
Emscripten, where a Python-loadable shared library is not generally available.
The C++ matrix and XML APIs remain part of `iiXml::Native` on those platforms.
For applications using Core's embedded CPython, iiSDKBuilder can AOT compile
`iixml_matrix.py` and attach it to an `iiSDKCore::CPython` host. The app supplies
the iiXml C library beside the compiled module or by `IIXML_MATRIX_LIBRARY`,
and provides the compiled-module directory to Core. The installed native CMake
package exposes `iiXmlNative_MATRIX_PYTHON_SOURCE` for Builder's AOT input.
The module does not link directly to Builder or Core. See the Builder's
CPython AOT contract.

```python
from iixml_matrix import Matrix
a = Matrix((2, 2), [1, 2, 3, 4])
b = Matrix.from_xml(a.to_xml()).reshape((4,))
assert (b / 2).values == [0.5, 1, 1.5, 2]
c = Matrix((2, 2, 2), range(1, 9))
assert c.sum(axis=-1).values == [3, 7, 11, 15]
assert c.mean(axis=(0, 2)).values == [3.5, 5.5]
assert c.tensordot(c, (1, 2), (1, 2)).shape == (2, 2)
```
