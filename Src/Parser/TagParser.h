#ifndef IIXML_PARSER_TAG_PARSER_H
#define IIXML_PARSER_TAG_PARSER_H

#include <optional>
#include <string>
#include <string_view>

namespace iixml::parser {

struct tag_value {
    std::string tag_name;
    std::string value;
};

class tag_parser {
public:
    [[nodiscard]] std::optional<tag_value> parse(std::string_view input) const;
};

} // namespace iixml::parser

#endif // IIXML_PARSER_TAG_PARSER_H
