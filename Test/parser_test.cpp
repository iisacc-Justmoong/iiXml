#include "iiXml.h"

#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

void parses_basic_tag() {
    const iiXml::parser::tag_parser parser;

    const std::optional<iiXml::parser::tag_value> parsed = parser.parse("<number>123</number>");

    expect(parsed.has_value(), "number tag should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->tag_name == "number", "tag name should be number");
    expect(parsed->value == "123", "tag value should be 123");
}

void parses_utf8_value() {
    const iiXml::parser::tag_parser parser;

    const std::optional<iiXml::parser::tag_value> parsed = parser.parse("<number>숫자</number>");

    expect(parsed.has_value(), "utf8 value tag should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->tag_name == "number", "utf8 value tag name should be number");
    expect(parsed->value == "숫자", "utf8 value should be preserved");
}

void parses_opening_tag_with_attributes() {
    const iiXml::parser::tag_parser parser;

    const std::optional<iiXml::parser::tag_value> parsed =
        parser.parse("<number type=\"int\">42</number>");

    expect(parsed.has_value(), "tag with attributes should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->tag_name == "number", "tag name should ignore attributes");
    expect(parsed->value == "42", "tag value with attributes should be preserved");
}

void preserves_inner_whitespace() {
    const iiXml::parser::tag_parser parser;

    const std::optional<iiXml::parser::tag_value> parsed = parser.parse(" \n<number> 123 </number>\t");

    expect(parsed.has_value(), "outer whitespace should be ignored");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->value == " 123 ", "inner whitespace should be preserved");
}

void parses_cross_nested_tags_as_independent_values() {
    const iiXml::parser::tag_parser parser;
    const std::string input = "<a>\n    <b>\n</a>\n    </b>";

    const std::optional<std::vector<iiXml::parser::tag_value>> parsed = parser.parse_all(input);

    expect(parsed.has_value(), "cross nested tags should parse as multiple values");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 2, "cross nested parse should keep both a and b");
    if (parsed->size() != 2) {
        return;
    }

    expect((*parsed)[0].tag_name == "a", "first parsed tag should be a");
    expect((*parsed)[0].value == "\n    <b>\n", "a parsed value should keep b opening markup");
    expect((*parsed)[0].raw == "<a>\n    <b>\n</a>", "a raw span should be preserved");

    expect((*parsed)[1].tag_name == "b", "second parsed tag should be b");
    expect((*parsed)[1].value == "\n</a>\n    ", "b parsed value should keep a closing markup");
    expect((*parsed)[1].raw == "<b>\n</a>\n    </b>", "b raw span should be preserved");
}

void parses_many_cross_nested_tags_as_independent_values() {
    const iiXml::parser::tag_parser parser;
    const std::string input = "<p><bold><italic>text</p><p>really</bold> useful</italic></p>";

    const std::optional<std::vector<iiXml::parser::tag_value>> parsed = parser.parse_all(input);

    expect(parsed.has_value(), "many cross nested tags should parse as multiple values");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 4, "many cross nested parse should keep every tag");
    if (parsed->size() != 4) {
        return;
    }

    expect((*parsed)[0].tag_name == "p", "first parsed tag should be first p");
    expect((*parsed)[0].value == "<bold><italic>text",
        "first p value should be fixed only at its matching close tag");
    expect((*parsed)[0].raw == "<p><bold><italic>text</p>",
        "first p raw span should be preserved");

    expect((*parsed)[1].tag_name == "bold", "second parsed tag should be bold");
    expect((*parsed)[1].value == "<italic>text</p><p>really",
        "bold value should be fixed only at its matching close tag");
    expect((*parsed)[1].raw == "<bold><italic>text</p><p>really</bold>",
        "bold raw span should cross paragraph tags");

    expect((*parsed)[2].tag_name == "italic", "third parsed tag should be italic");
    expect((*parsed)[2].value == "text</p><p>really</bold> useful",
        "italic value should be fixed only at its matching close tag");
    expect((*parsed)[2].raw == "<italic>text</p><p>really</bold> useful</italic>",
        "italic raw span should be preserved");

    expect((*parsed)[3].tag_name == "p", "fourth parsed tag should be second p");
    expect((*parsed)[3].value == "really</bold> useful</italic>",
        "second p value should be fixed only at its matching close tag");
    expect((*parsed)[3].raw == "<p>really</bold> useful</italic></p>",
        "second p raw span should be preserved");
}

void rejects_missing_close_tag() {
    const iiXml::parser::tag_parser parser;

    expect(!parser.parse("<number>123").has_value(), "missing close tag should fail");
}

void rejects_mismatched_close_tag() {
    const iiXml::parser::tag_parser parser;

    expect(!parser.parse("<number>123</text>").has_value(), "mismatched close tag should fail");
}

void rejects_invalid_tag_name() {
    const iiXml::parser::tag_parser parser;

    expect(!parser.parse("<1number>123</1number>").has_value(), "invalid tag name should fail");
}

} // namespace

int main() {
    parses_basic_tag();
    parses_utf8_value();
    parses_opening_tag_with_attributes();
    preserves_inner_whitespace();
    parses_cross_nested_tags_as_independent_values();
    parses_many_cross_nested_tags_as_independent_values();
    rejects_missing_close_tag();
    rejects_mismatched_close_tag();
    rejects_invalid_tag_name();

    return failures == 0 ? 0 : 1;
}
