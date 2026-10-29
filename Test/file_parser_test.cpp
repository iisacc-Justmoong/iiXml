#include <iiXml>

#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {
int failures = 0;
void expect(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; ++failures; }
}

iiXml::IO::FileReader fixture_reader() {
    return [](const std::filesystem::path& path) -> std::optional<std::string> {
        if (path == "valid.xml") return "<number>42</number>";
        if (path == "utf8.xml") return "<number>숫자</number>";
        if (path == "invalid.xml") return "<number>42</text>";
        if (path == "validated.xml") return "<!Doctype XML>\n<XML><number>42</number></XML>";
        return std::nullopt;
    };
}

void parses_file_content_with_tag_parser() {
    const iiXml::Parser::FileParser parser(fixture_reader());
    const auto parsed = parser.ParseFile("valid.xml");
    expect(parsed.has_value(), "valid tag file should parse");
    if (parsed) {
        expect(parsed->TagName == "number", "file parser should preserve tag name");
        expect(parsed->Value == "42", "file parser should preserve tag value");
    }
}
void preserves_utf8_file_value() {
    const iiXml::Parser::FileParser parser(fixture_reader());
    const auto parsed = parser.ParseFile("utf8.xml");
    expect(parsed.has_value(), "UTF-8 tag file should parse");
    if (parsed) expect(parsed->Value == "숫자", "file parser should preserve UTF-8 value");
}
void rejects_missing_invalid_and_unconfigured_inputs() {
    const iiXml::Parser::FileParser parser(fixture_reader());
    expect(!parser.ParseFile("missing.xml"), "missing file should not parse");
    expect(!parser.ParseFile("invalid.xml"), "invalid tag content should not parse");
    const iiXml::Parser::FileParser unconfigured;
    expect(!unconfigured.ParseFile("valid.xml"), "an absent reader should fail closed");
    const iiXml::Writer::GetFile validated(fixture_reader());
    const auto result = validated.ParseFile("validated.xml");
    expect(result.Status == iiXml::Writer::GetFileStatus::Parsed, "validated file should parse");
    const iiXml::Writer::GetFile no_reader;
    expect(no_reader.ParseFile("validated.xml").Status == iiXml::Writer::GetFileStatus::FileReadFailed,
           "validated parser should report an absent reader");
}
void reader_exceptions_do_not_escape() {
    const iiXml::IO::FileReader reader = [](const auto&) -> std::optional<std::string> {
        throw std::runtime_error("storage failure");
    };
    const iiXml::Parser::FileParser parser(reader);
    const iiXml::Writer::GetFile validated(reader);
    expect(!parser.ParseFile("any"), "parser should report a reader exception");
    expect(validated.ParseFile("any").Status == iiXml::Writer::GetFileStatus::ExceptionThrown,
           "validated parser should report a reader exception");
}
}
int main() {
    parses_file_content_with_tag_parser();
    preserves_utf8_file_value();
    rejects_missing_invalid_and_unconfigured_inputs();
    reader_exceptions_do_not_escape();
    return failures == 0 ? 0 : 1;
}
