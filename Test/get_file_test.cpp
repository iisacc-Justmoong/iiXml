#include <iiXml>

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
    const iiXml::Writer::GetFile input;

    const iiXml::Writer::GetFileResult result =
        input.ParseXml("<!Doctype XML>\n<XML><number>1</number></XML>");

    expect(result.Status == iiXml::Writer::GetFileStatus::Parsed,
        "valid Doctype XML text should parse");
    expect(!result.Reason.empty(), "valid Doctype XML text should return non-empty reason");
    expect(result.Token.has_value(), "valid Doctype XML text should return token");
    if (!result.Token.has_value()) {
        return;
    }

    expect(result.Token->TagName == "XML", "root tag should be passed to tag parser");
    expect(result.Token->Value == "<number>1</number>", "root value should preserve inner XML");
}

void parses_xml_declaration_and_doctype_text() {
    const iiXml::Writer::GetFile input;

    const iiXml::Writer::GetFileResult result =
        input.ParseXml("<?xml version=\"1.0\"?>\n<!Doctype XML>\n<XML type=\"root\"><number>1</number></XML>");

    expect(result.Status == iiXml::Writer::GetFileStatus::Parsed,
        "XML declaration and Doctype should be stripped before tag parser");
    expect(!result.Reason.empty(), "XML declaration and Doctype should return non-empty reason");
    expect(result.Token.has_value(), "XML declaration and Doctype should return token");
    if (!result.Token.has_value()) {
        return;
    }

    expect(result.Token->TagName == "XML", "root tag with attributes should parse");
    expect(result.Token->Value == "<number>1</number>", "root tag value should parse");
}

void parses_xml_declaration_then_arbitrary_doctype_text() {
    const iiXml::Writer::GetFile input;

    const iiXml::Writer::GetFileResult result =
        input.ParseXml("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!Doctype ABCD>\n<ABCD><number>2</number></ABCD>");

    expect(result.Status == iiXml::Writer::GetFileStatus::Parsed,
        "XML declaration followed by arbitrary Doctype should parse");
    expect(!result.Reason.empty(), "arbitrary Doctype document should return non-empty reason");
    expect(result.Token.has_value(), "arbitrary Doctype document should return token");
    if (!result.Token.has_value()) {
        return;
    }

    expect(result.Token->TagName == "ABCD", "arbitrary root tag should be passed to tag parser");
    expect(result.Token->Value == "<number>2</number>", "arbitrary root value should parse");
}

void parses_file_after_validation() {
    QTemporaryDir directory;
    expect(directory.isValid(), "temporary directory should be valid");
    if (!directory.isValid()) {
        return;
    }

    const QString path = directory.filePath("input.xml");
    write_file(path, "<!Doctype XML>\n<XML><number>42</number></XML>");

    const iiXml::Writer::GetFile input;
    const iiXml::Writer::GetFileResult result = input.ParseFile(std::filesystem::path(path.toStdString()));

    expect(result.Status == iiXml::Writer::GetFileStatus::Parsed,
        "valid XML file should parse");
    expect(!result.Reason.empty(), "valid XML file should return non-empty reason");
    expect(result.Token.has_value(), "valid XML file should return token");
    if (!result.Token.has_value()) {
        return;
    }

    expect(result.Token->TagName == "XML", "file root tag should parse");
    expect(result.Token->Value == "<number>42</number>", "file root value should parse");
}

void parses_non_xml_extension_file_after_validation() {
    QTemporaryDir directory;
    expect(directory.isValid(), "temporary directory should be valid");
    if (!directory.isValid()) {
        return;
    }

    const QString path = directory.filePath("input.custom_payload");
    write_file(path, "<!Doctype XML>\n<XML><number>77</number></XML>");

    const iiXml::Writer::GetFile input;
    const iiXml::Writer::GetFileResult result = input.ParseFile(std::filesystem::path(path.toStdString()));

    expect(result.Status == iiXml::Writer::GetFileStatus::Parsed,
        "valid XML content should parse even when extension is not xml");
    expect(!result.Reason.empty(), "non-xml extension file should return non-empty reason");
    expect(result.Token.has_value(), "non-xml extension file should return token when content is valid XML");
    if (!result.Token.has_value()) {
        return;
    }

    expect(result.Token->TagName == "XML", "non-xml extension file root tag should parse");
    expect(result.Token->Value == "<number>77</number>", "non-xml extension file root value should parse");
}

void parses_cross_nested_tags_after_validation() {
    const iiXml::Writer::GetFile input;

    const iiXml::Writer::GetFileResult result =
        input.ParseXml("<!Doctype XML>\n<XML><a><b></a></b></XML>");

    expect(result.Status == iiXml::Writer::GetFileStatus::Parsed,
        "cross nested tags should parse after validation");
    expect(!result.Reason.empty(), "cross nested tag input should return non-empty reason");
    expect(result.Token.has_value(), "cross nested tag input should return token");
    if (!result.Token.has_value()) {
        return;
    }

    expect(result.Token->TagName == "XML", "cross nested tag input should preserve root tag");
    expect(result.Token->Value == "<a><b></a></b>",
        "cross nested tag input should preserve raw root value");
}

void returns_invalid_xml_file_when_doctype_fails() {
    const iiXml::Writer::GetFile input;

    const iiXml::Writer::GetFileResult result = input.ParseXml("<XML></XML>");

    expect(result.Status == iiXml::Writer::GetFileStatus::InvalidXmlFile,
        "Doctype failure should return invalid_xml_file");
    expect(!result.Reason.empty(), "Doctype failure should return non-empty reason");
    expect(!result.Token.has_value(), "Doctype failure should not return token");
}

void returns_invalid_tag_closure_when_validator_fails() {
    const iiXml::Writer::GetFile input;

    const iiXml::Writer::GetFileResult result =
        input.ParseXml("<!Doctype XML>\n<XML><number>1</XML>");

    expect(result.Status == iiXml::Writer::GetFileStatus::InvalidTagClosure,
        "tag closure failure should return invalid_tag_closure");
    expect(!result.Reason.empty(), "tag closure failure should return non-empty reason");
    expect(!result.Token.has_value(), "tag closure failure should not return token");
}

void returns_file_read_failed_for_missing_file() {
    const iiXml::Writer::GetFile input;
    const std::filesystem::path path = std::filesystem::current_path() / "missing_get_file.xml";

    std::filesystem::remove(path);
    const iiXml::Writer::GetFileResult result = input.ParseFile(path);

    expect(result.Status == iiXml::Writer::GetFileStatus::FileReadFailed,
        "missing file should return file_read_failed");
    expect(!result.Reason.empty(), "missing file should return non-empty reason");
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
