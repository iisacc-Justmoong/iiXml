#include "InputValidator.h"

#include "Src/Elements/DOCTYPE.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

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

std::string_view body_after_doctype(std::string_view input, const iiXml::elements::doctype_match& matched) {
    std::size_t offset = 0;
    if (input.starts_with(utf8_bom)) {
        offset += utf8_bom.size();
    }

    while (offset < input.size() && is_space(input[offset])) {
        ++offset;
    }

    offset += matched.raw.size();
    return offset <= input.size() ? input.substr(offset) : std::string_view{};
}

bool starts_with_at(std::string_view input, std::size_t index, std::string_view token) {
    return index + token.size() <= input.size() && input.substr(index, token.size()) == token;
}

std::optional<std::size_t> find_token_end(std::string_view input, std::size_t begin, std::string_view token) {
    const std::size_t end = input.find(token, begin);
    if (end == std::string_view::npos) {
        return std::nullopt;
    }

    return end + token.size();
}

std::optional<std::size_t> find_markup_end(std::string_view input, std::size_t begin) {
    char quote = '\0';

    for (std::size_t index = begin; index < input.size(); ++index) {
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

        if (value == '>') {
            return index;
        }
    }

    return std::nullopt;
}

std::optional<std::string> read_tag_name(std::string_view markup, std::size_t begin) {
    if (begin >= markup.size() || !is_name_start(markup[begin])) {
        return std::nullopt;
    }

    std::size_t end = begin + 1;
    while (end < markup.size() && is_name_char(markup[end])) {
        ++end;
    }

    return std::string(markup.substr(begin, end - begin));
}

bool is_self_closing_markup(std::string_view markup) {
    if (markup.size() < 2 || markup.back() != '>') {
        return false;
    }

    std::size_t index = markup.size() - 1;
    while (index > 0 && is_space(markup[index - 1])) {
        --index;
    }

    return index > 0 && markup[index - 1] == '/';
}

} // namespace

namespace iiXml::writer {

validation_exit InputValidator::validate(std::string_view input) const {
    const iiXml::elements::DOCTYPE doctype;
    const std::optional<iiXml::elements::doctype_match> matched = doctype.match_top(input);
    if (!matched.has_value()) {
        return validation_exit::invalid_xml_file;
    }

    if (!has_valid_tag_closure(body_after_doctype(input, *matched))) {
        return validation_exit::invalid_tag_closure;
    }

    return validation_exit::valid;
}

bool InputValidator::has_valid_tag_closure(std::string_view input) const {
    input = trim_outer(input);
    std::vector<std::string> open_tags;
    bool saw_element = false;

    for (std::size_t index = 0; index < input.size();) {
        const std::size_t tag_start = input.find('<', index);
        if (tag_start == std::string_view::npos) {
            break;
        }

        if (starts_with_at(input, tag_start, "<!--")) {
            const std::optional<std::size_t> end = find_token_end(input, tag_start + 4, "-->");
            if (!end.has_value()) {
                return false;
            }
            index = *end;
            continue;
        }

        if (starts_with_at(input, tag_start, "<![CDATA[")) {
            const std::optional<std::size_t> end = find_token_end(input, tag_start + 9, "]]>");
            if (!end.has_value()) {
                return false;
            }
            index = *end;
            continue;
        }

        if (starts_with_at(input, tag_start, "<?")) {
            const std::optional<std::size_t> end = find_token_end(input, tag_start + 2, "?>");
            if (!end.has_value()) {
                return false;
            }
            index = *end;
            continue;
        }

        if (starts_with_at(input, tag_start, "<!DOCTYPE")) {
            const iiXml::elements::DOCTYPE doctype;
            const std::optional<iiXml::elements::doctype_match> matched = doctype.match_top(input.substr(tag_start));
            if (!matched.has_value()) {
                return false;
            }
            index = tag_start + matched->raw.size();
            continue;
        }

        const std::optional<std::size_t> tag_end = find_markup_end(input, tag_start + 1);
        if (!tag_end.has_value()) {
            return false;
        }

        const std::string_view markup = input.substr(tag_start, *tag_end - tag_start + 1);
        if (markup.size() < 3) {
            return false;
        }

        if (markup[1] == '/') {
            const std::optional<std::string> tag_name = read_tag_name(markup, 2);
            if (!tag_name.has_value() || open_tags.empty() || open_tags.back() != *tag_name) {
                return false;
            }

            open_tags.pop_back();
        } else {
            const std::optional<std::string> tag_name = read_tag_name(markup, 1);
            if (!tag_name.has_value()) {
                return false;
            }

            saw_element = true;
            if (!is_self_closing_markup(markup)) {
                open_tags.push_back(*tag_name);
            }
        }

        index = *tag_end + 1;
    }

    return saw_element && open_tags.empty();
}

} // namespace iiXml::writer
