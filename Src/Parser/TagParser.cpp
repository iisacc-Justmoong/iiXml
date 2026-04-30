#include "TagParser.h"

#include "Src/Elements/OpenTag.h"

#include <QByteArray>
#include <QDebug>
#include <QString>

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

tag_parser::tag_parser(QObject* parent)
    : QObject(parent) {
    qDebug() << "iiXml::parser::tag_parser::tag_parser constructed";
}

std::optional<tag_value> tag_parser::parse(std::string_view input) const {
    qDebug() << "iiXml::parser::tag_parser::parse begin"
             << "input_size=" << input.size();
    try {
        input = trim_outer(input);
        if (input.empty() || input.front() != '<') {
            qDebug() << "iiXml::parser::tag_parser::parse failed"
                     << "reason=missing opening bracket";
            return std::nullopt;
        }

        const std::size_t opening_end = input.find('>');
        if (opening_end == std::string_view::npos) {
            qDebug() << "iiXml::parser::tag_parser::parse failed"
                     << "reason=opening tag not closed";
            return std::nullopt;
        }

        const std::string_view tag_name = read_opening_tag_name(input.substr(1, opening_end - 1));
        if (!is_valid_tag_name(tag_name)) {
            qDebug() << "iiXml::parser::tag_parser::parse failed"
                     << "reason=invalid tag name";
            return std::nullopt;
        }

        const std::string closing_tag = "</" + std::string(tag_name) + ">";
        if (input.size() < opening_end + 1 + closing_tag.size()) {
            qDebug() << "iiXml::parser::tag_parser::parse failed"
                     << "reason=input shorter than closing tag";
            return std::nullopt;
        }

        const std::size_t closing_start = input.size() - closing_tag.size();
        if (input.compare(closing_start, closing_tag.size(), closing_tag) != 0) {
            qDebug() << "iiXml::parser::tag_parser::parse failed"
                     << "reason=closing tag mismatch";
            return std::nullopt;
        }

        const std::size_t value_start = opening_end + 1;
        const std::size_t value_size = closing_start - value_start;
        qDebug() << "iiXml::parser::tag_parser::parse parsed"
                 << "tag=" << QString::fromStdString(std::string(tag_name))
                 << "value_size=" << value_size;
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

std::optional<std::vector<tag_value>> tag_parser::parse_all(std::string_view input) const {
    qDebug() << "iiXml::parser::tag_parser::parse_all begin"
             << "input_size=" << input.size();
    try {
        const iiXml::elements::OpenTag open_tag;
        const std::optional<std::vector<iiXml::elements::open_tag_value>> parsed =
            open_tag.parse_open_tags(input);
        if (!parsed.has_value()) {
            qDebug() << "iiXml::parser::tag_parser::parse_all failed"
                     << "reason=open tag parser rejected input";
            return std::nullopt;
        }

        std::vector<tag_value> result;
        result.reserve(parsed->size());
        for (const iiXml::elements::open_tag_value& tag : *parsed) {
            result.push_back(tag_value{
                tag.tag_name,
                tag.value,
                tag.raw
            });
        }

        qDebug() << "iiXml::parser::tag_parser::parse_all parsed"
                 << "tag_count=" << result.size();
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
