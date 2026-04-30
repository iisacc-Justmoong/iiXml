#include "iiXml.h"

#include <QTemporaryDir>
#include <QString>

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

void write_file(const QString& path, const std::string& content) {
    std::ofstream file(path.toStdString(), std::ios::binary);
    file << content;
}

void parses_valid_doctype_xml_text() {
    const iiXml::writer::GetFile input;

    const iiXml::writer::get_file_result result =
        input.parse_xml("<!DOCTYPE XML>\n<XML><number>1</number></XML>");

    expect(result.status == iiXml::writer::get_file_status::parsed,
        "valid DOCTYPE XML text should parse");
    expect(!result.reason.empty(), "valid DOCTYPE XML text should return non-empty reason");
    expect(result.token.has_value(), "valid DOCTYPE XML text should return token");
    if (!result.token.has_value()) {
        return;
    }

    expect(result.token->tag_name == "XML", "root tag should be passed to tag parser");
    expect(result.token->value == "<number>1</number>", "root value should preserve inner XML");
}

void parses_xml_declaration_and_doctype_text() {
    const iiXml::writer::GetFile input;

    const iiXml::writer::get_file_result result =
        input.parse_xml("<?xml version=\"1.0\"?>\n<!DOCTYPE XML>\n<XML type=\"root\"><number>1</number></XML>");

    expect(result.status == iiXml::writer::get_file_status::parsed,
        "XML declaration and DOCTYPE should be stripped before tag parser");
    expect(!result.reason.empty(), "XML declaration and DOCTYPE should return non-empty reason");
    expect(result.token.has_value(), "XML declaration and DOCTYPE should return token");
    if (!result.token.has_value()) {
        return;
    }

    expect(result.token->tag_name == "XML", "root tag with attributes should parse");
    expect(result.token->value == "<number>1</number>", "root tag value should parse");
}

void parses_xml_declaration_then_arbitrary_doctype_text() {
    const iiXml::writer::GetFile input;

    const iiXml::writer::get_file_result result =
        input.parse_xml("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE ABCD>\n<ABCD><number>2</number></ABCD>");

    expect(result.status == iiXml::writer::get_file_status::parsed,
        "XML declaration followed by arbitrary DOCTYPE should parse");
    expect(!result.reason.empty(), "arbitrary DOCTYPE document should return non-empty reason");
    expect(result.token.has_value(), "arbitrary DOCTYPE document should return token");
    if (!result.token.has_value()) {
        return;
    }

    expect(result.token->tag_name == "ABCD", "arbitrary root tag should be passed to tag parser");
    expect(result.token->value == "<number>2</number>", "arbitrary root value should parse");
}

void parses_file_after_validation() {
    QTemporaryDir directory;
    expect(directory.isValid(), "temporary directory should be valid");
    if (!directory.isValid()) {
        return;
    }

    const QString path = directory.filePath("input.xml");
    write_file(path, "<!DOCTYPE XML>\n<XML><number>42</number></XML>");

    const iiXml::writer::GetFile input;
    const iiXml::writer::get_file_result result = input.parse_file(std::filesystem::path(path.toStdString()));

    expect(result.status == iiXml::writer::get_file_status::parsed,
        "valid XML file should parse");
    expect(!result.reason.empty(), "valid XML file should return non-empty reason");
    expect(result.token.has_value(), "valid XML file should return token");
    if (!result.token.has_value()) {
        return;
    }

    expect(result.token->tag_name == "XML", "file root tag should parse");
    expect(result.token->value == "<number>42</number>", "file root value should parse");
}

void parses_non_xml_extension_file_after_validation() {
    QTemporaryDir directory;
    expect(directory.isValid(), "temporary directory should be valid");
    if (!directory.isValid()) {
        return;
    }

    const QString path = directory.filePath("input.custom_payload");
    write_file(path, "<!DOCTYPE XML>\n<XML><number>77</number></XML>");

    const iiXml::writer::GetFile input;
    const iiXml::writer::get_file_result result = input.parse_file(std::filesystem::path(path.toStdString()));

    expect(result.status == iiXml::writer::get_file_status::parsed,
        "valid XML content should parse even when extension is not xml");
    expect(!result.reason.empty(), "non-xml extension file should return non-empty reason");
    expect(result.token.has_value(), "non-xml extension file should return token when content is valid XML");
    if (!result.token.has_value()) {
        return;
    }

    expect(result.token->tag_name == "XML", "non-xml extension file root tag should parse");
    expect(result.token->value == "<number>77</number>", "non-xml extension file root value should parse");
}

void parses_cross_nested_tags_after_validation() {
    const iiXml::writer::GetFile input;

    const iiXml::writer::get_file_result result =
        input.parse_xml("<!DOCTYPE XML>\n<XML><a><b></a></b></XML>");

    expect(result.status == iiXml::writer::get_file_status::parsed,
        "cross nested tags should parse after validation");
    expect(!result.reason.empty(), "cross nested tag input should return non-empty reason");
    expect(result.token.has_value(), "cross nested tag input should return token");
    if (!result.token.has_value()) {
        return;
    }

    expect(result.token->tag_name == "XML", "cross nested tag input should preserve root tag");
    expect(result.token->value == "<a><b></a></b>",
        "cross nested tag input should preserve raw root value");
}

void returns_invalid_xml_file_when_doctype_fails() {
    const iiXml::writer::GetFile input;

    const iiXml::writer::get_file_result result = input.parse_xml("<XML></XML>");

    expect(result.status == iiXml::writer::get_file_status::invalid_xml_file,
        "DOCTYPE failure should return invalid_xml_file");
    expect(!result.reason.empty(), "DOCTYPE failure should return non-empty reason");
    expect(!result.token.has_value(), "DOCTYPE failure should not return token");
}

void returns_invalid_tag_closure_when_validator_fails() {
    const iiXml::writer::GetFile input;

    const iiXml::writer::get_file_result result =
        input.parse_xml("<!DOCTYPE XML>\n<XML><number>1</XML>");

    expect(result.status == iiXml::writer::get_file_status::invalid_tag_closure,
        "tag closure failure should return invalid_tag_closure");
    expect(!result.reason.empty(), "tag closure failure should return non-empty reason");
    expect(!result.token.has_value(), "tag closure failure should not return token");
}

void returns_file_read_failed_for_missing_file() {
    const iiXml::writer::GetFile input;
    const std::filesystem::path path = std::filesystem::current_path() / "missing_get_file.xml";

    std::filesystem::remove(path);
    const iiXml::writer::get_file_result result = input.parse_file(path);

    expect(result.status == iiXml::writer::get_file_status::file_read_failed,
        "missing file should return file_read_failed");
    expect(!result.reason.empty(), "missing file should return non-empty reason");
}

} // namespace

int main() {
    parses_valid_doctype_xml_text();
    parses_xml_declaration_and_doctype_text();
    parses_xml_declaration_then_arbitrary_doctype_text();
    parses_file_after_validation();
    parses_non_xml_extension_file_after_validation();
    parses_cross_nested_tags_after_validation();
    returns_invalid_xml_file_when_doctype_fails();
    returns_invalid_tag_closure_when_validator_fails();
    returns_file_read_failed_for_missing_file();

    return failures == 0 ? 0 : 1;
}
