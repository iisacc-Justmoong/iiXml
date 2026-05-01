#include "TagParser.h"

#include "Src/Logging/XmlLog.h"
#include "Src/Elements/OpenTag.h"

#include <QByteArray>
#include <QDebug>
#include <QString>

#include <cstddef>
#include <exception>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

std::string_view read_opening_tag_name(std::string_view opening_tag) {
    opening_tag = trim_outer(opening_tag);
    std::size_t end = 0;
    while (end < opening_tag.size() && !is_space(opening_tag[end])) {
        ++end;
    }

    return opening_tag.substr(0, end);
}

} // namespace

namespace iiXml::parser {

namespace {

struct tag_node_entry {
    tag_node node;
    std::vector<std::size_t> children;
};

bool contains_range(const tag_range& parent, const tag_range& child) {
    return parent.raw_begin < child.raw_begin && child.raw_end <= parent.raw_end;
}

std::optional<std::vector<tag_field>> parse_fields(
    std::string_view input,
    const tag_range& range
) {
    iiXml::logging::log_parse_event(
        "iiXml::parser::tag_parser::parse_all",
        "fields_begin",
        input,
        range.raw_begin
    );

    if (range.value_begin <= range.raw_begin + 1 || range.value_begin > input.size()) {
        iiXml::logging::log_parse_failure(
            "iiXml::parser::tag_parser::parse_all",
            "invalid tag range before field parse",
            input,
            range.raw_begin
        );
        return std::nullopt;
    }

    const std::size_t close_bracket = range.value_begin - 1;
    if (close_bracket >= input.size() || input[range.raw_begin] != '<' || input[close_bracket] != '>') {
        iiXml::logging::log_parse_failure(
            "iiXml::parser::tag_parser::parse_all",
            "invalid opening markup before field parse",
            input,
            range.raw_begin
        );
        return std::nullopt;
    }

    const iiXml::elements::InlineProperties properties;
    const std::optional<std::vector<iiXml::elements::inline_property>> parsed =
        properties.parse(input.substr(range.raw_begin, range.value_begin - range.raw_begin), range.raw_begin);
    if (!parsed.has_value()) {
        iiXml::logging::log_parse_failure(
            "iiXml::parser::tag_parser::parse_all",
            "field parser rejected opening markup",
            input,
            range.raw_begin
        );
        return std::nullopt;
    }

    iiXml::logging::log_output_summary(
        "iiXml::parser::tag_parser::parse_all",
        "fields_parsed",
        std::string("field_count=") + std::to_string(parsed->size())
    );

    std::vector<tag_field> fields;
    fields.reserve(parsed->size());
    for (const iiXml::elements::inline_property& property : *parsed) {
        fields.push_back(tag_field{
            property.name,
            property.name_begin,
            property.name_end,
            property.has_value,
            property.value_begin,
            property.value_end,
            property.value_type,
            property.type_declared
        });
    }

    return fields;
}

std::optional<std::vector<tag_node>> build_hierarchy(
    std::string_view input,
    std::vector<iiXml::elements::open_tag_range>& ranges
) {
    std::vector<tag_node_entry> entries;
    entries.reserve(ranges.size());
    std::vector<std::size_t> roots;
    std::vector<std::size_t> active_nodes;

    for (iiXml::elements::open_tag_range& source : ranges) {
        iiXml::logging::log_parse_event(
            "iiXml::parser::tag_parser::parse_all",
            "node_range",
            input,
            source.raw_begin
        );

        tag_range range{
            std::move(source.tag_name),
            source.raw_begin,
            source.value_begin,
            source.value_end,
            source.raw_end
        };

        std::optional<std::vector<tag_field>> fields = parse_fields(input, range);
        if (!fields.has_value()) {
            return std::nullopt;
        }

        const std::size_t current_index = entries.size();
        entries.push_back(tag_node_entry{
            tag_node{std::move(range), std::move(*fields), {}},
            {}
        });

        while (!active_nodes.empty()
            && entries[active_nodes.back()].node.range.raw_end <= entries[current_index].node.range.raw_begin) {
            active_nodes.pop_back();
        }

        std::optional<std::size_t> parent_index;
        for (auto iterator = active_nodes.rbegin(); iterator != active_nodes.rend(); ++iterator) {
            if (contains_range(entries[*iterator].node.range, entries[current_index].node.range)) {
                parent_index = *iterator;
                break;
            }
        }

        if (parent_index.has_value()) {
            entries[*parent_index].children.push_back(current_index);
        } else {
            roots.push_back(current_index);
        }

        active_nodes.push_back(current_index);
    }

    std::function<tag_node(std::size_t)> materialize = [&](std::size_t index) {
        tag_node node = std::move(entries[index].node);
        node.children.reserve(entries[index].children.size());
        for (std::size_t child : entries[index].children) {
            node.children.push_back(materialize(child));
        }
        return node;
    };

    std::vector<tag_node> result;
    result.reserve(roots.size());
    for (std::size_t root : roots) {
        result.push_back(materialize(root));
    }

    return result;
}

} // namespace

tag_parser::tag_parser(QObject* parent)
    : QObject(parent) {
    qDebug() << "iiXml::parser::tag_parser::tag_parser constructed";
}

std::optional<tag_value> tag_parser::parse(std::string_view input) const {
    qDebug() << "iiXml::parser::tag_parser::parse begin"
             << "input_size=" << input.size();
    iiXml::logging::log_input_summary("iiXml::parser::tag_parser::parse", input);
    try {
        input = trim_outer(input);
        if (input.empty() || input.front() != '<') {
            iiXml::logging::log_parse_failure(
                "iiXml::parser::tag_parser::parse",
                "missing opening bracket",
                input,
                0
            );
            return std::nullopt;
        }

        const std::size_t opening_end = input.find('>');
        if (opening_end == std::string_view::npos) {
            iiXml::logging::log_parse_failure(
                "iiXml::parser::tag_parser::parse",
                "opening tag not closed",
                input,
                0
            );
            return std::nullopt;
        }
        iiXml::logging::log_parse_event(
            "iiXml::parser::tag_parser::parse",
            "opening_tag",
            input,
            0
        );

        const std::string_view tag_name = read_opening_tag_name(input.substr(1, opening_end - 1));
        if (!is_valid_tag_name(tag_name)) {
            iiXml::logging::log_parse_failure(
                "iiXml::parser::tag_parser::parse",
                "invalid tag name",
                input,
                1
            );
            return std::nullopt;
        }

        const std::string closing_tag = "</" + std::string(tag_name) + ">";
        if (input.size() < opening_end + 1 + closing_tag.size()) {
            iiXml::logging::log_parse_failure(
                "iiXml::parser::tag_parser::parse",
                "input shorter than closing tag",
                input,
                input.size()
            );
            return std::nullopt;
        }

        const std::size_t closing_start = input.size() - closing_tag.size();
        if (input.compare(closing_start, closing_tag.size(), closing_tag) != 0) {
            iiXml::logging::log_parse_failure(
                "iiXml::parser::tag_parser::parse",
                "closing tag mismatch",
                input,
                closing_start
            );
            return std::nullopt;
        }
        iiXml::logging::log_parse_event(
            "iiXml::parser::tag_parser::parse",
            "closing_tag",
            input,
            closing_start
        );

        const std::size_t value_start = opening_end + 1;
        const std::size_t value_size = closing_start - value_start;
        qDebug() << "iiXml::parser::tag_parser::parse parsed"
                 << "tag=" << QString::fromStdString(std::string(tag_name))
                 << "value_size=" << value_size;
        iiXml::logging::log_output_summary(
            "iiXml::parser::tag_parser::parse",
            "parsed",
            std::string("tag=") + std::string(tag_name)
                + " value_size=" + std::to_string(value_size)
                + " raw_size=" + std::to_string(input.size())
        );
        return tag_value{
            std::string(tag_name),
            std::string(input.substr(value_start, value_size)),
            std::string(input)
        };
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::parser::tag_parser::parse exception"
                 << "what=" << exception.what();
        return std::nullopt;
    } catch (...) {
        qDebug() << "iiXml::parser::tag_parser::parse exception"
                 << "what=unknown";
        return std::nullopt;
    }
}

std::optional<std::vector<tag_node>> tag_parser::parse_all(std::string_view input) const {
    qDebug() << "iiXml::parser::tag_parser::parse_all begin"
             << "input_size=" << input.size();
    iiXml::logging::log_input_summary("iiXml::parser::tag_parser::parse_all", input);
    try {
        const iiXml::elements::OpenTag open_tag;
        std::optional<std::vector<iiXml::elements::open_tag_range>> parsed =
            open_tag.parse_open_tags(input);
        if (!parsed.has_value()) {
            iiXml::logging::log_parse_failure(
                "iiXml::parser::tag_parser::parse_all",
                "open tag parser rejected input",
                input,
                0
            );
            return std::nullopt;
        }
        iiXml::logging::log_output_summary(
            "iiXml::parser::tag_parser::parse_all",
            "ranges_collected",
            std::string("range_count=") + std::to_string(parsed->size())
        );

        std::optional<std::vector<tag_node>> result = build_hierarchy(input, *parsed);
        if (!result.has_value()) {
            iiXml::logging::log_parse_failure(
                "iiXml::parser::tag_parser::parse_all",
                "hierarchy build failed",
                input,
                0
            );
            return std::nullopt;
        }

        qDebug() << "iiXml::parser::tag_parser::parse_all parsed"
                 << "root_count=" << result->size();
        iiXml::logging::log_output_summary(
            "iiXml::parser::tag_parser::parse_all",
            "parsed",
            std::string("root_count=") + std::to_string(result->size())
        );
        return result;
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::parser::tag_parser::parse_all exception"
                 << "what=" << exception.what();
        return std::nullopt;
    } catch (...) {
        qDebug() << "iiXml::parser::tag_parser::parse_all exception"
                 << "what=unknown";
        return std::nullopt;
    }
}

void tag_parser::parseTag(const QString& input) {
    qDebug() << "iiXml::parser::tag_parser::parseTag begin"
             << "input_size=" << input.size();
    try {
        const QByteArray utf8 = input.toUtf8();
        const std::string bytes(utf8.constData(), static_cast<std::size_t>(utf8.size()));
        iiXml::logging::log_input_summary(
            "iiXml::parser::tag_parser::parseTag",
            std::string_view(bytes.data(), bytes.size())
        );
        const std::optional<tag_value> parsed = parse(std::string_view(bytes.data(), bytes.size()));

        if (!parsed.has_value()) {
            qDebug() << "iiXml::parser::tag_parser::parseTag failed"
                     << "reason=tag parse failed";
            emit parseFailed("tag parse failed");
            return;
        }

        qDebug() << "iiXml::parser::tag_parser::parseTag parsed"
                 << "tag=" << QString::fromStdString(parsed->tag_name)
                 << "value_size=" << parsed->value.size();
        iiXml::logging::log_output_summary(
            "iiXml::parser::tag_parser::parseTag",
            "parsed",
            std::string("tag=") + parsed->tag_name
                + " value_size=" + std::to_string(parsed->value.size())
        );
        emit tagParsed(QString::fromStdString(parsed->tag_name), QString::fromStdString(parsed->value));
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::parser::tag_parser::parseTag exception"
                 << "what=" << exception.what();
        emit parseFailed(QString::fromStdString(
            std::string("tag parse exception: ") + exception.what()
        ));
    } catch (...) {
        qDebug() << "iiXml::parser::tag_parser::parseTag exception"
                 << "what=unknown";
        emit parseFailed("tag parse exception: unknown");
    }
}

} // namespace iiXml::parser
