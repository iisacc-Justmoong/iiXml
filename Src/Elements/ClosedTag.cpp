#include "ClosedTag.h"

#include "Src/Logging/XmlLog.h"

#include <QByteArray>
#include <QDebug>
#include <QString>

#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

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

std::size_t first_non_space(std::string_view input) {
    std::size_t begin = 0;
    while (begin < input.size() && is_space(input[begin])) {
        ++begin;
    }

    return begin;
}

struct closed_tag_scan {
    std::optional<iiXml::Elements::ClosedTagMatch> Match;
    std::string Reason;
    std::size_t Offset;
};

std::optional<std::size_t> find_markup_end(std::string_view input, std::size_t begin) {
    const std::size_t end = input.find('>', begin);
    if (end == std::string_view::npos) {
        return std::nullopt;
    }

    return end;
}

std::optional<std::string> read_tag_name(
    std::string_view input,
    std::size_t begin,
    std::size_t& end
) {
    if (begin >= input.size() || !is_name_start(input[begin])) {
        return std::nullopt;
    }

    end = begin + 1;
    while (end < input.size() && is_name_char(input[end])) {
        ++end;
    }

    return std::string(input.substr(begin, end - begin));
}

bool contains_only_space(std::string_view input, std::size_t begin, std::size_t end) {
    for (std::size_t index = begin; index < end; ++index) {
        if (!is_space(input[index])) {
            return false;
        }
    }

    return true;
}

closed_tag_scan scan_immediate_closed_tag(
    std::string_view input,
    std::size_t source_offset
) {
    const std::size_t begin = first_non_space(input);
    if (begin >= input.size()) {
        return closed_tag_scan{std::nullopt, "input is empty", source_offset + begin};
    }

    if (begin + 2 > input.size() || input[begin] != '<' || input[begin + 1] != '/') {
        return closed_tag_scan{
            std::nullopt,
            "input does not start with a closed tag",
            source_offset + begin
        };
    }

    const std::optional<std::size_t> tag_end = find_markup_end(input, begin + 2);
    if (!tag_end.has_value()) {
        return closed_tag_scan{
            std::nullopt,
            "closed tag not terminated",
            source_offset + begin
        };
    }

    std::size_t name_end = begin + 2;
    std::optional<std::string> tag_name = read_tag_name(input, begin + 2, name_end);
    if (!tag_name.has_value()) {
        return closed_tag_scan{
            std::nullopt,
            "invalid closed tag name",
            source_offset + begin + 2
        };
    }

    if (!contains_only_space(input, name_end, *tag_end)) {
        return closed_tag_scan{
            std::nullopt,
            "closed tag contains unexpected content",
            source_offset + name_end
        };
    }

    const std::string raw(input.substr(begin, *tag_end - begin + 1));
    iiXml::Elements::ClosedTagMatch match{
        std::move(*tag_name),
        raw,
        source_offset + begin,
        source_offset + *tag_end + 1,
        true
    };

    return closed_tag_scan{std::move(match), {}, source_offset + begin};
}

std::string to_utf8_string(const QString& value) {
    const QByteArray utf8 = value.toUtf8();
    return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

} // namespace

namespace iiXml::Elements {

ClosedTag::ClosedTag(QObject* parent)
    : QObject(parent) {
    qDebug() << "iiXml::Elements::ClosedTag::ClosedTag constructed";
}

bool ClosedTag::IsImmediateClosedTag(std::string_view input) const {
    qDebug() << "iiXml::Elements::ClosedTag::IsImmediateClosedTag begin"
             << "input_size=" << input.size();
    iiXml::Logging::LogInputSummary(
        "iiXml::Elements::ClosedTag::IsImmediateClosedTag",
        input
    );
    try {
        const closed_tag_scan scanned = scan_immediate_closed_tag(input, 0);
        if (!scanned.Match.has_value()) {
            qDebug() << "iiXml::Elements::ClosedTag::IsImmediateClosedTag rejected"
                     << "reason=" << QString::fromStdString(scanned.Reason);
            iiXml::Logging::LogParseFailure(
                "iiXml::Elements::ClosedTag::IsImmediateClosedTag",
                scanned.Reason,
                input,
                scanned.Offset
            );
            return false;
        }

        qDebug() << "iiXml::Elements::ClosedTag::IsImmediateClosedTag matched"
                 << "tag=" << QString::fromStdString(scanned.Match->TagName);
        iiXml::Logging::LogOutputSummary(
            "iiXml::Elements::ClosedTag::IsImmediateClosedTag",
            "matched",
            std::string("tag=") + scanned.Match->TagName
        );
        return true;
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Elements::ClosedTag::IsImmediateClosedTag exception"
                 << "what=" << exception.what();
        return false;
    } catch (...) {
        qDebug() << "iiXml::Elements::ClosedTag::IsImmediateClosedTag exception"
                 << "what=unknown";
        return false;
    }
}

std::optional<ClosedTagMatch> ClosedTag::MatchImmediate(
    std::string_view input,
    std::size_t source_offset
) const {
    qDebug() << "iiXml::Elements::ClosedTag::MatchImmediate begin"
             << "input_size=" << input.size()
             << "source_offset=" << source_offset;
    iiXml::Logging::LogInputSummary(
        "iiXml::Elements::ClosedTag::MatchImmediate",
        input
    );
    try {
        closed_tag_scan scanned = scan_immediate_closed_tag(input, source_offset);
        if (!scanned.Match.has_value()) {
            qDebug() << "iiXml::Elements::ClosedTag::MatchImmediate rejected"
                     << "reason=" << QString::fromStdString(scanned.Reason);
            iiXml::Logging::LogParseFailure(
                "iiXml::Elements::ClosedTag::MatchImmediate",
                scanned.Reason,
                input,
                scanned.Offset >= source_offset ? scanned.Offset - source_offset : 0
            );
            return std::nullopt;
        }

        qDebug() << "iiXml::Elements::ClosedTag::MatchImmediate matched"
                 << "tag=" << QString::fromStdString(scanned.Match->TagName)
                 << "raw_begin=" << scanned.Match->RawBegin
                 << "raw_end=" << scanned.Match->RawEnd;
        iiXml::Logging::LogParseEvent(
            "iiXml::Elements::ClosedTag::MatchImmediate",
            std::string("closed_tag name=") + scanned.Match->TagName,
            input,
            scanned.Match->RawBegin >= source_offset
                ? scanned.Match->RawBegin - source_offset
                : 0
        );
        iiXml::Logging::LogOutputSummary(
            "iiXml::Elements::ClosedTag::MatchImmediate",
            "matched",
            std::string("tag=") + scanned.Match->TagName
                + " raw_size=" + std::to_string(scanned.Match->Raw.size())
        );
        return std::move(scanned.Match);
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Elements::ClosedTag::MatchImmediate exception"
                 << "what=" << exception.what();
        return std::nullopt;
    } catch (...) {
        qDebug() << "iiXml::Elements::ClosedTag::MatchImmediate exception"
                 << "what=unknown";
        return std::nullopt;
    }
}

void ClosedTag::ParseClosedTag(const QString& input) {
    qDebug() << "iiXml::Elements::ClosedTag::ParseClosedTag begin"
             << "input_size=" << input.size();
    try {
        const std::string bytes = to_utf8_string(input);
        iiXml::Logging::LogInputSummary(
            "iiXml::Elements::ClosedTag::ParseClosedTag",
            std::string_view(bytes.data(), bytes.size())
        );

        const closed_tag_scan scanned =
            scan_immediate_closed_tag(std::string_view(bytes.data(), bytes.size()), 0);
        if (!scanned.Match.has_value()) {
            qDebug() << "iiXml::Elements::ClosedTag::ParseClosedTag rejected"
                     << "reason=" << QString::fromStdString(scanned.Reason);
            iiXml::Logging::LogParseFailure(
                "iiXml::Elements::ClosedTag::ParseClosedTag",
                scanned.Reason,
                bytes,
                scanned.Offset
            );
            emit ClosedTagFlagged(false);
            emit ClosedTagRejected(QString::fromStdString(scanned.Reason));
            return;
        }

        qDebug() << "iiXml::Elements::ClosedTag::ParseClosedTag flagged"
                 << "tag=" << QString::fromStdString(scanned.Match->TagName)
                 << "raw=" << QString::fromStdString(scanned.Match->Raw);
        iiXml::Logging::LogOutputSummary(
            "iiXml::Elements::ClosedTag::ParseClosedTag",
            "flagged",
            std::string("tag=") + scanned.Match->TagName
        );
        emit ClosedTagFlagged(scanned.Match->Flag);
        emit ClosedTagParsed(
            QString::fromStdString(scanned.Match->TagName),
            QString::fromStdString(scanned.Match->Raw)
        );
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Elements::ClosedTag::ParseClosedTag exception"
                 << "what=" << exception.what();
        emit ClosedTagFlagged(false);
        emit ClosedTagRejected(QString::fromStdString(
            std::string("closed tag parse exception: ") + exception.what()
        ));
    } catch (...) {
        qDebug() << "iiXml::Elements::ClosedTag::ParseClosedTag exception"
                 << "what=unknown";
        emit ClosedTagFlagged(false);
        emit ClosedTagRejected("closed tag parse exception: unknown");
    }
}

} // namespace iiXml::Elements
