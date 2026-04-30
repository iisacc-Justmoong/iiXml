#include "TagParser.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace {

bool is_space(char value) {
    return value == ' ' || value == '\n' || value == '\r' || value == '\t';
}

std::string_view trim_outer(std::string_view input) {
    std::size_t begin = 0;
    while (begin < input.size() && is_space(input[begin])) {
        ++begin;
    }

    std::size_t end = input.size();
    while (end > begin && is_space(input[end - 1])) {
        --end;
    }

    return input.substr(begin, end - begin);
}

bool is_alpha(char value) {
    return ('a' <= value && value <= 'z') || ('A' <= value && value <= 'Z');
}

bool is_digit(char value) {
    return '0' <= value && value <= '9';
}

bool is_tag_start(char value) {
    return is_alpha(value) || value == '_';
}

bool is_tag_char(char value) {
    return is_tag_start(value) || is_digit(value) || value == '-' || value == '.' || value == ':';
}

bool is_valid_tag_name(std::string_view tag_name) {
    if (tag_name.empty() || !is_tag_start(tag_name.front())) {
        return false;
    }

    for (char value : tag_name.substr(1)) {
        if (!is_tag_char(value)) {
            return false;
        }
    }

    return true;
}

} // namespace

namespace iixml::parser {

std::optional<tag_value> tag_parser::parse(std::string_view input) const {
    input = trim_outer(input);
    if (input.empty() || input.front() != '<') {
        return std::nullopt;
    }

    const std::size_t opening_end = input.find('>');
    if (opening_end == std::string_view::npos) {
        return std::nullopt;
    }

    const std::string_view tag_name = input.substr(1, opening_end - 1);
    if (!is_valid_tag_name(tag_name)) {
        return std::nullopt;
    }

    const std::string closing_tag = "</" + std::string(tag_name) + ">";
    if (input.size() < opening_end + 1 + closing_tag.size()) {
        return std::nullopt;
    }

    const std::size_t closing_start = input.size() - closing_tag.size();
    if (input.compare(closing_start, closing_tag.size(), closing_tag) != 0) {
        return std::nullopt;
    }

    const std::size_t value_start = opening_end + 1;
    const std::size_t value_size = closing_start - value_start;
    return tag_value{
        std::string(tag_name),
        std::string(input.substr(value_start, value_size))
    };
}

} // namespace iixml::parser
