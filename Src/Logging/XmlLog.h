#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace iiXml::logging {

struct source_position {
    std::size_t offset;
    std::size_t line;
    std::size_t column;
};

[[nodiscard]] source_position locate_source_position(
    std::string_view input,
    std::size_t offset
);

[[nodiscard]] std::string source_context(
    std::string_view input,
    std::size_t offset,
    std::size_t radius = 32
);

void log_input_summary(
    const char* scope,
    std::string_view input
);

void log_parse_event(
    const char* scope,
    std::string_view event,
    std::string_view input,
    std::size_t offset
);

void log_output_summary(
    const char* scope,
    std::string_view status,
    std::string_view summary
);

void log_parse_failure(
    const char* scope,
    std::string_view reason,
    std::string_view input,
    std::size_t offset
);

} // namespace iiXml::logging
