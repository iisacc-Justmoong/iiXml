#include "DOCTYPE.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view xml_declaration_start = "<?xml";
constexpr std::string_view xml_declaration_end = "?>";
constexpr std::string_view doctype_start = "<!DOCTYPE";
constexpr std::string_view utf8_bom = "\xEF\xBB\xBF";

bool is_space(char value) {
    return value == ' ' || value == '\n' || value == '\r' || value == '\t';
}

bool is_alpha(char value) {
    return ('a' <= value && value <= 'z') || ('A' <= value && value <= 'Z');
}

bool is_digit(char value) {
    return '0' <= value && value <= '9';
}

bool is_name_start(char value) {
    return is_alpha(value) || value == '_' || value == ':';
}

bool is_name_char(char value) {
    return is_name_start(value) || is_digit(value) || value == '-' || value == '.';
}

std::string_view trim_top(std::string_view input) {
    if (input.starts_with(utf8_bom)) {
        input.remove_prefix(utf8_bom.size());
    }

    while (!input.empty() && is_space(input.front())) {
        input.remove_prefix(1);
    }

    return input;
}

bool starts_with_token(std::string_view input, std::string_view token) {
    return input.size() >= token.size() && input.substr(0, token.size()) == token;
}

std::optional<std::size_t> find_xml_declaration_end(std::string_view input) {
    const std::size_t end = input.find(xml_declaration_end);
    if (end == std::string_view::npos) {
        return std::nullopt;
    }

    return end + xml_declaration_end.size();
}

std::optional<std::size_t> find_doctype_end(std::string_view input) {
    char quote = '\0';
    int subset_depth = 0;

    for (std::size_t index = 0; index < input.size(); ++index) {
        const char value = input[index];

        if (quote != '\0') {
            if (value == quote) {
                quote = '\0';
            }
            continue;
        }

        if (value == '"' || value == '\'') {
            quote = value;
            continue;
        }

        if (value == '[') {
            ++subset_depth;
            continue;
        }

        if (value == ']' && subset_depth > 0) {
            --subset_depth;
            continue;
        }

        if (value == '>' && subset_depth == 0) {
            return index + 1;
        }
    }

    return std::nullopt;
}

bool has_xml_declaration_body(std::string_view declaration) {
    if (declaration.size() <= xml_declaration_start.size() + xml_declaration_end.size()) {
        return false;
    }

    const char after_start = declaration[xml_declaration_start.size()];
    return is_space(after_start);
}

bool has_doctype_name(std::string_view declaration) {
    if (declaration.size() <= doctype_start.size() + 1) {
        return false;
    }

    std::size_t index = doctype_start.size();
    if (!is_space(declaration[index])) {
        return false;
    }

    while (index < declaration.size() && is_space(declaration[index])) {
        ++index;
    }

    if (index >= declaration.size() || !is_name_start(declaration[index])) {
        return false;
    }

    ++index;
    while (index < declaration.size() && is_name_char(declaration[index])) {
        ++index;
    }

    return index < declaration.size() && (is_space(declaration[index]) || declaration[index] == '>');
}

} // namespace

namespace iiXml::elements {

std::optional<doctype_match> DOCTYPE::match_top(std::string_view input) const {
    input = trim_top(input);

    if (starts_with_token(input, xml_declaration_start)) {
        const std::optional<std::size_t> end = find_xml_declaration_end(input);
        if (!end.has_value()) {
            return std::nullopt;
        }

        const std::string_view raw = input.substr(0, *end);
        if (!has_xml_declaration_body(raw)) {
            return std::nullopt;
        }

        return doctype_match{
            doctype_kind::xml_declaration,
            std::string(raw)
        };
    }

    if (starts_with_token(input, doctype_start)) {
        const std::optional<std::size_t> end = find_doctype_end(input);
        if (!end.has_value()) {
            return std::nullopt;
        }

        const std::string_view raw = input.substr(0, *end);
        if (!has_doctype_name(raw)) {
            return std::nullopt;
        }

        return doctype_match{
            doctype_kind::doctype_declaration,
            std::string(raw)
        };
    }

    return std::nullopt;
}

bool DOCTYPE::is_top_doctype(std::string_view input) const {
    const std::optional<doctype_match> matched = match_top(input);
    return matched.has_value() && matched->kind == doctype_kind::doctype_declaration;
}

bool DOCTYPE::is_top_xml_declaration(std::string_view input) const {
    const std::optional<doctype_match> matched = match_top(input);
    return matched.has_value() && matched->kind == doctype_kind::xml_declaration;
}

} // namespace iiXml::elements
