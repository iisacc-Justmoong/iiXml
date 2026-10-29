#include "XmlLog.h"

#include <QDebug>
#include <QString>

#include <algorithm>

namespace iiXml::Logging {

SourcePosition LocateSourcePosition(
    std::string_view input,
    std::size_t offset
) {
    offset = std::min(offset, input.size());

    SourcePosition position{offset, 1, 1};
    for (std::size_t index = 0; index < offset; ++index) {
        if (input[index] == '\n') {
            ++position.Line;
            position.Column = 1;
        } else {
            ++position.Column;
        }
    }

    return position;
}

std::string SourceContext(
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

void LogInputSummary(
    const char* scope,
    std::string_view input
) {
    qDebug().noquote() << scope
                       << "input"
                       << "input_size=" << input.size()
                       << "line_count=" << count_lines(input)
                       << "preview=" << QString::fromStdString(SourceContext(input, 0, 48));
}

void LogParseEvent(
    const char* scope,
    std::string_view event,
    std::string_view input,
    std::size_t offset
) {
    const SourcePosition position = LocateSourcePosition(input, offset);
    qDebug().noquote() << scope
                       << "parsing"
                       << "event=" << QString::fromStdString(std::string(event))
                       << "offset=" << position.Offset
                       << "line=" << position.Line
                       << "column=" << position.Column
                       << "context=" << QString::fromStdString(SourceContext(input, offset));
}

void LogOutputSummary(
    const char* scope,
    std::string_view status,
    std::string_view summary
) {
    qDebug().noquote() << scope
                       << "output"
                       << "status=" << QString::fromStdString(std::string(status))
                       << "summary=" << QString::fromStdString(std::string(summary));
}

void LogParseFailure(
    const char* scope,
    std::string_view reason,
    std::string_view input,
    std::size_t offset
) {
    const SourcePosition position = LocateSourcePosition(input, offset);
    qDebug().noquote() << scope
                       << "failed"
                       << "reason=" << QString::fromStdString(std::string(reason))
                       << "offset=" << position.Offset
                       << "line=" << position.Line
                       << "column=" << position.Column
                       << "context=" << QString::fromStdString(SourceContext(input, offset));
}

} // namespace iiXml::Logging
