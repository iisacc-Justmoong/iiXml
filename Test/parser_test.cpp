#include <iiXml>

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

std::string_view raw_view(std::string_view input, const iiXml::Parser::TagRange& range) {
    return input.substr(range.RawBegin, range.RawEnd - range.RawBegin);
}

std::string_view value_view(std::string_view input, const iiXml::Parser::TagRange& range) {
    return input.substr(range.ValueBegin, range.ValueEnd - range.ValueBegin);
}

std::string_view field_value_view(std::string_view input, const iiXml::Parser::TagField& field) {
    return input.substr(field.ValueBegin, field.ValueEnd - field.ValueBegin);
}

void parses_basic_tag() {
    const iiXml::Parser::TagParser parser;

    const std::optional<iiXml::Parser::TagValue> parsed = parser.Parse("<number>123</number>");

    expect(parsed.has_value(), "number tag should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->TagName == "number", "tag name should be number");
    expect(parsed->Value == "123", "tag value should be 123");
}

void parses_basic_tag_result() {
    const iiXml::Parser::TagParser parser;

    const iiXml::Parser::TagParseResult result = parser.ParseResult("<number>123</number>");

    expect(result.Status == iiXml::Parser::TagParseStatus::Parsed,
        "tag result status should be Parsed");
    expect(result.Token.has_value(), "tag result should contain token");
    expect(result.Diagnostic.Reason == "tag parsed",
        "successful tag result should include structured reason");
    if (!result.Token.has_value()) {
        return;
    }

    expect(result.Token->TagName == "number", "tag result name should be number");
    expect(result.Token->Value == "123", "tag result value should be 123");
    expect(result.Token->Raw == "<number>123</number>", "tag result raw should preserve input");
}

void parses_utf8_value() {
    const iiXml::Parser::TagParser parser;

    const std::optional<iiXml::Parser::TagValue> parsed = parser.Parse("<number>숫자</number>");

    expect(parsed.has_value(), "utf8 value tag should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->TagName == "number", "utf8 value tag name should be number");
    expect(parsed->Value == "숫자", "utf8 value should be preserved");
}

void parses_opening_tag_with_attributes() {
    const iiXml::Parser::TagParser parser;

    const std::optional<iiXml::Parser::TagValue> parsed =
        parser.Parse("<number type=\"int\">42</number>");

    expect(parsed.has_value(), "tag with attributes should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->TagName == "number", "tag name should ignore attributes");
    expect(parsed->Value == "42", "tag value with attributes should be preserved");
}

void preserves_inner_whitespace() {
    const iiXml::Parser::TagParser parser;

    const std::optional<iiXml::Parser::TagValue> parsed = parser.Parse(" \n<number> 123 </number>\t");

    expect(parsed.has_value(), "outer whitespace should be ignored");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->Value == " 123 ", "inner whitespace should be preserved");
}

void parses_cross_nested_tags_as_independent_values() {
    const iiXml::Parser::TagParser parser;
    const std::string input = "<a>\n    <b>\n</a>\n    </b>";

    const std::optional<std::vector<iiXml::Parser::TagNode>> parsed = parser.ParseAll(input);

    expect(parsed.has_value(), "cross nested tags should parse as multiple values");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 2, "cross nested parse should keep both a and b");
    if (parsed->size() != 2) {
        return;
    }

    expect((*parsed)[0].Range.TagName == "a", "first parsed tag should be a");
    expect(value_view(input, (*parsed)[0].Range) == "\n    <b>\n",
        "a parsed value range should keep b opening markup");
    expect(raw_view(input, (*parsed)[0].Range) == "<a>\n    <b>\n</a>",
        "a raw range should be preserved");

    expect((*parsed)[1].Range.TagName == "b", "second parsed tag should be b");
    expect(value_view(input, (*parsed)[1].Range) == "\n</a>\n    ",
        "b parsed value range should keep a closing markup");
    expect(raw_view(input, (*parsed)[1].Range) == "<b>\n</a>\n    </b>",
        "b raw range should be preserved");
}

void parses_cross_nested_tags_result() {
    const iiXml::Parser::TagParser parser;
    const std::string input = "<a><b></a></b>";

    const iiXml::Parser::TagTreeParseResult result = parser.ParseAllResult(input);

    expect(result.Status == iiXml::Parser::TagTreeParseStatus::Parsed,
        "tree parse result status should be Parsed");
    expect(result.Nodes.has_value(), "tree parse result should contain nodes");
    expect(result.Diagnostic.Reason == "tag tree parsed",
        "successful tree result should include structured reason");
    if (!result.Nodes.has_value()) {
        return;
    }

    expect(result.Nodes->size() == 2, "tree parse result should keep both cross nested tags");
    if (result.Nodes->size() != 2) {
        return;
    }

    expect((*result.Nodes)[0].Range.TagName == "a", "first tree result node should be a");
    expect((*result.Nodes)[1].Range.TagName == "b", "second tree result node should be b");
}

void parses_cross_nested_tags_as_range_document() {
    const iiXml::Parser::TagParser parser;
    const std::string input = "<a><b></a></b>";

    const iiXml::Parser::TagDocumentResult result = parser.ParseAllDocumentResult(input);

    expect(result.Status == iiXml::Parser::TagTreeParseStatus::Parsed,
        "tag document result status should be Parsed");
    expect(result.Document.has_value(), "tag document result should contain a document");
    expect(result.Diagnostic.Reason == "tag document parsed",
        "successful tag document result should include structured reason");
    if (!result.Document.has_value()) {
        return;
    }

    const iiXml::Parser::TagDocument& document = *result.Document;
    expect(document.Source == input, "tag document should own the original source once");
    expect(document.Nodes.size() == 2, "tag document should keep both cross nested roots");
    if (document.Nodes.size() != 2) {
        return;
    }

    expect(document.Nodes[0].Range.TagName == "a", "first document node should be a");
    expect(document.ValueView(document.Nodes[0]) == "<b>",
        "first document node should expose value as source view");
    expect(document.RawView(document.Nodes[0]) == "<a><b></a>",
        "first document node should expose raw as source view");

    expect(document.Nodes[1].Range.TagName == "b", "second document node should be b");
    expect(document.ValueView(document.Nodes[1]) == "</a>",
        "second document node should expose crossed value as source view");
    expect(document.RawView(document.Nodes[1]) == "<b></a></b>",
        "second document node should expose crossed raw as source view");

    std::optional<iiXml::Parser::TagDocument> parsed = parser.ParseAllDocument(input);
    expect(parsed.has_value(), "ParseAllDocument should return an embeddable document");
    if (!parsed.has_value()) {
        return;
    }

    iiXml::Parser::TagDocument embedded_document = std::move(*parsed);
    expect(embedded_document.ValueView(embedded_document.Nodes[0]) == "<b>",
        "moved tag document should still resolve values from owned source");
}

void parses_many_cross_nested_tags_as_independent_values() {
    const iiXml::Parser::TagParser parser;
    const std::string input = "<p><bold><italic>text</p><p>really</bold> useful</italic></p>";

    const std::optional<std::vector<iiXml::Parser::TagNode>> parsed = parser.ParseAll(input);

    expect(parsed.has_value(), "many cross nested tags should parse as multiple values");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 4, "many cross nested parse should keep every tag");
    if (parsed->size() != 4) {
        return;
    }

    expect((*parsed)[0].Range.TagName == "p", "first parsed tag should be first p");
    expect(value_view(input, (*parsed)[0].Range) == "<bold><italic>text",
        "first p value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[0].Range) == "<p><bold><italic>text</p>",
        "first p raw range should be preserved");

    expect((*parsed)[1].Range.TagName == "bold", "second parsed tag should be bold");
    expect(value_view(input, (*parsed)[1].Range) == "<italic>text</p><p>really",
        "bold value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[1].Range) == "<bold><italic>text</p><p>really</bold>",
        "bold raw range should cross paragraph tags");

    expect((*parsed)[2].Range.TagName == "italic", "third parsed tag should be italic");
    expect(value_view(input, (*parsed)[2].Range) == "text</p><p>really</bold> useful",
        "italic value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[2].Range) == "<italic>text</p><p>really</bold> useful</italic>",
        "italic raw range should be preserved");

    expect((*parsed)[3].Range.TagName == "p", "fourth parsed tag should be second p");
    expect(value_view(input, (*parsed)[3].Range) == "really</bold> useful</italic>",
        "second p value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[3].Range) == "<p>really</bold> useful</italic></p>",
        "second p raw range should be preserved");
}

void parses_repeated_tag_names_by_latest_open_tag() {
    const iiXml::Parser::TagParser parser;
    const std::string input = "<p><p>inner</p>outer</p>";

    const std::optional<std::vector<iiXml::Parser::TagNode>> parsed =
        parser.ParseAll(input);

    expect(parsed.has_value(), "repeated tag names should parse as multiple values");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 1, "repeated tag name parse should return the outer p root");
    if (parsed->size() != 1) {
        return;
    }

    expect((*parsed)[0].Range.TagName == "p", "outer p should remain first by open order");
    expect(value_view(input, (*parsed)[0].Range) == "<p>inner</p>outer",
        "outer p value range should stay alive until its own close tag");
    expect(raw_view(input, (*parsed)[0].Range) == "<p><p>inner</p>outer</p>",
        "outer p raw range should include the inner p");
    expect((*parsed)[0].Children.size() == 1, "outer p should contain the inner p child");
    if ((*parsed)[0].Children.size() != 1) {
        return;
    }

    const iiXml::Parser::TagNode& inner = (*parsed)[0].Children[0];
    expect(inner.Range.TagName == "p", "inner p should also survive as a p tag");
    expect(value_view(input, inner.Range) == "inner",
        "inner p value range should close at the first p close tag");
    expect(raw_view(input, inner.Range) == "<p>inner</p>",
        "inner p raw range should be preserved");
}

void parses_hierarchical_document_with_fields() {
    const iiXml::Parser::TagParser parser;
    const std::string input =
        "<contents id=\"abc\"><body>"
        "<paragraph order=1 visible=true ratio=0.5 label=\"1\">one</paragraph>"
        "<paragraph>two</paragraph></body></contents>";

    const std::optional<std::vector<iiXml::Parser::TagNode>> parsed =
        parser.ParseAll(input);

    expect(parsed.has_value(), "hierarchical document should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 1, "hierarchical document should have one root");
    if (parsed->size() != 1) {
        return;
    }

    const iiXml::Parser::TagNode& contents = (*parsed)[0];
    expect(contents.Range.TagName == "contents", "root tag should be contents");
    expect(contents.Fields.size() == 1, "contents should expose id field");
    if (contents.Fields.size() == 1) {
        expect(contents.Fields[0].Name == "id", "contents field name should be id");
        expect(contents.Fields[0].HasValue, "contents id field should have value");
        expect(field_value_view(input, contents.Fields[0]) == "abc",
            "contents id field value should be abc");
        expect(contents.Fields[0].ValueType == iiXml::Elements::InlinePropertyType::StringType,
            "contents id field should infer string type");
        expect(!contents.Fields[0].TypeDeclared,
            "contents id field should not mark declared type");
    }

    expect(contents.Children.size() == 1, "contents should contain body child");
    if (contents.Children.size() != 1) {
        return;
    }

    const iiXml::Parser::TagNode& body = contents.Children[0];
    expect(body.Range.TagName == "body", "body child should be body");
    expect(body.Children.size() == 2, "body should contain two paragraphs");
    if (body.Children.size() != 2) {
        return;
    }

    const iiXml::Parser::TagNode& first_paragraph = body.Children[0];
    expect(first_paragraph.Range.TagName == "paragraph", "first child should be paragraph");
    expect(value_view(input, first_paragraph.Range) == "one",
        "first paragraph value should be one");
    expect(first_paragraph.Fields.size() == 4,
        "first paragraph should expose order, visible, ratio, and label fields");
    if (first_paragraph.Fields.size() == 4) {
        expect(first_paragraph.Fields[0].Name == "order",
            "first paragraph field name should be order");
        expect(field_value_view(input, first_paragraph.Fields[0]) == "1",
            "first paragraph order should be 1");
        expect(first_paragraph.Fields[0].ValueType == iiXml::Elements::InlinePropertyType::IntType,
            "first paragraph order should infer int type");
        expect(!first_paragraph.Fields[0].TypeDeclared,
            "first paragraph order should not mark declared type");

        expect(first_paragraph.Fields[1].Name == "visible",
            "second paragraph field name should be visible");
        expect(field_value_view(input, first_paragraph.Fields[1]) == "true",
            "first paragraph visible should be true");
        expect(first_paragraph.Fields[1].ValueType == iiXml::Elements::InlinePropertyType::BoolType,
            "first paragraph visible should infer bool type");

        expect(first_paragraph.Fields[2].Name == "ratio",
            "third paragraph field name should be ratio");
        expect(field_value_view(input, first_paragraph.Fields[2]) == "0.5",
            "first paragraph ratio should be 0.5");
        expect(first_paragraph.Fields[2].ValueType == iiXml::Elements::InlinePropertyType::FloatType,
            "first paragraph ratio should infer float type");

        expect(first_paragraph.Fields[3].Name == "label",
            "fourth paragraph field name should be label");
        expect(field_value_view(input, first_paragraph.Fields[3]) == "1",
            "first paragraph label should be 1");
        expect(first_paragraph.Fields[3].ValueType == iiXml::Elements::InlinePropertyType::StringType,
            "quoted first paragraph label should remain string type");
    }

    const iiXml::Parser::TagNode& second_paragraph = body.Children[1];
    expect(second_paragraph.Range.TagName == "paragraph", "second child should be paragraph");
    expect(value_view(input, second_paragraph.Range) == "two",
        "second paragraph value should be two");
    expect(second_paragraph.Fields.empty(), "second paragraph should have no fields");
}

void parses_hierarchical_range_document_with_fields() {
    const iiXml::Parser::TagParser parser;
    const std::string input =
        "<contents id=\"abc\"><body>"
        "<paragraph order=1 visible=true ratio=0.5 label=\"1\">one</paragraph>"
        "<paragraph>two</paragraph></body></contents>";

    const std::optional<iiXml::Parser::TagDocument> parsed =
        parser.ParseAllDocument(input);

    expect(parsed.has_value(), "hierarchical tag document should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->Nodes.size() == 1, "hierarchical tag document should have one root");
    if (parsed->Nodes.size() != 1) {
        return;
    }

    const iiXml::Parser::TagNode& contents = parsed->Nodes[0];
    expect(contents.Range.TagName == "contents", "document root tag should be contents");
    expect(contents.Fields.size() == 1, "document root should expose id field");
    if (contents.Fields.size() == 1) {
        expect(parsed->FieldNameView(contents.Fields[0]) == "id",
            "document field name view should be id");
        expect(parsed->FieldValueView(contents.Fields[0]) == "abc",
            "document field value view should be abc");
    }

    expect(contents.Children.size() == 1, "document root should contain body child");
    if (contents.Children.size() != 1 || contents.Children[0].Children.size() != 2) {
        return;
    }

    const iiXml::Parser::TagNode& first_paragraph = contents.Children[0].Children[0];
    expect(parsed->ValueView(first_paragraph) == "one",
        "document child value view should be one");
    expect(first_paragraph.Fields.size() == 4,
        "document first paragraph should expose four fields");
    if (first_paragraph.Fields.size() == 4) {
        expect(parsed->FieldValueView(first_paragraph.Fields[0]) == "1",
            "document order field value view should be 1");
        expect(parsed->FieldValueView(first_paragraph.Fields[1]) == "true",
            "document visible field value view should be true");
        expect(parsed->FieldValueView(first_paragraph.Fields[2]) == "0.5",
            "document ratio field value view should be 0.5");
        expect(parsed->FieldValueView(first_paragraph.Fields[3]) == "1",
            "document label field value view should be 1");
    }
}

void rejects_missing_close_tag() {
    const iiXml::Parser::TagParser parser;

    expect(!parser.Parse("<number>123").has_value(), "missing close tag should fail");
}

void reports_missing_close_tag_result() {
    const iiXml::Parser::TagParser parser;

    const iiXml::Parser::TagParseResult result = parser.ParseResult("<number>123");

    expect(result.Status == iiXml::Parser::TagParseStatus::InputShorterThanClosingTag,
        "missing close tag result should report InputShorterThanClosingTag");
    expect(!result.Token.has_value(), "missing close tag result should not contain token");
    expect(result.Diagnostic.Reason == "input shorter than closing tag",
        "missing close tag result should include failure reason");
    expect(!result.Diagnostic.Context.empty(),
        "missing close tag result should include diagnostic context");
}

void rejects_mismatched_close_tag() {
    const iiXml::Parser::TagParser parser;

    expect(!parser.Parse("<number>123</text>").has_value(), "mismatched close tag should fail");
}

void reports_mismatched_close_tag_result() {
    const iiXml::Parser::TagParser parser;

    const iiXml::Parser::TagParseResult result = parser.ParseResult("<number>123</text>");

    expect(result.Status == iiXml::Parser::TagParseStatus::ClosingTagMismatch,
        "mismatched close tag result should report ClosingTagMismatch");
    expect(!result.Token.has_value(), "mismatched close tag result should not contain token");
    expect(result.Diagnostic.Reason == "closing tag mismatch",
        "mismatched close tag result should include failure reason");
    expect(result.Diagnostic.Line == 1, "mismatched close tag result should include line");
    expect(result.Diagnostic.Column > 1, "mismatched close tag result should include column");
}

void rejects_invalid_tag_name() {
    const iiXml::Parser::TagParser parser;

    expect(!parser.Parse("<1number>123</1number>").has_value(), "invalid tag name should fail");
}

void reports_invalid_tag_name_result() {
    const iiXml::Parser::TagParser parser;

    const iiXml::Parser::TagParseResult result = parser.ParseResult("<1number>123</1number>");

    expect(result.Status == iiXml::Parser::TagParseStatus::InvalidTagName,
        "invalid tag name result should report InvalidTagName");
    expect(!result.Token.has_value(), "invalid tag name result should not contain token");
    expect(result.Diagnostic.Offset == 1, "invalid tag name result should report name offset");
    expect(result.Diagnostic.Line == 1, "invalid tag name result should report line");
    expect(result.Diagnostic.Column == 2, "invalid tag name result should report column");
}

void reports_tree_parse_failure_result() {
    const iiXml::Parser::TagParser parser;

    const iiXml::Parser::TagTreeParseResult result = parser.ParseAllResult("<root><child></root>");

    expect(result.Status == iiXml::Parser::TagTreeParseStatus::OpenTagParserRejected,
        "tree parse failure should report OpenTagParserRejected");
    expect(!result.Nodes.has_value(), "tree parse failure should not contain nodes");
    expect(result.Diagnostic.Reason == "open tag parser rejected input",
        "tree parse failure should include failure reason");
    expect(!result.Diagnostic.Context.empty(),
        "tree parse failure should include diagnostic context");
}

void reports_tag_document_failure_result() {
    const iiXml::Parser::TagParser parser;

    const iiXml::Parser::TagDocumentResult result =
        parser.ParseAllDocumentResult("<root><child></root>");

    expect(result.Status == iiXml::Parser::TagTreeParseStatus::OpenTagParserRejected,
        "tag document failure should report OpenTagParserRejected");
    expect(!result.Document.has_value(),
        "tag document failure should not contain a document");
    expect(result.Diagnostic.Reason == "open tag parser rejected input",
        "tag document failure should preserve range parser failure reason");
    expect(!parser.ParseAllDocument("<root><child></root>").has_value(),
        "ParseAllDocument should return nullopt on range parser rejection");
}

} // namespace

int main() {
    parses_basic_tag();
    parses_basic_tag_result();
    parses_utf8_value();
    parses_opening_tag_with_attributes();
    preserves_inner_whitespace();
    parses_cross_nested_tags_as_independent_values();
    parses_cross_nested_tags_result();
    parses_cross_nested_tags_as_range_document();
    parses_many_cross_nested_tags_as_independent_values();
    parses_repeated_tag_names_by_latest_open_tag();
    parses_hierarchical_document_with_fields();
    parses_hierarchical_range_document_with_fields();
    rejects_missing_close_tag();
    reports_missing_close_tag_result();
    rejects_mismatched_close_tag();
    reports_mismatched_close_tag_result();
    rejects_invalid_tag_name();
    reports_invalid_tag_name_result();
    reports_tree_parse_failure_result();
    reports_tag_document_failure_result();

    return failures == 0 ? 0 : 1;
}
