#include "iiXml.h"

#include <iostream>
#include <optional>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

void parses_basic_tag() {
    const iixml::parser::tag_parser parser;

    const std::optional<iixml::parser::tag_value> parsed = parser.parse("<number>123</number>");

    expect(parsed.has_value(), "number tag should parse");
    expect(parsed->tag_name == "number", "tag name should be number");
    expect(parsed->value == "123", "tag value should be 123");
}

void parses_utf8_value() {
    const iixml::parser::tag_parser parser;

    const std::optional<iixml::parser::tag_value> parsed = parser.parse("<number>숫자</number>");

    expect(parsed.has_value(), "utf8 value tag should parse");
    expect(parsed->tag_name == "number", "utf8 value tag name should be number");
    expect(parsed->value == "숫자", "utf8 value should be preserved");
}

void preserves_inner_whitespace() {
    const iixml::parser::tag_parser parser;

    const std::optional<iixml::parser::tag_value> parsed = parser.parse(" \n<number> 123 </number>\t");

    expect(parsed.has_value(), "outer whitespace should be ignored");
    expect(parsed->value == " 123 ", "inner whitespace should be preserved");
}

void rejects_missing_close_tag() {
    const iixml::parser::tag_parser parser;

    expect(!parser.parse("<number>123").has_value(), "missing close tag should fail");
}

void rejects_mismatched_close_tag() {
    const iixml::parser::tag_parser parser;

    expect(!parser.parse("<number>123</text>").has_value(), "mismatched close tag should fail");
}

void rejects_invalid_tag_name() {
    const iixml::parser::tag_parser parser;

    expect(!parser.parse("<1number>123</1number>").has_value(), "invalid tag name should fail");
}

} // namespace

int main() {
    parses_basic_tag();
    parses_utf8_value();
    preserves_inner_whitespace();
    rejects_missing_close_tag();
    rejects_mismatched_close_tag();
    rejects_invalid_tag_name();

    return failures == 0 ? 0 : 1;
}
