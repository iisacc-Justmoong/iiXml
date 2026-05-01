#include <iiXml>

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

std::string_view raw_view(std::string_view input, const iiXml::Elements::OpenTagRange& range) {
    return input.substr(range.RawBegin, range.RawEnd - range.RawBegin);
}

std::string_view value_view(std::string_view input, const iiXml::Elements::OpenTagRange& range) {
    return input.substr(range.ValueBegin, range.ValueEnd - range.ValueBegin);
}

void closes_stack_top_tag() {
    const iiXml::Elements::OpenTag open_tag;
    std::vector<std::string> open_tags{"a", "b"};

    const bool closed = open_tag.CloseOpenTag(open_tags, "b");

    expect(closed, "top open tag should close");
    expect(open_tags.size() == 1, "closing top tag should remove one entry");
    expect(open_tags.front() == "a", "closing top tag should preserve lower tags");
}

void closes_cross_nested_tag() {
    const iiXml::Elements::OpenTag open_tag;
    std::vector<std::string> open_tags{"a", "b"};

    const bool closed_a = open_tag.CloseOpenTag(open_tags, "a");
    const bool closed_b = open_tag.CloseOpenTag(open_tags, "b");

    expect(closed_a, "cross nested a tag should close before b");
    expect(closed_b, "remaining b tag should close after a");
    expect(open_tags.empty(), "cross nested closures should clear the open stack");
}

void rejects_unknown_closing_tag() {
    const iiXml::Elements::OpenTag open_tag;
    std::vector<std::string> open_tags{"a"};

    const bool closed = open_tag.CloseOpenTag(open_tags, "b");

    expect(!closed, "unknown closing tag should fail");
    expect(open_tags.size() == 1, "unknown closing tag should not mutate the stack");
    expect(open_tags.front() == "a", "unknown closing tag should preserve the open tag");
}

void rejects_empty_closing_tag() {
    const iiXml::Elements::OpenTag open_tag;
    std::vector<std::string> open_tags{"a"};

    const bool closed = open_tag.CloseOpenTag(open_tags, "");

    expect(!closed, "empty closing tag name should fail");
    expect(open_tags.size() == 1, "empty closing tag should not mutate the stack");
}

void parses_cross_nested_tags_without_rewriting() {
    const iiXml::Elements::OpenTag open_tag;
    const std::string input = "<a>\n    <b>\n</a>\n    </b>";

    const std::optional<std::vector<iiXml::Elements::OpenTagRange>> parsed =
        open_tag.ParseOpenTags(input);

    expect(parsed.has_value(), "cross nested tags should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 2, "cross nested input should keep both tags");
    if (parsed->size() != 2) {
        return;
    }

    expect((*parsed)[0].TagName == "a", "first preserved tag should be a");
    expect(value_view(input, (*parsed)[0]) == "\n    <b>\n",
        "a value range should preserve b opening markup");
    expect(raw_view(input, (*parsed)[0]) == "<a>\n    <b>\n</a>",
        "a raw range should preserve original a span");

    expect((*parsed)[1].TagName == "b", "second preserved tag should be b");
    expect(value_view(input, (*parsed)[1]) == "\n</a>\n    ",
        "b value range should preserve a closing markup");
    expect(raw_view(input, (*parsed)[1]) == "<b>\n</a>\n    </b>",
        "b raw range should preserve original b span");
}

void parses_many_cross_nested_tags_without_rewriting() {
    const iiXml::Elements::OpenTag open_tag;
    const std::string input = "<p><bold><italic>text</p><p>really</bold> useful</italic></p>";

    const std::optional<std::vector<iiXml::Elements::OpenTagRange>> parsed =
        open_tag.ParseOpenTags(input);

    expect(parsed.has_value(), "many cross nested tags should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 4, "many cross nested input should keep every tag");
    if (parsed->size() != 4) {
        return;
    }

    expect((*parsed)[0].TagName == "p", "first preserved tag should be first p");
    expect(value_view(input, (*parsed)[0]) == "<bold><italic>text",
        "first p value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[0]) == "<p><bold><italic>text</p>",
        "first p raw range should preserve original span");

    expect((*parsed)[1].TagName == "bold", "second preserved tag should be bold");
    expect(value_view(input, (*parsed)[1]) == "<italic>text</p><p>really",
        "bold value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[1]) == "<bold><italic>text</p><p>really</bold>",
        "bold raw range should cross both p tags");

    expect((*parsed)[2].TagName == "italic", "third preserved tag should be italic");
    expect(value_view(input, (*parsed)[2]) == "text</p><p>really</bold> useful",
        "italic value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[2]) == "<italic>text</p><p>really</bold> useful</italic>",
        "italic raw range should preserve its full crossing span");

    expect((*parsed)[3].TagName == "p", "fourth preserved tag should be second p");
    expect(value_view(input, (*parsed)[3]) == "really</bold> useful</italic>",
        "second p value range should be fixed only at its matching close tag");
    expect(raw_view(input, (*parsed)[3]) == "<p>really</bold> useful</italic></p>",
        "second p raw range should preserve original span");
}

void matches_repeated_tag_names_by_latest_open_tag() {
    const iiXml::Elements::OpenTag open_tag;
    const std::string input = "<p><p>inner</p>outer</p>";

    const std::optional<std::vector<iiXml::Elements::OpenTagRange>> parsed =
        open_tag.ParseOpenTags(input);

    expect(parsed.has_value(), "repeated tag names should parse");
    if (!parsed.has_value()) {
        return;
    }

    expect(parsed->size() == 2, "repeated tag names should preserve both p tags");
    if (parsed->size() != 2) {
        return;
    }

    expect((*parsed)[0].TagName == "p", "outer p should remain first by open order");
    expect(value_view(input, (*parsed)[0]) == "<p>inner</p>outer",
        "outer p value range should stay alive until its own closing tag");
    expect(raw_view(input, (*parsed)[0]) == "<p><p>inner</p>outer</p>",
        "outer p raw range should include the inner p");

    expect((*parsed)[1].TagName == "p", "inner p should also survive as a p tag");
    expect(value_view(input, (*parsed)[1]) == "inner",
        "inner p value range should close at the first p close tag");
    expect(raw_view(input, (*parsed)[1]) == "<p>inner</p>",
        "inner p raw range should be preserved");
}

void rejects_unclosed_preserved_tag() {
    const iiXml::Elements::OpenTag open_tag;

    const std::optional<std::vector<iiXml::Elements::OpenTagRange>> parsed =
        open_tag.ParseOpenTags("<a><b></a>");

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
    matches_repeated_tag_names_by_latest_open_tag();
    rejects_unclosed_preserved_tag();

    return failures == 0 ? 0 : 1;
}
