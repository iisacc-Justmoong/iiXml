#include <iiXml>

#include <QObject>
#include <QTemporaryDir>
#include <QString>

#include <fstream>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

void writes_file(const QString& path, const std::string& content) {
    std::ofstream file(path.toStdString(), std::ios::binary);
    file << content;
}

void tag_parser_emits_parsed_signal() {
    iiXml::Parser::TagParser parser;
    bool emitted = false;
    QString TagName;
    QString value;

    QObject::connect(&parser, &iiXml::Parser::TagParser::TagParsed,
        [&](const QString& emitted_tag_name, const QString& emitted_value) {
            emitted = true;
            TagName = emitted_tag_name;
            value = emitted_value;
        });

    parser.ParseTag("<number>숫자</number>");

    expect(emitted, "TagParser should emit TagParsed");
    expect(TagName == "number", "TagParser should emit tag name");
    expect(value == "숫자", "TagParser should emit tag value");
}

void tag_parser_emits_failure_signal() {
    iiXml::Parser::TagParser parser;
    bool failed = false;

    QObject::connect(&parser, &iiXml::Parser::TagParser::ParseFailed,
        [&](const QString& reason) {
            failed = !reason.isEmpty();
        });

    parser.ParseTag("<number>1</text>");

    expect(failed, "TagParser should emit ParseFailed");
}

void file_parser_emits_parsed_signal() {
    QTemporaryDir directory;
    expect(directory.isValid(), "temporary directory should be valid");
    if (!directory.isValid()) {
        return;
    }

    const QString path = directory.filePath("input.xml");
    writes_file(path, "<number>42</number>");

    iiXml::Parser::FileParser parser;
    bool emitted = false;
    QString TagName;
    QString value;

    QObject::connect(&parser, &iiXml::Parser::FileParser::TagParsed,
        [&](const QString& emitted_tag_name, const QString& emitted_value) {
            emitted = true;
            TagName = emitted_tag_name;
            value = emitted_value;
        });

    parser.ParseFileInput(path);

    expect(emitted, "FileParser should emit TagParsed");
    expect(TagName == "number", "FileParser should emit tag name");
    expect(value == "42", "FileParser should emit tag value");
}

void doctype_emits_match_signal() {
    iiXml::Elements::Doctype doctype;
    bool matched = false;
    iiXml::Elements::Doctype::Kind kind = iiXml::Elements::Doctype::Kind::XmlDeclaration;
    QString raw;

    QObject::connect(&doctype, &iiXml::Elements::Doctype::DoctypeMatched,
        [&](iiXml::Elements::Doctype::Kind emitted_kind, const QString& emitted_raw) {
            matched = true;
            kind = emitted_kind;
            raw = emitted_raw;
        });

    doctype.MatchTopInput("<!Doctype XML>\n<XML></XML>");

    expect(matched, "Doctype should emit DoctypeMatched");
    expect(kind == iiXml::Elements::Doctype::Kind::DoctypeDeclaration,
        "Doctype should emit doctype declaration kind");
    expect(raw == "<!Doctype XML>", "Doctype should emit raw declaration");
}

void doctype_emits_rejection_signal() {
    iiXml::Elements::Doctype doctype;
    bool rejected = false;

    QObject::connect(&doctype, &iiXml::Elements::Doctype::DoctypeRejected,
        [&](const QString& reason) {
            rejected = !reason.isEmpty();
        });

    doctype.MatchTopInput("<XML></XML>");

    expect(rejected, "Doctype should emit DoctypeRejected");
}

void input_validator_emits_invalid_xml_file() {
    iiXml::Writer::InputValidator validator;
    bool invalid_xml_file = false;
    bool failed = false;
    iiXml::Writer::InputValidator::ValidationExit result =
        iiXml::Writer::InputValidator::ValidationExit::Valid;

    QObject::connect(&validator, &iiXml::Writer::InputValidator::ValidationFinished,
        [&](iiXml::Writer::InputValidator::ValidationExit emitted_result) {
            result = emitted_result;
        });
    QObject::connect(&validator, &iiXml::Writer::InputValidator::InvalidXmlFile,
        [&]() {
            invalid_xml_file = true;
        });
    QObject::connect(&validator, &iiXml::Writer::InputValidator::ValidationFailed,
        [&](iiXml::Writer::InputValidator::ValidationExit, const QString& reason) {
            failed = !reason.isEmpty();
        });

    validator.ValidateInput("<XML></XML>");

    expect(result == iiXml::Writer::InputValidator::ValidationExit::InvalidXmlFile,
        "InputValidator should emit InvalidXmlFile result");
    expect(invalid_xml_file, "InputValidator should emit InvalidXmlFile");
    expect(failed, "InputValidator should emit ValidationFailed with reason");
}

void input_validator_emits_invalid_tag_closure() {
    iiXml::Writer::InputValidator validator;
    bool invalid_tag_closure = false;
    bool failed = false;
    iiXml::Writer::InputValidator::ValidationExit result =
        iiXml::Writer::InputValidator::ValidationExit::Valid;

    QObject::connect(&validator, &iiXml::Writer::InputValidator::ValidationFinished,
        [&](iiXml::Writer::InputValidator::ValidationExit emitted_result) {
            result = emitted_result;
        });
    QObject::connect(&validator, &iiXml::Writer::InputValidator::InvalidTagClosure,
        [&]() {
            invalid_tag_closure = true;
        });
    QObject::connect(&validator, &iiXml::Writer::InputValidator::ValidationFailed,
        [&](iiXml::Writer::InputValidator::ValidationExit, const QString& reason) {
            failed = !reason.isEmpty();
        });

    validator.ValidateInput("<!Doctype XML>\n<XML><number>1</XML>");

    expect(result == iiXml::Writer::InputValidator::ValidationExit::InvalidTagClosure,
        "InputValidator should emit InvalidTagClosure result");
    expect(invalid_tag_closure, "InputValidator should emit InvalidTagClosure");
    expect(failed, "InputValidator should emit ValidationFailed with reason");
}

void input_validator_emits_valid_xml() {
    iiXml::Writer::InputValidator validator;
    bool valid_xml = false;
    iiXml::Writer::InputValidator::ValidationExit result =
        iiXml::Writer::InputValidator::ValidationExit::InvalidXmlFile;

    QObject::connect(&validator, &iiXml::Writer::InputValidator::ValidationFinished,
        [&](iiXml::Writer::InputValidator::ValidationExit emitted_result) {
            result = emitted_result;
        });
    QObject::connect(&validator, &iiXml::Writer::InputValidator::ValidXml,
        [&]() {
            valid_xml = true;
        });

    validator.ValidateInput("<!Doctype XML>\n<XML><number>1</number></XML>");

    expect(result == iiXml::Writer::InputValidator::ValidationExit::Valid,
        "InputValidator should emit Valid result");
    expect(valid_xml, "InputValidator should emit ValidXml");
}

void get_file_emits_parsed_signal() {
    iiXml::Writer::GetFile input;
    bool emitted = false;
    QString TagName;
    QString value;

    QObject::connect(&input, &iiXml::Writer::GetFile::Parsed,
        [&](const QString& emitted_tag_name, const QString& emitted_value) {
            emitted = true;
            TagName = emitted_tag_name;
            value = emitted_value;
        });

    input.ReadXml("<!Doctype XML>\n<XML><number>1</number></XML>");

    expect(emitted, "GetFile should emit Parsed");
    expect(TagName == "XML", "GetFile should emit root tag name");
    expect(value == "<number>1</number>", "GetFile should emit root value");
}

void get_file_emits_invalid_tag_closure() {
    iiXml::Writer::GetFile input;
    bool invalid_tag_closure = false;
    bool failed = false;
    iiXml::Writer::GetFile::Status status = iiXml::Writer::GetFile::Status::Parsed;

    QObject::connect(&input, &iiXml::Writer::GetFile::InvalidTagClosure,
        [&]() {
            invalid_tag_closure = true;
        });
    QObject::connect(&input, &iiXml::Writer::GetFile::Failed,
        [&](iiXml::Writer::GetFile::Status emitted_status, const QString& reason) {
            failed = !reason.isEmpty();
            status = emitted_status;
        });

    input.ReadXml("<!Doctype XML>\n<XML><number>1</XML>");

    expect(invalid_tag_closure, "GetFile should emit InvalidTagClosure");
    expect(failed, "GetFile should emit Failed with reason");
    expect(status == iiXml::Writer::GetFile::Status::InvalidTagClosure,
        "GetFile failed signal should emit InvalidTagClosure status");
}

} // namespace

int main() {
    tag_parser_emits_parsed_signal();
    tag_parser_emits_failure_signal();
    file_parser_emits_parsed_signal();
    doctype_emits_match_signal();
    doctype_emits_rejection_signal();
    input_validator_emits_invalid_xml_file();
    input_validator_emits_invalid_tag_closure();
    input_validator_emits_valid_xml();
    get_file_emits_parsed_signal();
    get_file_emits_invalid_tag_closure();

    return failures == 0 ? 0 : 1;
}
