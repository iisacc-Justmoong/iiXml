#include <iiXml/Matrix.h>
#include <iiXml/MatrixXml.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace ii::xml;
static_assert(!std::is_constructible_v<Number, bool>, "Booleans must not become numeric values implicitly");
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x "\n"; std::exit(1); } } while (false)

static Number integer(std::int64_t n) { return Number{n}; }
static Number real(double n) { return Number{n}; }

int main() {
    const auto a = Matrix::create({2, 2}, {integer(1), integer(2), integer(3), integer(4)});
    CHECK(a && a->shape() == std::vector<std::size_t>({2, 2}));
    CHECK(a->at(std::array<std::size_t, 2>{1, 0}) == integer(3));
    CHECK(!a->at(std::array<std::size_t, 2>{2, 0}));
    CHECK(!a->at(std::array<std::size_t, 1>{0}));
    auto reshaped = a->reshape({2, 1, 2});
    CHECK(reshaped && reshaped->values() == a->values());
    CHECK(reshaped->at(std::array<std::size_t, 3>{1, 0, 1}) == integer(4));
    CHECK(!a->reshape({3, 2}));
    CHECK(!Matrix::create({2}, {integer(1)}));
    CHECK(!Matrix::create({std::numeric_limits<std::size_t>::max(), 2}, {}));
    CHECK(!Matrix::create({1}, {real(std::numeric_limits<double>::infinity())}));
    CHECK(!Matrix::create({1}, {real(std::numeric_limits<double>::quiet_NaN())}));
    auto scalar = Matrix::create({}, {integer(2)});
    CHECK(scalar && scalar->at({}) == integer(2));
    auto source = *scalar;
    auto moved = std::move(source);
    CHECK(moved.valid() && moved.at({}) == integer(2));
    if (!source.valid()) {
        CHECK(!source.at({}) && !source.add(moved) && !source.sum() && !matrix_to_xml(source));
    }
    auto empty = Matrix::create({0, 3}, {});
    CHECK(empty && empty->sum() == integer(0) && !empty->mean());
    CHECK(empty->reshape({3, 0}) && !empty->at(std::array<std::size_t, 2>{0, 0}));
    CHECK(Matrix::create({std::numeric_limits<std::size_t>::max(), 0, 2}, {}));

    auto scaled = a->multiply(*scalar);
    CHECK(scaled && scaled->values() == std::vector<Number>({integer(2), integer(4), integer(6), integer(8)}));
    auto quotient = a->divide(*scalar);
    CHECK(quotient && quotient->values() == std::vector<Number>({real(0.5), integer(1), real(1.5), integer(2)}));
    auto subtracted = scalar->subtract(*a);
    CHECK(subtracted && subtracted->values() == std::vector<Number>({integer(1), integer(0), integer(-1), integer(-2)}));
    auto mixed = Matrix::create({2, 2}, {real(0.25), integer(1), real(-0.5), integer(5)});
    auto added = a->add(*mixed);
    CHECK(added && added->values() == std::vector<Number>({real(1.25), integer(3), real(2.5), integer(9)}));
    CHECK(a->sum() == integer(10) && a->mean() == real(2.5));
    CHECK(!a->add(*reshaped));
    auto zero = Matrix::create({}, {integer(0)});
    CHECK(!a->divide(*zero));
    auto large = Matrix::create({}, {integer(std::numeric_limits<std::int64_t>::max())});
    auto one = Matrix::create({}, {integer(1)});
    auto minus_one = Matrix::create({}, {integer(-1)});
    auto minimum = Matrix::create({}, {integer(std::numeric_limits<std::int64_t>::min())});
    auto floating = Matrix::create({}, {real(1.0)});
    CHECK(!large->add(*one) && !minimum->subtract(*one) && !minimum->multiply(*minus_one));
    CHECK(!minimum->divide(*minus_one) && !large->add(*floating));
    auto exact_large = Matrix::create({}, {integer(std::int64_t{1} << 60)});
    CHECK(exact_large->add(*floating));
    auto maxreal = Matrix::create({}, {real(std::numeric_limits<double>::max())});
    CHECK(!maxreal->add(*maxreal));

    auto right = Matrix::create({2, 3}, {integer(5), integer(6), integer(7), integer(8), integer(9), integer(10)});
    auto product = a->matmul(*right);
    CHECK(product && product->shape() == std::vector<std::size_t>({2, 3}));
    CHECK(product->values() == std::vector<Number>({integer(21), integer(24), integer(27), integer(47), integer(54), integer(61)}));
    CHECK(!right->matmul(*a) && !scalar->matmul(*a));
    auto left_zero = Matrix::create({2, 0}, {});
    auto right_zero = Matrix::create({0, 3}, {});
    auto product_zero = left_zero->matmul(*right_zero);
    CHECK(product_zero && product_zero->shape() == std::vector<std::size_t>({2, 3}));
    CHECK(product_zero->values() == std::vector<Number>(6, integer(0)));
    auto enormous_empty = Matrix::create({std::numeric_limits<std::size_t>::max(), 0}, {});
    auto no_columns = Matrix::create({0, 0}, {});
    CHECK(enormous_empty->matmul(*no_columns)->values().empty());

    auto batch = Matrix::create({2, 1, 2, 3}, {integer(1), integer(2), integer(3), integer(4), integer(5), integer(6),
                                                 integer(7), integer(8), integer(9), integer(10), integer(11), integer(12)});
    auto kernels = Matrix::create({1, 2, 3, 2}, {integer(1), integer(0), integer(0), integer(1), integer(1), integer(1),
                                                  integer(2), integer(0), integer(0), integer(2), integer(1), integer(-1)});
    auto batches = batch->matmul(*kernels);
    CHECK(batches && batches->shape() == std::vector<std::size_t>({2, 2, 2, 2}));
    CHECK(batches->values() == std::vector<Number>({integer(4), integer(5), integer(10), integer(11),
        integer(5), integer(1), integer(14), integer(4), integer(16), integer(17), integer(22), integer(23),
        integer(23), integer(7), integer(32), integer(10)}));
    auto vec = Matrix::create({3}, {integer(1), integer(2), integer(3)});
    auto dot = vec->matmul(*vec);
    CHECK(dot && dot->shape().empty() && dot->values() == std::vector<Number>({integer(14)}));
    auto batched_vec = batch->matmul(*vec);
    CHECK(batched_vec && batched_vec->shape() == std::vector<std::size_t>({2, 1, 2}));
    CHECK(batched_vec->values() == std::vector<Number>({integer(14), integer(32), integer(50), integer(68)}));

    auto axis_sum = batch->sum(std::array<std::ptrdiff_t, 2>{0, -1});
    CHECK(axis_sum && axis_sum->shape() == std::vector<std::size_t>({1, 2}));
    CHECK(axis_sum->values() == std::vector<Number>({integer(30), integer(48)}));
    auto axis_mean = batch->mean(-1, true);
    CHECK(axis_mean && axis_mean->shape() == std::vector<std::size_t>({2, 1, 2, 1}));
    CHECK(axis_mean->values() == std::vector<Number>({integer(2), integer(5), integer(8), integer(11)}));
    CHECK(!batch->sum(std::array<std::ptrdiff_t, 2>{0, -4}));
    CHECK(!batch->sum(4));
    auto empty_sum = left_zero->sum(1);
    CHECK(empty_sum && empty_sum->values() == std::vector<Number>({integer(0), integer(0)}));
    CHECK(!left_zero->mean(1));

    auto tensor_left = Matrix::create({2, 2, 2}, {integer(1), integer(2), integer(3), integer(4),
                                                  integer(5), integer(6), integer(7), integer(8)});
    auto tensor_right = Matrix::create({2, 2, 2}, {integer(1), integer(0), integer(0), integer(1),
                                                   integer(1), integer(1), integer(2), integer(0)});
    auto contraction = tensor_left->tensordot(*tensor_right,
        std::array<std::ptrdiff_t, 2>{1, 2}, std::array<std::ptrdiff_t, 2>{0, 1});
    CHECK(contraction && contraction->shape() == std::vector<std::size_t>({2, 2}));
    CHECK(contraction->values() == std::vector<Number>({integer(12), integer(5), integer(28), integer(13)}));
    CHECK(!tensor_left->tensordot(*tensor_right, std::array<std::ptrdiff_t, 2>{0, 0},
                                  std::array<std::ptrdiff_t, 2>{0, 1}));

    auto xml = matrix_to_xml(*mixed);
    CHECK(xml);
    auto roundtrip = matrix_from_xml(*xml);
    CHECK(roundtrip && roundtrip->shape() == mixed->shape() && roundtrip->values() == mixed->values());
    auto inferred = matrix_from_xml("<matrix shape='2 2'><value>1</value><value> 2.5 </value><value>3e0</value><value type='real'>4</value></matrix>");
    CHECK(inferred && inferred->values() == std::vector<Number>({integer(1), real(2.5), real(3.0), real(4.0)}));
    auto scalar_xml = matrix_to_xml(*scalar);
    CHECK(scalar_xml && matrix_from_xml(*scalar_xml) && matrix_from_xml(*scalar_xml)->shape().empty());
    auto empty_xml = matrix_from_xml("<matrix shape='0 3'/>");
    CHECK(empty_xml && empty_xml->values().empty());
    auto extremes = Matrix::create({4}, {real(-0.0), real(std::numeric_limits<double>::denorm_min()),
                                        integer(std::numeric_limits<std::int64_t>::min()), real(1.0)});
    CHECK(extremes);
    auto extreme_xml = matrix_to_xml(*extremes);
    CHECK(extreme_xml);
    auto extreme_roundtrip = matrix_from_xml(*extreme_xml);
    CHECK(extreme_roundtrip && extreme_roundtrip->values() == extremes->values());
    CHECK(std::signbit(std::get<double>(extreme_roundtrip->values()[0])));
    for (const std::string bad : {
        "<matrix><value>1</value></matrix>",
        "<matrix shape='1'><value>true</value></matrix>",
        "<matrix shape='1'><value>NaN</value></matrix>",
        "<matrix shape='1'><value>9223372036854775808</value></matrix>",
        "<matrix shape='1'><value type='integer'>1.5</value></matrix>",
        "<matrix shape='1'><value type='bool'>1</value></matrix>",
        "<matrix shape='1'><value>1</value><value>2</value></matrix>",
        "<matrix shape='1'><other>1</other></matrix>",
        "<matrix shape='1'><value><value>1</value></value></matrix>",
        "<matrix shape='1' unknown='x'><value>1</value></matrix>",
        "<matrix xmlns='urn:x' shape='1'><value>1</value></matrix>",
        "<!DOCTYPE matrix SYSTEM 'file:///not-read'><matrix shape='0'/>",
    }) CHECK(!matrix_from_xml(bad));
    Limits small; small.input_bytes = 8;
    CHECK(!matrix_from_xml("<matrix shape='0'/>", small));
    small = {}; small.output_bytes = 20;
    CHECK(!matrix_to_xml(*a, small));
    std::cout << "numeric matrix contracts passed\n";
}
