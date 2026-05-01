#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace iiXml::Logging {

struct SourcePosition {
    std::size_t Offset;
    std::size_t Line;
    std::size_t Column;
};

[[nodiscard]] SourcePosition LocateSourcePosition(
    std::string_view Input,
    std::size_t Offset
);

[[nodiscard]] std::string SourceContext(
    std::string_view Input,
    std::size_t Offset,
    std::size_t Radius = 32
);

void LogInputSummary(
    const char* Scope,
    std::string_view Input
);

void LogParseEvent(
    const char* Scope,
    std::string_view Event,
    std::string_view Input,
    std::size_t Offset
);

void LogOutputSummary(
    const char* Scope,
    std::string_view Status,
    std::string_view Summary
);

void LogParseFailure(
    const char* Scope,
    std::string_view Reason,
    std::string_view Input,
    std::size_t Offset
);

} // namespace iiXml::Logging
