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

void closes_stack_top_tag() {
    const iiXml::elements::OpenTag open_tag;
    std::vector<std::string> open_tags{"a", "b"};

    const bool closed = open_tag.close_open_tag(open_tags, "b");

    expect(closed, "top open tag should close");
    expect(open_tags.size() == 1, "closing top tag should remove one entry");
    expect(open_tags.front() == "a", "closing top tag should preserve lower tags");
}

void closes_cross_nested_tag() {
    const iiXml::elements::OpenTag open_tag;
    std::vector<std::string> open_tags{"a", "b"};

    const bool closed_a = open_tag.close_open_tag(open_tags, "a");
    const bool closed_b = open_tag.close_open_tag(open_tags, "b");

    expect(closed_a, "cross nested a tag should close before b");
    expect(closed_b, "remaining b tag should close after a");
    expect(open_tags.empty(), "cross nested closures should clear the open stack");
}

void rejects_unknown_closing_tag() {
    const iiXml::elements::OpenTag open_tag;
    std::vector<std::string> open_tags{"a"};

    const bool closed = open_tag.close_open_tag(open_tags, "b");

    expect(!closed, "unknown closing tag should fail");
    expect(open_tags.size() == 1, "unknown closing tag should not mutate the stack");
    expect(open_tags.front() == "a", "unknown closing tag should preserve the open tag");
}

void rejects_empty_closing_tag() {
    const iiXml::elements::OpenTag open_tag;
    std::vector<std::string> open_tags{"a"};

    const bool closed = open_tag.close_open_tag(open_tags, "");

    expect(!closed, "empty closing tag name should fail");
    expect(open_tags.size() == 1, "empty closing tag should not mutate the stack");
}

void parses_cross_nested_tags_without_rewriting() {
    const iiXml::elements::OpenTag open_tag;
    const std::string input = "<a>\n    <b>\n</a>\n    </b>";

    const std::optional<std::vector<iiXml::elements::open_tag_value>> parsed =
        open_tag.parse_open_tags(input);

    expect(parsed.has_value(), "cross nested tags should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 2, "cross nested input should keep both tags");
    if (parsed->size() != 2) {
        return;
    }

    expect((*parsed)[0].tag_name == "a", "first preserved tag should be a");
    expect((*parsed)[0].value == "\n    <b>\n", "a value should preserve b opening markup");
    expect((*parsed)[0].raw == "<a>\n    <b>\n</a>", "a raw text should preserve original a span");

    expect((*parsed)[1].tag_name == "b", "second preserved tag should be b");
    expect((*parsed)[1].value == "\n</a>\n    ", "b value should preserve a closing markup");
    expect((*parsed)[1].raw == "<b>\n</a>\n    </b>", "b raw text should preserve original b span");
}

void parses_many_cross_nested_tags_without_rewriting() {
    const iiXml::elements::OpenTag open_tag;
    const std::string input = "<p><bold><italic>text</p><p>really</bold> useful</italic></p>";

    const std::optional<std::vector<iiXml::elements::open_tag_value>> parsed =
        open_tag.parse_open_tags(input);

    expect(parsed.has_value(), "many cross nested tags should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 4, "many cross nested input should keep every tag");
    if (parsed->size() != 4) {
        return;
    }

    expect((*parsed)[0].tag_name == "p", "first preserved tag should be first p");
    expect((*parsed)[0].value == "<bold><italic>text",
        "first p value should be fixed only at its matching close tag");
    expect((*parsed)[0].raw == "<p><bold><italic>text</p>",
        "first p raw text should preserve original span");

    expect((*parsed)[1].tag_name == "bold", "second preserved tag should be bold");
    expect((*parsed)[1].value == "<italic>text</p><p>really",
        "bold value should be fixed only at its matching close tag");
    expect((*parsed)[1].raw == "<bold><italic>text</p><p>really</bold>",
        "bold raw text should cross both p tags");

    expect((*parsed)[2].tag_name == "italic", "third preserved tag should be italic");
    expect((*parsed)[2].value == "text</p><p>really</bold> useful",
        "italic value should be fixed only at its matching close tag");
    expect((*parsed)[2].raw == "<italic>text</p><p>really</bold> useful</italic>",
        "italic raw text should preserve its full crossing span");

    expect((*parsed)[3].tag_name == "p", "fourth preserved tag should be second p");
    expect((*parsed)[3].value == "really</bold> useful</italic>",
        "second p value should be fixed only at its matching close tag");
    expect((*parsed)[3].raw == "<p>really</bold> useful</italic></p>",
        "second p raw text should preserve original span");
}

void rejects_unclosed_preserved_tag() {
    const iiXml::elements::OpenTag open_tag;

    const std::optional<std::vector<iiXml::elements::open_tag_value>> parsed =
        open_tag.parse_open_tags("<a><b></a>");

    expect(!parsed.has_value(), "unclosed preserved tag should fail");
}

} // namespace

int main() {
    closes_stack_top_tag();
    closes_cross_nested_tag();
    rejects_unknown_closing_tag();
    rejects_empty_closing_tag();
    parses_cross_nested_tags_without_rewriting();
    parses_many_cross_nested_tags_without_rewriting();
    rejects_unclosed_preserved_tag();

    return failures == 0 ? 0 : 1;
}
