#include "iiXml.h"

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

std::string_view value_view(
    std::string_view input,
    const iiXml::elements::inline_property& property
) {
    return input.substr(property.value_begin, property.value_end - property.value_begin);
}

void parses_multiple_literal_properties() {
    const iiXml::elements::InlineProperties properties;
    const std::string input =
        "<resource title=\"hello\" quoted_number=\"42\" quoted_bool=\"true\" count=42 "
        "opacity=0.75 enabled=true visible=false>";

    const std::optional<std::vector<iiXml::elements::inline_property>> parsed =
        properties.parse(input);

    expect(parsed.has_value(), "multiple literal properties should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 7, "resource should expose seven properties");
    if (parsed->size() != 7) {
        return;
    }

    expect((*parsed)[0].name == "title", "first property should be title");
    expect(value_view(input, (*parsed)[0]) == "hello", "title value should be hello");
    expect((*parsed)[0].value_type == iiXml::elements::inline_property_type::string_type,
        "quoted title property should infer string type");
    expect(!(*parsed)[0].type_declared, "title property should not mark declared type");

    expect((*parsed)[1].name == "quoted_number", "second property should be quoted_number");
    expect(value_view(input, (*parsed)[1]) == "42", "quoted_number value should be 42");
    expect((*parsed)[1].value_type == iiXml::elements::inline_property_type::string_type,
        "quoted number should remain string type");
    expect(!(*parsed)[1].type_declared,
        "quoted_number property should not mark declared type");

    expect((*parsed)[2].name == "quoted_bool", "third property should be quoted_bool");
    expect(value_view(input, (*parsed)[2]) == "true", "quoted_bool value should be true");
    expect((*parsed)[2].value_type == iiXml::elements::inline_property_type::string_type,
        "quoted boolean-looking value should remain string type");
    expect(!(*parsed)[2].type_declared,
        "quoted_bool property should not mark declared type");

    expect((*parsed)[3].name == "count", "fourth property should be count");
    expect(value_view(input, (*parsed)[3]) == "42", "count value should be 42");
    expect((*parsed)[3].value_type == iiXml::elements::inline_property_type::int_type,
        "unquoted count should infer int type");
    expect(!(*parsed)[3].type_declared, "count property should not mark declared type");

    expect((*parsed)[4].name == "opacity", "fifth property should be opacity");
    expect(value_view(input, (*parsed)[4]) == "0.75", "opacity value should be 0.75");
    expect((*parsed)[4].value_type == iiXml::elements::inline_property_type::float_type,
        "unquoted opacity should infer float type");
    expect(!(*parsed)[4].type_declared, "opacity property should not mark declared type");

    expect((*parsed)[5].name == "enabled", "sixth property should be enabled");
    expect(value_view(input, (*parsed)[5]) == "true", "enabled value should be true");
    expect((*parsed)[5].value_type == iiXml::elements::inline_property_type::bool_type,
        "enabled property should infer bool type");
    expect(!(*parsed)[5].type_declared, "enabled property should not mark declared type");

    expect((*parsed)[6].name == "visible", "seventh property should be visible");
    expect(value_view(input, (*parsed)[6]) == "false", "visible value should be false");
    expect((*parsed)[6].value_type == iiXml::elements::inline_property_type::bool_type,
        "visible property should infer bool type");
    expect(!(*parsed)[6].type_declared, "visible property should not mark declared type");
}

void parses_flag_and_unquoted_properties() {
    const iiXml::elements::InlineProperties properties;
    const std::string input = "<resource selected count=7 ratio=1.5 enabled=false>";

    const std::optional<std::vector<iiXml::elements::inline_property>> parsed =
        properties.parse(input);

    expect(parsed.has_value(), "flag and unquoted properties should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 4, "resource should expose four properties");
    if (parsed->size() != 4) {
        return;
    }

    expect((*parsed)[0].name == "selected", "first property should be selected");
    expect(!(*parsed)[0].has_value, "selected should be a flag property");
    expect((*parsed)[0].value_type == iiXml::elements::inline_property_type::string_type,
        "flag property should default to string type");

    expect((*parsed)[1].name == "count", "second property should be count");
    expect(value_view(input, (*parsed)[1]) == "7", "count value should be 7");
    expect((*parsed)[1].value_type == iiXml::elements::inline_property_type::int_type,
        "unquoted count should infer int type");

    expect((*parsed)[2].name == "ratio", "third property should be ratio");
    expect(value_view(input, (*parsed)[2]) == "1.5", "ratio value should be 1.5");
    expect((*parsed)[2].value_type == iiXml::elements::inline_property_type::float_type,
        "unquoted ratio should infer float type");

    expect((*parsed)[3].name == "enabled", "fourth property should be enabled");
    expect(value_view(input, (*parsed)[3]) == "false", "enabled value should be false");
    expect((*parsed)[3].value_type == iiXml::elements::inline_property_type::bool_type,
        "unquoted enabled should infer bool type");
}

void rejects_unclosed_quoted_property() {
    const iiXml::elements::InlineProperties properties;

    const std::optional<std::vector<iiXml::elements::inline_property>> parsed =
        properties.parse("<resource title=\"hello>");

    expect(!parsed.has_value(), "unclosed quoted property should fail");
}

} // namespace

int main() {
    parses_multiple_literal_properties();
    parses_flag_and_unquoted_properties();
    rejects_unclosed_quoted_property();

    return failures == 0 ? 0 : 1;
}
