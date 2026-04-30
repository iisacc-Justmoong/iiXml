#pragma once

#include <string_view>

namespace iiXml::writer {

enum class validation_exit {
    valid,
    invalid_xml_file,
    invalid_tag_closure
};

class InputValidator {
public:
    [[nodiscard]] validation_exit validate(std::string_view input) const;
    [[nodiscard]] bool has_valid_tag_closure(std::string_view input) const;
};

} // namespace iiXml::writer
