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

std::string_view field_value_view(std::string_view input, const iiXml::parser::tag_field& field) {
    return input.substr(field.value_begin, field.value_end - field.value_begin);
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

    const std::optional<std::vector<iiXml::parser::tag_node>> parsed = parser.parse_all(input);

    expect(parsed.has_value(), "cross nested tags should parse as multiple values");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 2, "cross nested parse should keep both a and b");
    if (parsed->size() != 2) {
        return;
    }

    expect((*parsed)[0].range.tag_name == "a", "first parsed tag should be a");
    expect(value_view(input, (*parsed)[0].range) == "\n    <b>\n",
        "a parsed value range should keep b opening markup");
    expect(raw_view(input, (*parsed)[0].range) == "<a>\n    <b>\n</a>",
        "a raw range should be preserved");

    expect((*parsed)[1].range.tag_name == "b", "second parsed tag should be b");
    expect(value_view(input, (*parsed)[1].range) == "\n</a>\n    ",
        "b parsed value range should keep a closing markup");
    expect(raw_view(input, (*parsed)[1].range) == "<b>\n</a>\n    </b>",
        "b raw range should be preserved");
}

void parses_many_cross_nested_tags_as_independent_values() {
    const iiXml::parser::tag_parser parser;
    const std::string input = "<p><bold><italic>text</p><p>really</bold> useful</italic></p>";

    const std::optional<std::vector<iiXml::parser::tag_node>> parsed = parser.parse_all(input);

    expect(parsed.has_value(), "many cross nested tags should parse as multiple values");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 4, "many cross nested parse should keep every tag");
    if (parsed->size() != 4) {
        return;
    }

    expect((*parsed)[0].range.tag_name == "p", "first parsed tag should be first p");
    expect(value_view(input, (*parsed)[0].range) == "<bold><italic>text",
        "first p value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[0].range) == "<p><bold><italic>text</p>",
        "first p raw range should be preserved");

    expect((*parsed)[1].range.tag_name == "bold", "second parsed tag should be bold");
    expect(value_view(input, (*parsed)[1].range) == "<italic>text</p><p>really",
        "bold value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[1].range) == "<bold><italic>text</p><p>really</bold>",
        "bold raw range should cross paragraph tags");

    expect((*parsed)[2].range.tag_name == "italic", "third parsed tag should be italic");
    expect(value_view(input, (*parsed)[2].range) == "text</p><p>really</bold> useful",
        "italic value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[2].range) == "<italic>text</p><p>really</bold> useful</italic>",
        "italic raw range should be preserved");

    expect((*parsed)[3].range.tag_name == "p", "fourth parsed tag should be second p");
    expect(value_view(input, (*parsed)[3].range) == "really</bold> useful</italic>",
        "second p value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[3].range) == "<p>really</bold> useful</italic></p>",
        "second p raw range should be preserved");
}

void parses_repeated_tag_names_by_latest_open_tag() {
    const iiXml::parser::tag_parser parser;
    const std::string input = "<p><p>inner</p>outer</p>";

    const std::optional<std::vector<iiXml::parser::tag_node>> parsed =
        parser.parse_all(input);

    expect(parsed.has_value(), "repeated tag names should parse as multiple values");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 1, "repeated tag name parse should return the outer p root");
    if (parsed->size() != 1) {
        return;
    }

    expect((*parsed)[0].range.tag_name == "p", "outer p should remain first by open order");
    expect(value_view(input, (*parsed)[0].range) == "<p>inner</p>outer",
        "outer p value range should stay alive until its own close tag");
    expect(raw_view(input, (*parsed)[0].range) == "<p><p>inner</p>outer</p>",
        "outer p raw range should include the inner p");
    expect((*parsed)[0].children.size() == 1, "outer p should contain the inner p child");
    if ((*parsed)[0].children.size() != 1) {
        return;
    }

    const iiXml::parser::tag_node& inner = (*parsed)[0].children[0];
    expect(inner.range.tag_name == "p", "inner p should also survive as a p tag");
    expect(value_view(input, inner.range) == "inner",
        "inner p value range should close at the first p close tag");
    expect(raw_view(input, inner.range) == "<p>inner</p>",
        "inner p raw range should be preserved");
}

void parses_hierarchical_document_with_fields() {
    const iiXml::parser::tag_parser parser;
    const std::string input =
        "<contents id=\"abc\"><body>"
        "<paragraph order=1 visible=true ratio=0.5 label=\"1\">one</paragraph>"
        "<paragraph>two</paragraph></body></contents>";

    const std::optional<std::vector<iiXml::parser::tag_node>> parsed =
        parser.parse_all(input);

    expect(parsed.has_value(), "hierarchical document should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 1, "hierarchical document should have one root");
    if (parsed->size() != 1) {
        return;
    }

    const iiXml::parser::tag_node& contents = (*parsed)[0];
    expect(contents.range.tag_name == "contents", "root tag should be contents");
    expect(contents.fields.size() == 1, "contents should expose id field");
    if (contents.fields.size() == 1) {
        expect(contents.fields[0].name == "id", "contents field name should be id");
        expect(contents.fields[0].has_value, "contents id field should have value");
        expect(field_value_view(input, contents.fields[0]) == "abc",
            "contents id field value should be abc");
        expect(contents.fields[0].value_type == iiXml::elements::inline_property_type::string_type,
            "contents id field should infer string type");
        expect(!contents.fields[0].type_declared,
            "contents id field should not mark declared type");
    }

    expect(contents.children.size() == 1, "contents should contain body child");
    if (contents.children.size() != 1) {
        return;
    }

    const iiXml::parser::tag_node& body = contents.children[0];
    expect(body.range.tag_name == "body", "body child should be body");
    expect(body.children.size() == 2, "body should contain two paragraphs");
    if (body.children.size() != 2) {
        return;
    }

    const iiXml::parser::tag_node& first_paragraph = body.children[0];
    expect(first_paragraph.range.tag_name == "paragraph", "first child should be paragraph");
    expect(value_view(input, first_paragraph.range) == "one",
        "first paragraph value should be one");
    expect(first_paragraph.fields.size() == 4,
        "first paragraph should expose order, visible, ratio, and label fields");
    if (first_paragraph.fields.size() == 4) {
        expect(first_paragraph.fields[0].name == "order",
            "first paragraph field name should be order");
        expect(field_value_view(input, first_paragraph.fields[0]) == "1",
            "first paragraph order should be 1");
        expect(first_paragraph.fields[0].value_type == iiXml::elements::inline_property_type::int_type,
            "first paragraph order should infer int type");
        expect(!first_paragraph.fields[0].type_declared,
            "first paragraph order should not mark declared type");

        expect(first_paragraph.fields[1].name == "visible",
            "second paragraph field name should be visible");
        expect(field_value_view(input, first_paragraph.fields[1]) == "true",
            "first paragraph visible should be true");
        expect(first_paragraph.fields[1].value_type == iiXml::elements::inline_property_type::bool_type,
            "first paragraph visible should infer bool type");

        expect(first_paragraph.fields[2].name == "ratio",
            "third paragraph field name should be ratio");
        expect(field_value_view(input, first_paragraph.fields[2]) == "0.5",
            "first paragraph ratio should be 0.5");
        expect(first_paragraph.fields[2].value_type == iiXml::elements::inline_property_type::float_type,
            "first paragraph ratio should infer float type");

        expect(first_paragraph.fields[3].name == "label",
            "fourth paragraph field name should be label");
        expect(field_value_view(input, first_paragraph.fields[3]) == "1",
            "first paragraph label should be 1");
        expect(first_paragraph.fields[3].value_type == iiXml::elements::inline_property_type::string_type,
            "quoted first paragraph label should remain string type");
    }

    const iiXml::parser::tag_node& second_paragraph = body.children[1];
    expect(second_paragraph.range.tag_name == "paragraph", "second child should be paragraph");
    expect(value_view(input, second_paragraph.range) == "two",
        "second paragraph value should be two");
    expect(second_paragraph.fields.empty(), "second paragraph should have no fields");
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
    parses_hierarchical_document_with_fields();
    rejects_missing_close_tag();
    rejects_mismatched_close_tag();
    rejects_invalid_tag_name();

    return failures == 0 ? 0 : 1;
}
