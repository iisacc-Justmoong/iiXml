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

std::string_view raw_view(std::string_view input, const iiXml::parser::tag_range& range) {
    return input.substr(range.raw_begin, range.raw_end - range.raw_begin);
}

std::string_view value_view(std::string_view input, const iiXml::parser::tag_range& range) {
    return input.substr(range.value_begin, range.value_end - range.value_begin);
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

    const std::optional<std::vector<iiXml::parser::tag_range>> parsed = parser.parse_all(input);

    expect(parsed.has_value(), "cross nested tags should parse as multiple values");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 2, "cross nested parse should keep both a and b");
    if (parsed->size() != 2) {
        return;
    }

    expect((*parsed)[0].tag_name == "a", "first parsed tag should be a");
    expect(value_view(input, (*parsed)[0]) == "\n    <b>\n",
        "a parsed value range should keep b opening markup");
    expect(raw_view(input, (*parsed)[0]) == "<a>\n    <b>\n</a>",
        "a raw range should be preserved");

    expect((*parsed)[1].tag_name == "b", "second parsed tag should be b");
    expect(value_view(input, (*parsed)[1]) == "\n</a>\n    ",
        "b parsed value range should keep a closing markup");
    expect(raw_view(input, (*parsed)[1]) == "<b>\n</a>\n    </b>",
        "b raw range should be preserved");
}

void parses_many_cross_nested_tags_as_independent_values() {
    const iiXml::parser::tag_parser parser;
    const std::string input = "<p><bold><italic>text</p><p>really</bold> useful</italic></p>";

    const std::optional<std::vector<iiXml::parser::tag_range>> parsed = parser.parse_all(input);

    expect(parsed.has_value(), "many cross nested tags should parse as multiple values");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 4, "many cross nested parse should keep every tag");
    if (parsed->size() != 4) {
        return;
    }

    expect((*parsed)[0].tag_name == "p", "first parsed tag should be first p");
    expect(value_view(input, (*parsed)[0]) == "<bold><italic>text",
        "first p value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[0]) == "<p><bold><italic>text</p>",
        "first p raw range should be preserved");

    expect((*parsed)[1].tag_name == "bold", "second parsed tag should be bold");
    expect(value_view(input, (*parsed)[1]) == "<italic>text</p><p>really",
        "bold value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[1]) == "<bold><italic>text</p><p>really</bold>",
        "bold raw range should cross paragraph tags");

    expect((*parsed)[2].tag_name == "italic", "third parsed tag should be italic");
    expect(value_view(input, (*parsed)[2]) == "text</p><p>really</bold> useful",
        "italic value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[2]) == "<italic>text</p><p>really</bold> useful</italic>",
        "italic raw range should be preserved");

    expect((*parsed)[3].tag_name == "p", "fourth parsed tag should be second p");
    expect(value_view(input, (*parsed)[3]) == "really</bold> useful</italic>",
        "second p value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[3]) == "<p>really</bold> useful</italic></p>",
        "second p raw range should be preserved");
}

void parses_repeated_tag_names_by_latest_open_tag() {
    const iiXml::parser::tag_parser parser;
    const std::string input = "<p><p>inner</p>outer</p>";

    const std::optional<std::vector<iiXml::parser::tag_range>> parsed =
        parser.parse_all(input);

    expect(parsed.has_value(), "repeated tag names should parse as multiple values");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 2, "repeated tag name parse should keep both p tags");
    if (parsed->size() != 2) {
        return;
    }

    expect((*parsed)[0].tag_name == "p", "outer p should remain first by open order");
    expect(value_view(input, (*parsed)[0]) == "<p>inner</p>outer",
        "outer p value range should stay alive until its own close tag");
    expect(raw_view(input, (*parsed)[0]) == "<p><p>inner</p>outer</p>",
        "outer p raw range should include the inner p");

    expect((*parsed)[1].tag_name == "p", "inner p should also survive as a p tag");
    expect(value_view(input, (*parsed)[1]) == "inner",
        "inner p value range should close at the first p close tag");
    expect(raw_view(input, (*parsed)[1]) == "<p>inner</p>",
        "inner p raw range should be preserved");
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
    parses_repeated_tag_names_by_latest_open_tag();
    rejects_missing_close_tag();
    rejects_mismatched_close_tag();
    rejects_invalid_tag_name();

    return failures == 0 ? 0 : 1;
}
