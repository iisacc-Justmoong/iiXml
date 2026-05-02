#include <iiXml>

#include <QTemporaryDir>
#include <QtGlobal>
#include <QString>
#include <QStringList>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

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
    write_file(validated_file, "<!Doctype XML>\n<XML><number>42</number></XML>");

    OpenTag open_tag;
    std::vector<std::string> open_tags{"a", "b"};
    const bool closed_cross_tag = open_tag.CloseOpenTag(open_tags, "a");
    const auto preserved_open_tags = open_tag.ParseOpenTags("<a><b></a></b>");
    ClosedTag closed_tag;
    const bool is_immediate_closed_tag = closed_tag.IsImmediateClosedTag("</flag>");
    const auto matched_closed_tag = closed_tag.MatchImmediate("</flag>");
    closed_tag.ParseClosedTag("</flag>");
    const auto rejected_closed_tag = closed_tag.MatchImmediate("<flag></flag>");
    closed_tag.ParseClosedTag("<flag></flag>");
    InlineProperties inline_properties;
    const auto parsed_inline_properties =
        inline_properties.Parse("<resource title=\"text\" count=1 enabled=true>");

    iiXml::Parser::TagParser parser;
    const auto parsed_tag = parser.Parse("<number>42</number>");
    const auto parsed_tag_result = parser.ParseResult("<number>42</number>");
    const auto parsed_tags = parser.ParseAll("<a><b></a></b>");
    const auto parsed_tags_result = parser.ParseAllResult("<a><b></a></b>");
    const auto parsed_tag_document = parser.ParseAllDocument("<a><b></a></b>");
    const auto parsed_tag_document_result = parser.ParseAllDocumentResult("<a><b></a></b>");
    std::string_view parsed_tag_document_value;
    if (parsed_tag_document.has_value() && !parsed_tag_document->Nodes.empty()) {
        parsed_tag_document_value = parsed_tag_document->ValueView(parsed_tag_document->Nodes[0]);
    }
    parser.ParseTag("<number>42</number>");

    iiXml::Parser::FileParser file_parser;
    const auto parsed_file =
        file_parser.ParseFile(std::filesystem::path(parser_file.toStdString()));
    file_parser.ParseFileInput(parser_file);

    iiXml::Elements::Doctype doctype;
    const auto DoctypeResult = doctype.MatchTop("<!Doctype XML>\n<XML></XML>");
    const auto explicit_doctype_result =
        doctype.MatchTopResult("<!Doctype XML>\n<XML></XML>");
    const auto DoctypeMatch = doctype.MatchTopMatch("<!Doctype XML>\n<XML></XML>");
    const bool is_doctype = doctype.IsTopDoctype("<!Doctype XML>");
    const bool is_xml_declaration = doctype.IsTopXmlDeclaration("<?xml version=\"1.0\"?>");
    doctype.MatchTopInput("<!Doctype XML>\n<XML></XML>");

    iiXml::Writer::InputValidator validator;
    const auto validation = validator.Validate("<!Doctype XML>\n<XML><number>1</number></XML>");
    const auto ValidationResult =
        validator.ValidateResult("<!Doctype XML>\n<XML><number>1</number></XML>");
    const bool valid_tag_closure =
        validator.HasValidTagClosure("<XML><number>1</number></XML>");
    validator.ValidateInput("<!Doctype XML>\n<XML><number>1</number></XML>");

    iiXml::Writer::GetFile get_file;
    const auto parsed_xml = get_file.ParseXml("<!Doctype XML>\n<XML><number>1</number></XML>");
    const auto parsed_validated_file =
        get_file.ParseFile(std::filesystem::path(validated_file.toStdString()));
    get_file.ReadXml("<!Doctype XML>\n<XML><number>1</number></XML>");
    get_file.ReadFile(validated_file);

    iiXml::Writer::GetStringToken string_token;
    const auto parsed_string =
        string_token.ParseString("<!Doctype XML>\n<XML><number>1</number></XML>");
    string_token.ReadString("<!Doctype XML>\n<XML><number>1</number></XML>");

    const auto rejected_tag = parser.Parse("<number>42</text>");
    const auto rejected_tag_result = parser.ParseResult("<number>42</text>");
    const auto rejected_tag_document = parser.ParseAllDocument("<root><child></root>");
    const auto rejected_tag_document_result =
        parser.ParseAllDocumentResult("<root><child></root>");
    parser.ParseTag("<number>42</text>");

    const QString missing_file = directory.filePath("missing.xml");
    const auto rejected_file =
        file_parser.ParseFile(std::filesystem::path(missing_file.toStdString()));
    file_parser.ParseFileInput(missing_file);

    const auto rejected_doctype = doctype.MatchTop("<XML></XML>");
    const auto rejected_doctype_match = doctype.MatchTopMatch("<XML></XML>");
    const bool rejected_is_doctype = doctype.IsTopDoctype("<XML></XML>");
    const bool rejected_is_xml_declaration = doctype.IsTopXmlDeclaration("<XML></XML>");
    doctype.MatchTopInput("<XML></XML>");

    const auto rejected_validation = validator.Validate("<XML></XML>");
    const auto rejected_validation_result = validator.ValidateResult("<XML></XML>");
    const bool rejected_tag_closure = validator.HasValidTagClosure("<XML><number>1</XML>");
    validator.ValidateInput("<XML></XML>");

    const auto rejected_xml = get_file.ParseXml("<XML></XML>");
    const auto rejected_missing_file =
        get_file.ParseFile(std::filesystem::path(missing_file.toStdString()));
    get_file.ReadXml("<XML></XML>");
    get_file.ReadFile(missing_file);

    const auto rejected_string = string_token.ParseString("<number>1</number>");
    string_token.ReadString("<number>1</number>");

    Launch();

    expect(parsed_tag.has_value(), "tag parser should parse in debug log test");
    expect(parsed_tag_result.Status == iiXml::Parser::TagParseStatus::Parsed,
        "tag parser result should parse in debug log test");
    expect(parsed_tags.has_value(), "tag parser should parse all tags in debug log test");
    expect(parsed_tags.has_value() && parsed_tags->size() == 2,
        "tag parser should preserve both cross nested tags in debug log test");
    expect(parsed_tags_result.Status == iiXml::Parser::TagTreeParseStatus::Parsed,
        "tag parser tree result should parse in debug log test");
    expect(parsed_tag_document.has_value(),
        "tag parser document should parse in debug log test");
    expect(parsed_tag_document_result.Status == iiXml::Parser::TagTreeParseStatus::Parsed,
        "tag parser document result should parse in debug log test");
    expect(parsed_tag_document_value == "<b>",
        "tag parser document should expose value views in debug log test");
    expect(closed_cross_tag, "OpenTag should close cross nested tag in debug log test");
    expect(preserved_open_tags.has_value(),
        "OpenTag should preserve cross nested tags in debug log test");
    expect(is_immediate_closed_tag, "ClosedTag should detect immediate closed tag in debug log test");
    expect(matched_closed_tag.has_value(), "ClosedTag should match immediate tag in debug log test");
    expect(!rejected_closed_tag.has_value(),
        "ClosedTag should reject non immediate closed tag in debug log test");
    expect(parsed_inline_properties.has_value(),
        "InlineProperties should parse in debug log test");
    expect(parsed_file.has_value(), "file parser should parse in debug log test");
    expect(DoctypeResult.Match.has_value(), "doctype should match in debug log test");
    expect(explicit_doctype_result.Match.has_value(), "doctype result should match in debug log test");
    expect(DoctypeMatch.has_value(), "doctype match should exist in debug log test");
    expect(is_doctype, "doctype predicate should be true in debug log test");
    expect(is_xml_declaration, "xml declaration predicate should be true in debug log test");
    expect(validation == iiXml::Writer::ValidationExit::Valid,
        "validation should be valid in debug log test");
    expect(ValidationResult.Exit == iiXml::Writer::ValidationExit::Valid,
        "validation result should be valid in debug log test");
    expect(valid_tag_closure, "tag closure should be valid in debug log test");
    expect(parsed_xml.Status == iiXml::Writer::GetFileStatus::Parsed,
        "GetFile XML should parse in debug log test");
    expect(parsed_validated_file.Status == iiXml::Writer::GetFileStatus::Parsed,
        "GetFile file should parse in debug log test");
    expect(parsed_string.Status == iiXml::Writer::GetStringTokenStatus::Parsed,
        "GetStringToken should parse in debug log test");
    expect(!rejected_tag.has_value(), "tag parser should reject invalid tag in debug log test");
    expect(rejected_tag_result.Status == iiXml::Parser::TagParseStatus::ClosingTagMismatch,
        "tag parser result should reject invalid tag in debug log test");
    expect(!rejected_tag_document.has_value(),
        "tag parser document should reject invalid tree in debug log test");
    expect(rejected_tag_document_result.Status == iiXml::Parser::TagTreeParseStatus::OpenTagParserRejected,
        "tag parser document result should reject invalid tree in debug log test");
    expect(!rejected_file.has_value(), "FileParser should reject missing file in debug log test");
    expect(!rejected_doctype.Match.has_value(), "Doctype should reject non declaration in debug log test");
    expect(!rejected_doctype_match.has_value(),
        "Doctype match should reject non declaration in debug log test");
    expect(!rejected_is_doctype, "Doctype predicate should reject non declaration in debug log test");
    expect(!rejected_is_xml_declaration,
        "XML declaration predicate should reject non declaration in debug log test");
    expect(rejected_validation == iiXml::Writer::ValidationExit::InvalidXmlFile,
        "InputValidator should reject invalid XML in debug log test");
    expect(rejected_validation_result.Exit == iiXml::Writer::ValidationExit::InvalidXmlFile,
        "InputValidator result should reject invalid XML in debug log test");
    expect(!rejected_tag_closure, "InputValidator should reject invalid closure in debug log test");
    expect(rejected_xml.Status == iiXml::Writer::GetFileStatus::InvalidXmlFile,
        "GetFile should reject invalid XML in debug log test");
    expect(rejected_missing_file.Status == iiXml::Writer::GetFileStatus::FileReadFailed,
        "GetFile should reject missing file in debug log test");
    expect(rejected_string.Status == iiXml::Writer::GetStringTokenStatus::InvalidXmlFile,
        "GetStringToken should reject invalid XML in debug log test");
}

} // namespace

int main() {
    const QtMessageHandler previous_handler = qInstallMessageHandler(capture_debug);
    exercises_debug_logged_objects();
    qInstallMessageHandler(previous_handler);

    expect(saw("OpenTag::OpenTag"), "OpenTag constructor should log with qDebug");
    expect(saw("iiXml::Elements::OpenTag::CloseOpenTag"),
        "OpenTag::CloseOpenTag should log with qDebug");
    expect(saw("iiXml::Elements::OpenTag::ParseOpenTags"),
        "OpenTag::ParseOpenTags should log with qDebug");
    expect(saw("ClosedTag::ClosedTag"), "ClosedTag constructor should log with qDebug");
    expect(saw("iiXml::Elements::ClosedTag::IsImmediateClosedTag"),
        "ClosedTag::IsImmediateClosedTag should log with qDebug");
    expect(saw("iiXml::Elements::ClosedTag::MatchImmediate"),
        "ClosedTag::MatchImmediate should log with qDebug");
    expect(saw("iiXml::Elements::ClosedTag::MatchImmediate rejected"),
        "ClosedTag::MatchImmediate rejection should log with qDebug");
    expect(saw("iiXml::Elements::ClosedTag::ParseClosedTag"),
        "ClosedTag::ParseClosedTag should log with qDebug");
    expect(saw("iiXml::Elements::ClosedTag::ParseClosedTag rejected"),
        "ClosedTag::ParseClosedTag rejection should log with qDebug");
    expect(saw("InlineProperties::InlineProperties"),
        "InlineProperties constructor should log with qDebug");
    expect(saw("iiXml::Elements::InlineProperties::Parse"),
        "InlineProperties::Parse should log with qDebug");
    expect(saw("iiXml::Parser::TagParser::TagParser"),
        "TagParser constructor should log with qDebug");
    expect(saw("iiXml::Parser::TagParser::Parse"),
        "TagParser::Parse should log with qDebug");
    expect(saw("iiXml::Parser::TagParser::ParseResult"),
        "TagParser::ParseResult should log with qDebug");
    expect(saw("iiXml::Parser::TagParser::ParseAll"),
        "TagParser::ParseAll should log with qDebug");
    expect(saw("iiXml::Parser::TagParser::ParseAllResult"),
        "TagParser::ParseAllResult should log with qDebug");
    expect(saw("iiXml::Parser::TagParser::ParseAllDocument"),
        "TagParser::ParseAllDocument should log with qDebug");
    expect(saw("iiXml::Parser::TagParser::ParseAllDocumentResult"),
        "TagParser::ParseAllDocumentResult should log with qDebug");
    expect(saw("iiXml::Parser::TagParser::ParseAllDocumentResult failed"),
        "TagParser::ParseAllDocumentResult failure should log with qDebug");
    expect(saw("iiXml::Parser::TagDocument::ValueView"),
        "TagDocument::ValueView should log with qDebug");
    expect(saw("iiXml::Parser::TagParser::Parse failed"),
        "TagParser::Parse failure should log with qDebug");
    expect(saw("iiXml::Parser::TagParser::ParseResult failed"),
        "TagParser::ParseResult failure should log with qDebug");
    expect(saw("iiXml::Parser::FileParser::FileParser"),
        "FileParser constructor should log with qDebug");
    expect(saw("iiXml::Parser::TagParser::ParseTag"),
        "TagParser::ParseTag should log with qDebug");
    expect(saw("iiXml::Parser::TagParser::ParseTag failed"),
        "TagParser::ParseTag failure should log with qDebug");
    expect(saw("iiXml::Parser::FileParser::ParseFileInput"),
        "FileParser::ParseFile should log with qDebug");
    expect(saw("iiXml::Parser::FileParser::ParseFileInput failed"),
        "FileParser::ParseFile failure should log with qDebug");
    expect(saw("iiXml::Parser::FileParser::ParseFile"),
        "FileParser::ParseFile should log with qDebug");
    expect(saw("iiXml::Parser::FileParser::ParseFile failed"),
        "FileParser::ParseFile failure should log with qDebug");
    expect(saw("iiXml::Elements::Doctype::Doctype"),
        "Doctype constructor should log with qDebug");
    expect(saw("iiXml::Elements::Doctype::MatchTopInput"),
        "Doctype::MatchTopInput should log with qDebug");
    expect(saw("iiXml::Elements::Doctype::MatchTopInput rejected"),
        "Doctype::MatchTopInput rejection should log with qDebug");
    expect(saw("iiXml::Elements::Doctype::MatchTopResult"),
        "Doctype::MatchTopResult should log with qDebug");
    expect(saw("iiXml::Elements::Doctype::MatchTopMatch"),
        "Doctype::MatchTopMatch should log with qDebug");
    expect(saw("iiXml::Elements::Doctype::MatchTopMatch failed"),
        "Doctype::MatchTopMatch failure should log with qDebug");
    expect(saw("iiXml::Elements::Doctype::IsTopDoctype"),
        "Doctype::IsTopDoctype should log with qDebug");
    expect(saw("iiXml::Elements::Doctype::IsTopXmlDeclaration"),
        "Doctype::IsTopXmlDeclaration should log with qDebug");
    expect(saw("iiXml::Elements::Doctype::MatchTopInput"),
        "Doctype::MatchTopInput should log with qDebug");
    expect(saw("iiXml::Elements::Doctype::MatchTopInput rejected"),
        "Doctype::MatchTopInput rejection should log with qDebug");
    expect(saw("iiXml::Writer::InputValidator::InputValidator"),
        "InputValidator constructor should log with qDebug");
    expect(saw("iiXml::Writer::InputValidator::Validate "),
        "InputValidator::Validate should log with qDebug");
    expect(saw("iiXml::Writer::InputValidator::Validate failed"),
        "InputValidator::Validate failure should log with qDebug");
    expect(saw("iiXml::Writer::InputValidator::ValidateResult"),
        "InputValidator::ValidateResult should log with qDebug");
    expect(saw("iiXml::Writer::InputValidator::HasValidTagClosure"),
        "InputValidator::HasValidTagClosure should log with qDebug");
    expect(saw("iiXml::Writer::InputValidator::HasValidTagClosure failed"),
        "InputValidator::HasValidTagClosure failure should log with qDebug");
    expect(saw("iiXml::Writer::InputValidator::ValidateInput"),
        "InputValidator::ValidateInput should log with qDebug");
    expect(saw("iiXml::Writer::InputValidator::ValidateInput failed"),
        "InputValidator::ValidateInput failure should log with qDebug");
    expect(saw("iiXml::Writer::GetFile::GetFile"),
        "GetFile constructor should log with qDebug");
    expect(saw("iiXml::Writer::GetFile::ParseXml"),
        "GetFile::ParseXml should log with qDebug");
    expect(saw("iiXml::Writer::GetFile::ParseXml failed"),
        "GetFile::ParseXml failure should log with qDebug");
    expect(saw("iiXml::Writer::GetFile::ParseFile"),
        "GetFile::ParseFile should log with qDebug");
    expect(saw("iiXml::Writer::GetFile::ParseFile failed"),
        "GetFile::ParseFile failure should log with qDebug");
    expect(saw("iiXml::Writer::GetFile::ReadXml"),
        "GetFile::ReadXml should log with qDebug");
    expect(saw("iiXml::Writer::GetFile::ReadXml failed"),
        "GetFile::ReadXml failure should log with qDebug");
    expect(saw("iiXml::Writer::GetFile::ReadFile"),
        "GetFile::ReadFile should log with qDebug");
    expect(saw("iiXml::Writer::GetFile::ReadFile failed"),
        "GetFile::ReadFile failure should log with qDebug");
    expect(saw("iiXml::Writer::GetStringToken::GetStringToken"),
        "GetStringToken constructor should log with qDebug");
    expect(saw("iiXml::Writer::GetStringToken::ParseString"),
        "GetStringToken::ParseString should log with qDebug");
    expect(saw("iiXml::Writer::GetStringToken::ParseString failed"),
        "GetStringToken::ParseString failure should log with qDebug");
    expect(saw("iiXml::Writer::GetStringToken::ReadString"),
        "GetStringToken::ReadString should log with qDebug");
    expect(saw("iiXml::Writer::GetStringToken::ReadString failed"),
        "GetStringToken::ReadString failure should log with qDebug");
    expect(saw("iiXml::Launch library Launch"), "Launch should log library Launch with qDebug");

    return failures == 0 ? 0 : 1;
}
