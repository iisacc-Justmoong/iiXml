#include "XmlLog.h"

#include <QDebug>
#include <QString>

#include <algorithm>

namespace iiXml::logging {

source_position locate_source_position(
    std::string_view input,
    std::size_t offset
) {
    offset = std::min(offset, input.size());

    source_position position{offset, 1, 1};
    for (std::size_t index = 0; index < offset; ++index) {
        if (input[index] == '\n') {
            ++position.line;
            position.column = 1;
        } else {
            ++position.column;
        }
    }

    return position;
}

std::string source_context(
    std::string_view input,
    std::size_t offset,
    std::size_t radius
) {
    offset = std::min(offset, input.size());
    const std::size_t begin = offset > radius ? offset - radius : 0;
    const std::size_t end = std::min(input.size(), offset + radius);

    std::string context;
    if (begin > 0) {
        context += "...";
    }

    for (char value : input.substr(begin, end - begin)) {
        switch (value) {
            case '\n':
                context += "\\n";
                break;
            case '\r':
                context += "\\r";
                break;
            case '\t':
                context += "\\t";
                break;
            default:
                context += value;
                break;
        }
    }

    if (end < input.size()) {
        context += "...";
    }

    return context;
}

std::size_t count_lines(std::string_view input) {
    if (input.empty()) {
        return 0;
    }

    return static_cast<std::size_t>(std::count(input.begin(), input.end(), '\n')) + 1;
}

void log_input_summary(
    const char* scope,
    std::string_view input
) {
    qDebug().noquote() << scope
                       << "input"
                       << "input_size=" << input.size()
                       << "line_count=" << count_lines(input)
                       << "preview=" << QString::fromStdString(source_context(input, 0, 48));
}

void log_parse_event(
    const char* scope,
    std::string_view event,
    std::string_view input,
    std::size_t offset
) {
    const source_position position = locate_source_position(input, offset);
    qDebug().noquote() << scope
                       << "parsing"
                       << "event=" << QString::fromStdString(std::string(event))
                       << "offset=" << position.offset
                       << "line=" << position.line
                       << "column=" << position.column
                       << "context=" << QString::fromStdString(source_context(input, offset));
}

void log_output_summary(
    const char* scope,
    std::string_view status,
    std::string_view summary
) {
    qDebug().noquote() << scope
                       << "output"
                       << "status=" << QString::fromStdString(std::string(status))
                       << "summary=" << QString::fromStdString(std::string(summary));
}

void log_parse_failure(
    const char* scope,
    std::string_view reason,
    std::string_view input,
    std::size_t offset
) {
    const source_position position = locate_source_position(input, offset);
    qDebug().noquote() << scope
                       << "failed"
                       << "reason=" << QString::fromStdString(std::string(reason))
                       << "offset=" << position.offset
                       << "line=" << position.line
                       << "column=" << position.column
                       << "context=" << QString::fromStdString(source_context(input, offset));
}

} // namespace iiXml::logging
