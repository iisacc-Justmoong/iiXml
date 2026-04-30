#include "iiXml.h"

#include <QTemporaryDir>
#include <QtGlobal>
#include <QString>
#include <QStringList>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

int failures = 0;
QStringList messages;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

void capture_debug(QtMsgType type, const QMessageLogContext&, const QString& message) {
    if (type == QtDebugMsg) {
        messages.append(message);
    }
}

bool saw(const QString& token) {
    for (const QString& message : messages) {
        if (message.contains(token)) {
            return true;
        }
    }

    return false;
}

void write_file(const QString& path, const std::string& content) {
    std::ofstream file(path.toStdString(), std::ios::binary);
    file << content;
}

void exercises_debug_logged_objects() {
    QTemporaryDir directory;
    expect(directory.isValid(), "temporary directory should be valid");
    if (!directory.isValid()) {
        return;
    }

    const QString parser_file = directory.filePath("parser.xml");
    const QString validated_file = directory.filePath("validated.custom");
    write_file(parser_file, "<number>42</number>");
    write_file(validated_file, "<!DOCTYPE XML>\n<XML><number>42</number></XML>");

    OpenTag open_tag;
    ClosedTag closed_tag;
    InlineProperties inline_properties;

    iiXml::parser::tag_parser parser;
    const auto parsed_tag = parser.parse("<number>42</number>");
    parser.parseTag("<number>42</number>");

    iiXml::parser::FileParser file_parser;
    const auto parsed_file =
        file_parser.parse_file(std::filesystem::path(parser_file.toStdString()));
    file_parser.parseFile(parser_file);

    iiXml::elements::DOCTYPE doctype;
    const auto doctype_result = doctype.match_top("<!DOCTYPE XML>\n<XML></XML>");
    const auto explicit_doctype_result =
        doctype.match_top_result("<!DOCTYPE XML>\n<XML></XML>");
    const auto doctype_match = doctype.match_top_match("<!DOCTYPE XML>\n<XML></XML>");
    const bool is_doctype = doctype.is_top_doctype("<!DOCTYPE XML>");
    const bool is_xml_declaration = doctype.is_top_xml_declaration("<?xml version=\"1.0\"?>");
    doctype.matchTop("<!DOCTYPE XML>\n<XML></XML>");

    iiXml::writer::InputValidator validator;
    const auto validation = validator.validate("<!DOCTYPE XML>\n<XML><number>1</number></XML>");
    const auto validation_result =
        validator.validate_result("<!DOCTYPE XML>\n<XML><number>1</number></XML>");
    const bool valid_tag_closure =
        validator.has_valid_tag_closure("<XML><number>1</number></XML>");
    validator.validateInput("<!DOCTYPE XML>\n<XML><number>1</number></XML>");

    iiXml::writer::GetFile get_file;
    const auto parsed_xml = get_file.parse_xml("<!DOCTYPE XML>\n<XML><number>1</number></XML>");
    const auto parsed_validated_file =
        get_file.parse_file(std::filesystem::path(validated_file.toStdString()));
    get_file.readXml("<!DOCTYPE XML>\n<XML><number>1</number></XML>");
    get_file.readFile(validated_file);

    iiXml::writer::GetStringToken string_token;
    const auto parsed_string =
        string_token.parse_string("<!DOCTYPE XML>\n<XML><number>1</number></XML>");
    string_token.readString("<!DOCTYPE XML>\n<XML><number>1</number></XML>");

    const auto rejected_tag = parser.parse("<number>42</text>");
    parser.parseTag("<number>42</text>");

    const QString missing_file = directory.filePath("missing.xml");
    const auto rejected_file =
        file_parser.parse_file(std::filesystem::path(missing_file.toStdString()));
    file_parser.parseFile(missing_file);

    const auto rejected_doctype = doctype.match_top("<XML></XML>");
    const auto rejected_doctype_match = doctype.match_top_match("<XML></XML>");
    const bool rejected_is_doctype = doctype.is_top_doctype("<XML></XML>");
    const bool rejected_is_xml_declaration = doctype.is_top_xml_declaration("<XML></XML>");
    doctype.matchTop("<XML></XML>");

    const auto rejected_validation = validator.validate("<XML></XML>");
    const auto rejected_validation_result = validator.validate_result("<XML></XML>");
    const bool rejected_tag_closure = validator.has_valid_tag_closure("<XML><number>1</XML>");
    validator.validateInput("<XML></XML>");

    const auto rejected_xml = get_file.parse_xml("<XML></XML>");
    const auto rejected_missing_file =
        get_file.parse_file(std::filesystem::path(missing_file.toStdString()));
    get_file.readXml("<XML></XML>");
    get_file.readFile(missing_file);

    const auto rejected_string = string_token.parse_string("<number>1</number>");
    string_token.readString("<number>1</number>");

    launch();

    expect(parsed_tag.has_value(), "tag parser should parse in debug log test");
    expect(parsed_file.has_value(), "file parser should parse in debug log test");
    expect(doctype_result.match.has_value(), "doctype should match in debug log test");
    expect(explicit_doctype_result.match.has_value(), "doctype result should match in debug log test");
    expect(doctype_match.has_value(), "doctype match should exist in debug log test");
    expect(is_doctype, "doctype predicate should be true in debug log test");
    expect(is_xml_declaration, "xml declaration predicate should be true in debug log test");
    expect(validation == iiXml::writer::validation_exit::valid,
        "validation should be valid in debug log test");
    expect(validation_result.exit == iiXml::writer::validation_exit::valid,
        "validation result should be valid in debug log test");
    expect(valid_tag_closure, "tag closure should be valid in debug log test");
    expect(parsed_xml.status == iiXml::writer::get_file_status::parsed,
        "GetFile XML should parse in debug log test");
    expect(parsed_validated_file.status == iiXml::writer::get_file_status::parsed,
        "GetFile file should parse in debug log test");
    expect(parsed_string.status == iiXml::writer::get_string_token_status::parsed,
        "GetStringToken should parse in debug log test");
    expect(!rejected_tag.has_value(), "tag parser should reject invalid tag in debug log test");
    expect(!rejected_file.has_value(), "FileParser should reject missing file in debug log test");
    expect(!rejected_doctype.match.has_value(), "DOCTYPE should reject non declaration in debug log test");
    expect(!rejected_doctype_match.has_value(),
        "DOCTYPE match should reject non declaration in debug log test");
    expect(!rejected_is_doctype, "DOCTYPE predicate should reject non declaration in debug log test");
    expect(!rejected_is_xml_declaration,
        "XML declaration predicate should reject non declaration in debug log test");
    expect(rejected_validation == iiXml::writer::validation_exit::invalid_xml_file,
        "InputValidator should reject invalid XML in debug log test");
    expect(rejected_validation_result.exit == iiXml::writer::validation_exit::invalid_xml_file,
        "InputValidator result should reject invalid XML in debug log test");
    expect(!rejected_tag_closure, "InputValidator should reject invalid closure in debug log test");
    expect(rejected_xml.status == iiXml::writer::get_file_status::invalid_xml_file,
        "GetFile should reject invalid XML in debug log test");
    expect(rejected_missing_file.status == iiXml::writer::get_file_status::file_read_failed,
        "GetFile should reject missing file in debug log test");
    expect(rejected_string.status == iiXml::writer::get_string_token_status::invalid_xml_file,
        "GetStringToken should reject invalid XML in debug log test");
}

} // namespace

int main() {
    const QtMessageHandler previous_handler = qInstallMessageHandler(capture_debug);
    exercises_debug_logged_objects();
    qInstallMessageHandler(previous_handler);

    expect(saw("OpenTag::OpenTag"), "OpenTag constructor should log with qDebug");
    expect(saw("ClosedTag::ClosedTag"), "ClosedTag constructor should log with qDebug");
    expect(saw("InlineProperties::InlineProperties"),
        "InlineProperties constructor should log with qDebug");
    expect(saw("iiXml::parser::tag_parser::tag_parser"),
        "tag_parser constructor should log with qDebug");
    expect(saw("iiXml::parser::tag_parser::parse"),
        "tag_parser::parse should log with qDebug");
    expect(saw("iiXml::parser::tag_parser::parse failed"),
        "tag_parser::parse failure should log with qDebug");
    expect(saw("iiXml::parser::FileParser::FileParser"),
        "FileParser constructor should log with qDebug");
    expect(saw("iiXml::parser::tag_parser::parseTag"),
        "tag_parser::parseTag should log with qDebug");
    expect(saw("iiXml::parser::tag_parser::parseTag failed"),
        "tag_parser::parseTag failure should log with qDebug");
    expect(saw("iiXml::parser::FileParser::parse_file"),
        "FileParser::parse_file should log with qDebug");
    expect(saw("iiXml::parser::FileParser::parse_file failed"),
        "FileParser::parse_file failure should log with qDebug");
    expect(saw("iiXml::parser::FileParser::parseFile"),
        "FileParser::parseFile should log with qDebug");
    expect(saw("iiXml::parser::FileParser::parseFile failed"),
        "FileParser::parseFile failure should log with qDebug");
    expect(saw("iiXml::elements::DOCTYPE::DOCTYPE"),
        "DOCTYPE constructor should log with qDebug");
    expect(saw("iiXml::elements::DOCTYPE::match_top"),
        "DOCTYPE::match_top should log with qDebug");
    expect(saw("iiXml::elements::DOCTYPE::match_top failed"),
        "DOCTYPE::match_top failure should log with qDebug");
    expect(saw("iiXml::elements::DOCTYPE::match_top_result"),
        "DOCTYPE::match_top_result should log with qDebug");
    expect(saw("iiXml::elements::DOCTYPE::match_top_match"),
        "DOCTYPE::match_top_match should log with qDebug");
    expect(saw("iiXml::elements::DOCTYPE::match_top_match failed"),
        "DOCTYPE::match_top_match failure should log with qDebug");
    expect(saw("iiXml::elements::DOCTYPE::is_top_doctype"),
        "DOCTYPE::is_top_doctype should log with qDebug");
    expect(saw("iiXml::elements::DOCTYPE::is_top_xml_declaration"),
        "DOCTYPE::is_top_xml_declaration should log with qDebug");
    expect(saw("iiXml::elements::DOCTYPE::matchTop"),
        "DOCTYPE::matchTop should log with qDebug");
    expect(saw("iiXml::elements::DOCTYPE::matchTop rejected"),
        "DOCTYPE::matchTop rejection should log with qDebug");
    expect(saw("iiXml::writer::InputValidator::InputValidator"),
        "InputValidator constructor should log with qDebug");
    expect(saw("iiXml::writer::InputValidator::validate "),
        "InputValidator::validate should log with qDebug");
    expect(saw("iiXml::writer::InputValidator::validate failed"),
        "InputValidator::validate failure should log with qDebug");
    expect(saw("iiXml::writer::InputValidator::validate_result"),
        "InputValidator::validate_result should log with qDebug");
    expect(saw("iiXml::writer::InputValidator::has_valid_tag_closure"),
        "InputValidator::has_valid_tag_closure should log with qDebug");
    expect(saw("iiXml::writer::InputValidator::has_valid_tag_closure failed"),
        "InputValidator::has_valid_tag_closure failure should log with qDebug");
    expect(saw("iiXml::writer::InputValidator::validateInput"),
        "InputValidator::validateInput should log with qDebug");
    expect(saw("iiXml::writer::InputValidator::validateInput failed"),
        "InputValidator::validateInput failure should log with qDebug");
    expect(saw("iiXml::writer::GetFile::GetFile"),
        "GetFile constructor should log with qDebug");
    expect(saw("iiXml::writer::GetFile::parse_xml"),
        "GetFile::parse_xml should log with qDebug");
    expect(saw("iiXml::writer::GetFile::parse_xml failed"),
        "GetFile::parse_xml failure should log with qDebug");
    expect(saw("iiXml::writer::GetFile::parse_file"),
        "GetFile::parse_file should log with qDebug");
    expect(saw("iiXml::writer::GetFile::parse_file failed"),
        "GetFile::parse_file failure should log with qDebug");
    expect(saw("iiXml::writer::GetFile::readXml"),
        "GetFile::readXml should log with qDebug");
    expect(saw("iiXml::writer::GetFile::readXml failed"),
        "GetFile::readXml failure should log with qDebug");
    expect(saw("iiXml::writer::GetFile::readFile"),
        "GetFile::readFile should log with qDebug");
    expect(saw("iiXml::writer::GetFile::readFile failed"),
        "GetFile::readFile failure should log with qDebug");
    expect(saw("iiXml::writer::GetStringToken::GetStringToken"),
        "GetStringToken constructor should log with qDebug");
    expect(saw("iiXml::writer::GetStringToken::parse_string"),
        "GetStringToken::parse_string should log with qDebug");
    expect(saw("iiXml::writer::GetStringToken::parse_string failed"),
        "GetStringToken::parse_string failure should log with qDebug");
    expect(saw("iiXml::writer::GetStringToken::readString"),
        "GetStringToken::readString should log with qDebug");
    expect(saw("iiXml::writer::GetStringToken::readString failed"),
        "GetStringToken::readString failure should log with qDebug");
    expect(saw("iiXml::launch library launch"), "launch should log library launch with qDebug");

    return failures == 0 ? 0 : 1;
}
