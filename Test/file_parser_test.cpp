#include <iiXml>

#include <filesystem>
#include <fstream>
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

void write_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    file << content;
}

void parses_file_content_with_tag_parser() {
    const std::filesystem::path path = std::filesystem::current_path() / "file_parser_valid.xml";
    write_file(path, "<number>42</number>");

    const iiXml::Parser::FileParser parser;
    const std::optional<iiXml::Parser::TagValue> parsed = parser.ParseFile(path);

    expect(parsed.has_value(), "valid tag file should parse");
    if (!parsed.has_value()) {
        std::filesystem::remove(path);
        return;
    }

    expect(parsed->TagName == "number", "file parser should preserve tag name");
    expect(parsed->Value == "42", "file parser should preserve tag value");

    std::filesystem::remove(path);
}

void preserves_utf8_file_value() {
    const std::filesystem::path path = std::filesystem::current_path() / "file_parser_utf8.xml";
    write_file(path, "<number>숫자</number>");

    const iiXml::Parser::FileParser parser;
    const std::optional<iiXml::Parser::TagValue> parsed = parser.ParseFile(path);

    expect(parsed.has_value(), "utf8 tag file should parse");
    if (!parsed.has_value()) {
        std::filesystem::remove(path);
        return;
    }

    expect(parsed->Value == "숫자", "file parser should preserve utf8 value");

    std::filesystem::remove(path);
}

void returns_empty_for_missing_file() {
    const iiXml::Parser::FileParser parser;
    const std::filesystem::path path = std::filesystem::current_path() / "file_parser_missing.xml";

    std::filesystem::remove(path);
    expect(!parser.ParseFile(path).has_value(), "missing file should not parse");
}

void returns_empty_when_tag_parser_rejects_content() {
    const std::filesystem::path path = std::filesystem::current_path() / "file_parser_invalid.xml";
    write_file(path, "<number>42</text>");

    const iiXml::Parser::FileParser parser;

    expect(!parser.ParseFile(path).has_value(), "invalid tag content should not parse");

    std::filesystem::remove(path);
}

} // namespace

int main() {
    parses_file_content_with_tag_parser();
    preserves_utf8_file_value();
    returns_empty_for_missing_file();
    returns_empty_when_tag_parser_rejects_content();

    return failures == 0 ? 0 : 1;
}
