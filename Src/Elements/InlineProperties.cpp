#include "InlineProperties.h"

#include "Src/Logging/XmlLog.h"

#include <QDebug>

#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

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

void skip_spaces(std::string_view input, std::size_t& index, std::size_t end) {
    while (index < end && is_space(input[index])) {
        ++index;
    }
}

bool looks_like_int(std::string_view value) {
    if (value.empty()) {
        return false;
    }

    std::size_t index = 0;
    if (value[index] == '+' || value[index] == '-') {
        ++index;
    }

    if (index == value.size()) {
        return false;
    }

    for (; index < value.size(); ++index) {
        if (!is_digit(value[index])) {
            return false;
        }
    }

    return true;
}

bool looks_like_float(std::string_view value) {
    if (value.empty()) {
        return false;
    }

    std::size_t index = 0;
    if (value[index] == '+' || value[index] == '-') {
        ++index;
    }

    bool saw_digit = false;
    bool saw_dot = false;
    while (index < value.size()) {
        if (is_digit(value[index])) {
            saw_digit = true;
            ++index;
            continue;
        }

        if (value[index] == '.' && !saw_dot) {
            saw_dot = true;
            ++index;
            continue;
        }

        break;
    }

    bool saw_exponent = false;
    if (index < value.size() && (value[index] == 'e' || value[index] == 'E')) {
        saw_exponent = true;
        ++index;
        if (index < value.size() && (value[index] == '+' || value[index] == '-')) {
            ++index;
        }

        bool exponent_digit = false;
        while (index < value.size() && is_digit(value[index])) {
            exponent_digit = true;
            ++index;
        }

        if (!exponent_digit) {
            return false;
        }
    }

    return index == value.size() && saw_digit && (saw_dot || saw_exponent);
}

bool looks_like_bool(std::string_view value) {
    return value == "true" || value == "false";
}

iiXml::elements::inline_property_type infer_type(
    std::string_view value,
    bool was_quoted
) {
    if (was_quoted) {
        return iiXml::elements::inline_property_type::string_type;
    }

    if (looks_like_bool(value)) {
        return iiXml::elements::inline_property_type::bool_type;
    }

    if (looks_like_int(value)) {
        return iiXml::elements::inline_property_type::int_type;
    }

    if (looks_like_float(value)) {
        return iiXml::elements::inline_property_type::float_type;
    }

    return iiXml::elements::inline_property_type::string_type;
}

const char* property_type_name(iiXml::elements::inline_property_type type) {
    switch (type) {
        case iiXml::elements::inline_property_type::string_type:
            return "string";
        case iiXml::elements::inline_property_type::int_type:
            return "int";
        case iiXml::elements::inline_property_type::float_type:
            return "float";
        case iiXml::elements::inline_property_type::bool_type:
            return "bool";
    }

    return "unknown";
}

} // namespace

namespace iiXml::elements {

InlineProperties::InlineProperties(QObject* parent)
    : QObject(parent) {
    qDebug() << "iiXml::elements::InlineProperties::InlineProperties constructed";
}

std::optional<std::vector<inline_property>> InlineProperties::parse(
    std::string_view opening_tag,
    std::size_t source_offset
) const {
    qDebug() << "iiXml::elements::InlineProperties::parse begin"
             << "input_size=" << opening_tag.size()
             << "source_offset=" << source_offset;
    iiXml::logging::log_input_summary("iiXml::elements::InlineProperties::parse", opening_tag);
    try {
        if (opening_tag.size() < 2 || opening_tag.front() != '<' || opening_tag.back() != '>') {
            iiXml::logging::log_parse_failure(
                "iiXml::elements::InlineProperties::parse",
                "opening tag markup expected",
                opening_tag,
                opening_tag.empty() ? 0 : opening_tag.size() - 1
            );
            return std::nullopt;
        }

        const std::size_t close_bracket = opening_tag.size() - 1;
        std::size_t index = 1;
        if (index >= close_bracket || !is_name_start(opening_tag[index])) {
            iiXml::logging::log_parse_failure(
                "iiXml::elements::InlineProperties::parse",
                "invalid tag name",
                opening_tag,
                index
            );
            return std::nullopt;
        }

        ++index;
        while (index < close_bracket && is_name_char(opening_tag[index])) {
            ++index;
        }

        std::vector<inline_property> properties;
        while (index < close_bracket) {
            skip_spaces(opening_tag, index, close_bracket);
            if (index >= close_bracket) {
                break;
            }

            if (opening_tag[index] == '/') {
                ++index;
                skip_spaces(opening_tag, index, close_bracket);
                if (index == close_bracket) {
                    break;
                }

                iiXml::logging::log_parse_failure(
                    "iiXml::elements::InlineProperties::parse",
                    "unexpected self closing marker",
                    opening_tag,
                    index
                );
                return std::nullopt;
            }

            if (!is_name_start(opening_tag[index])) {
                iiXml::logging::log_parse_failure(
                    "iiXml::elements::InlineProperties::parse",
                    "invalid property name",
                    opening_tag,
                    index
                );
                return std::nullopt;
            }

            const std::size_t name_begin = index;
            ++index;
            while (index < close_bracket && is_name_char(opening_tag[index])) {
                ++index;
            }
            const std::size_t name_end = index;
            iiXml::logging::log_parse_event(
                "iiXml::elements::InlineProperties::parse",
                std::string("property name=")
                    + std::string(opening_tag.substr(name_begin, name_end - name_begin)),
                opening_tag,
                name_begin
            );

            skip_spaces(opening_tag, index, close_bracket);

            bool has_value = false;
            bool type_declared = false;
            bool quoted_value = false;
            std::size_t value_begin = name_end;
            std::size_t value_end = name_end;
            inline_property_type value_type = inline_property_type::string_type;

            if (index < close_bracket && opening_tag[index] == '=') {
                has_value = true;
                ++index;
                skip_spaces(opening_tag, index, close_bracket);
                if (index >= close_bracket) {
                    iiXml::logging::log_parse_failure(
                        "iiXml::elements::InlineProperties::parse",
                        "missing property value",
                        opening_tag,
                        index
                    );
                    return std::nullopt;
                }

                if (opening_tag[index] == '"' || opening_tag[index] == '\'') {
                    quoted_value = true;
                    const char quote = opening_tag[index];
                    ++index;
                    value_begin = index;
                    while (index < close_bracket && opening_tag[index] != quote) {
                        ++index;
                    }
                    if (index >= close_bracket) {
                        iiXml::logging::log_parse_failure(
                            "iiXml::elements::InlineProperties::parse",
                            "unclosed quoted property value",
                            opening_tag,
                            value_begin > 0 ? value_begin - 1 : value_begin
                        );
                        return std::nullopt;
                    }
                    value_end = index;
                    ++index;
                } else {
                    value_begin = index;
                    while (index < close_bracket
                        && !is_space(opening_tag[index])
                        && opening_tag[index] != '/') {
                        ++index;
                    }
                    value_end = index;
                }

                value_type = infer_type(
                    opening_tag.substr(value_begin, value_end - value_begin),
                    quoted_value
                );
            }

            qDebug() << "iiXml::elements::InlineProperties::parse property"
                     << "name=" << QString::fromStdString(std::string(opening_tag.substr(name_begin, name_end - name_begin)))
                     << "has_value=" << has_value
                     << "type=" << property_type_name(value_type)
                     << "type_declared=" << type_declared;
            properties.push_back(inline_property{
                std::string(opening_tag.substr(name_begin, name_end - name_begin)),
                source_offset + name_begin,
                source_offset + name_end,
                has_value,
                source_offset + value_begin,
                source_offset + value_end,
                value_type,
                type_declared
            });
        }

        qDebug() << "iiXml::elements::InlineProperties::parse parsed"
                 << "property_count=" << properties.size();
        iiXml::logging::log_output_summary(
            "iiXml::elements::InlineProperties::parse",
            "parsed",
            std::string("property_count=") + std::to_string(properties.size())
        );
        return properties;
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::elements::InlineProperties::parse exception"
                 << "what=" << exception.what();
        return std::nullopt;
    } catch (...) {
        qDebug() << "iiXml::elements::InlineProperties::parse exception"
                 << "what=unknown";
        return std::nullopt;
    }
}

} // namespace iiXml::elements
