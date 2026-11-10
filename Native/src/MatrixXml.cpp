#include "MatrixXml.h"

#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>
#include <system_error>
#include <utility>

namespace ii::xml {
namespace {

auto fail(MatrixErrorCode code, std::string message) {
    return std::unexpected(MatrixError{code, std::move(message)});
}

bool whitespace(char ch) { return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'; }
std::string_view trim(std::string_view input) {
    while (!input.empty() && whitespace(input.front())) input.remove_prefix(1);
    while (!input.empty() && whitespace(input.back())) input.remove_suffix(1);
    return input;
}

MatrixResult<std::vector<std::size_t>> parse_shape(std::string_view text) {
    std::vector<std::size_t> shape;
    text = trim(text);
    while (!text.empty()) {
        const auto end = text.find_first_of(" \t\r\n");
        const auto token = text.substr(0, end);
        std::size_t dimension = 0;
        auto result = std::from_chars(token.data(), token.data() + token.size(), dimension);
        if (result.ec != std::errc{} || result.ptr != token.data() + token.size())
            return fail(MatrixErrorCode::invalid_shape, "Invalid XML matrix dimension");
        shape.push_back(dimension);
        if (end == text.npos) break;
        text = trim(text.substr(end));
    }
    return shape;
}

bool integer_syntax(std::string_view text) {
    if (!text.empty() && (text.front() == '-' || text.front() == '+')) text.remove_prefix(1);
    if (text.empty()) return false;
    for (const char ch : text) if (ch < '0' || ch > '9') return false;
    return true;
}

MatrixResult<Number> parse_number(std::string_view text, std::string_view type) {
    text = trim(text);
    if (text.empty()) return fail(MatrixErrorCode::invalid_number, "An XML value must contain a number");
    const bool integer = integer_syntax(text);
    if (type.empty()) type = integer ? "integer" : "real";
    if (type == "integer") {
        if (!integer) return fail(MatrixErrorCode::invalid_number, "Invalid integer value");
        if (text.front() == '+') text.remove_prefix(1);
        std::int64_t value = 0;
        auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
            return fail(MatrixErrorCode::invalid_number, "Integer is outside the signed 64-bit range");
        return Number{value};
    }
    if (type != "real") return fail(MatrixErrorCode::invalid_number, "Unsupported XML value type");
    for (const char ch : text) {
        if ((ch < '0' || ch > '9') && ch != '+' && ch != '-' && ch != '.' && ch != 'e' && ch != 'E')
            return fail(MatrixErrorCode::invalid_number, "Invalid real value");
    }
    if (text.front() == '+') text.remove_prefix(1);
    double value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, std::chars_format::general);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || !std::isfinite(value))
        return fail(MatrixErrorCode::invalid_number, "Invalid or out-of-range real value");
    return Number{value};
}

MatrixResult<std::string> number_text(const Number& number) {
    std::array<char, 128> buffer{};
    std::to_chars_result result;
    if (const auto* integer = std::get_if<std::int64_t>(&number))
        result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), *integer);
    else
        result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), std::get<double>(number),
                               std::chars_format::general, std::numeric_limits<double>::max_digits10);
    if (result.ec != std::errc{}) return fail(MatrixErrorCode::invalid_number, "Could not format matrix number");
    return std::string(buffer.data(), result.ptr);
}

MatrixResult<void> write_ok(Result<void> result) {
    if (!result) return fail(MatrixErrorCode::xml_error, result.error().message);
    return {};
}

} // namespace

MatrixResult<Matrix> matrix_from_xml(std::string utf8, Limits limits) {
    auto reader = Reader::open(std::move(utf8), limits);
    if (!reader) return fail(MatrixErrorCode::xml_error, reader.error().message);
    bool root_seen = false;
    bool root_closed = false;
    bool in_value = false;
    std::vector<std::size_t> shape;
    std::vector<Number> values;
    std::string value_text;
    std::string value_type;
    while (true) {
        auto next = reader->next();
        if (!next) return fail(MatrixErrorCode::xml_error, next.error().message);
        if (!*next) break;
        const auto& event = **next;
        if (event.kind == Kind::comment || event.kind == Kind::processing_instruction) continue;
        if (event.kind == Kind::doctype) return fail(MatrixErrorCode::xml_error, "DOCTYPE is not part of the matrix format");
        if (event.kind == Kind::start_element) {
            if (!root_seen) {
                if (event.name != "matrix" || !event.namespace_uri.empty() || event.depth != 0)
                    return fail(MatrixErrorCode::xml_error, "Expected an unqualified matrix root");
                root_seen = true;
                bool has_shape = false;
                for (const auto& attr : event.attributes) {
                    if (attr.name != "shape" || !attr.namespace_uri.empty() || has_shape)
                        return fail(MatrixErrorCode::xml_error, "Unexpected matrix attribute");
                    has_shape = true;
                    auto parsed = parse_shape(attr.value);
                    if (!parsed) return std::unexpected(parsed.error());
                    shape = std::move(*parsed);
                }
                if (!has_shape) return fail(MatrixErrorCode::invalid_shape, "Matrix shape attribute is required");
                root_closed = event.empty;
            } else {
                if (root_closed || in_value || event.name != "value" || !event.namespace_uri.empty() || event.depth != 1)
                    return fail(MatrixErrorCode::xml_error, "Expected a matrix value element");
                value_type.clear();
                value_text.clear();
                for (const auto& attr : event.attributes) {
                    if (attr.name != "type" || !attr.namespace_uri.empty() || !value_type.empty())
                        return fail(MatrixErrorCode::xml_error, "Unexpected value attribute");
                    value_type = attr.value;
                    if (value_type != "integer" && value_type != "real")
                        return fail(MatrixErrorCode::invalid_number, "Unsupported XML value type");
                }
                if (event.empty) return fail(MatrixErrorCode::invalid_number, "An XML value must contain a number");
                in_value = true;
            }
        } else if (event.kind == Kind::text) {
            if (in_value) {
                if (value_text.size() > limits.text_bytes || event.value.size() > limits.text_bytes - value_text.size())
                    return fail(MatrixErrorCode::xml_error, "Matrix value text exceeds its byte budget");
                value_text += event.value;
            } else if (!trim(event.value).empty()) {
                return fail(MatrixErrorCode::xml_error, "Unexpected text outside a matrix value");
            }
        } else if (event.kind == Kind::end_element) {
            if (in_value && event.name == "value" && event.depth == 1) {
                auto value = parse_number(value_text, value_type);
                if (!value) return std::unexpected(value.error());
                values.push_back(*value);
                in_value = false;
            } else if (root_seen && !root_closed && !in_value && event.name == "matrix" && event.depth == 0) {
                root_closed = true;
            } else {
                return fail(MatrixErrorCode::xml_error, "Unexpected closing element");
            }
        }
    }
    if (!root_seen || !root_closed || in_value) return fail(MatrixErrorCode::xml_error, "Incomplete matrix XML");
    return Matrix::create(std::move(shape), std::move(values));
}

MatrixResult<std::string> matrix_to_xml(const Matrix& matrix, Limits limits) {
    if (!matrix.valid()) return fail(MatrixErrorCode::invalid_state, "Matrix is in a moved-from state");
    std::string shape;
    for (const auto dimension : matrix.shape()) {
        if (!shape.empty()) shape += ' ';
        shape += std::to_string(dimension);
        if (shape.size() > limits.text_bytes) return fail(MatrixErrorCode::xml_error, "Matrix shape exceeds its byte budget");
    }
    Writer writer(limits);
    const std::array<Attribute, 1> shape_attr{{{"shape", shape}}};
    if (auto result = write_ok(writer.start("matrix", shape_attr)); !result) return std::unexpected(result.error());
    for (const auto& value : matrix.values()) {
        const std::array<Attribute, 1> type_attr{{{"type", std::holds_alternative<std::int64_t>(value) ? "integer" : "real"}}};
        if (auto result = write_ok(writer.start("value", type_attr)); !result) return std::unexpected(result.error());
        auto text = number_text(value);
        if (!text) return std::unexpected(text.error());
        if (auto result = write_ok(writer.text(*text)); !result) return std::unexpected(result.error());
        if (auto result = write_ok(writer.end()); !result) return std::unexpected(result.error());
    }
    if (auto result = write_ok(writer.end()); !result) return std::unexpected(result.error());
    auto xml = writer.finish();
    if (!xml) return fail(MatrixErrorCode::xml_error, xml.error().message);
    return std::move(*xml);
}

} // namespace ii::xml
