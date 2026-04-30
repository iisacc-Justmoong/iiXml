#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace iiXml::elements {

enum class doctype_kind {
    xml_declaration,
    doctype_declaration
};

struct doctype_match {
    doctype_kind kind;
    std::string raw;
};

class DOCTYPE {
public:
    [[nodiscard]] std::optional<doctype_match> match_top(std::string_view input) const;
    [[nodiscard]] bool is_top_doctype(std::string_view input) const;
    [[nodiscard]] bool is_top_xml_declaration(std::string_view input) const;
};

} // namespace iiXml::elements
