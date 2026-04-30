#include "iiXml.h"

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
    iiXml::parser::tag_parser parser;
    bool emitted = false;
    QString tag_name;
    QString value;

    QObject::connect(&parser, &iiXml::parser::tag_parser::tagParsed,
        [&](const QString& emitted_tag_name, const QString& emitted_value) {
            emitted = true;
            tag_name = emitted_tag_name;
            value = emitted_value;
        });

    parser.parseTag("<number>숫자</number>");

    expect(emitted, "tag_parser should emit tagParsed");
    expect(tag_name == "number", "tag_parser should emit tag name");
    expect(value == "숫자", "tag_parser should emit tag value");
}

void tag_parser_emits_failure_signal() {
    iiXml::parser::tag_parser parser;
    bool failed = false;

    QObject::connect(&parser, &iiXml::parser::tag_parser::parseFailed,
        [&](const QString& reason) {
            failed = !reason.isEmpty();
        });

    parser.parseTag("<number>1</text>");

    expect(failed, "tag_parser should emit parseFailed");
}

void file_parser_emits_parsed_signal() {
    QTemporaryDir directory;
    expect(directory.isValid(), "temporary directory should be valid");
    if (!directory.isValid()) {
        return;
    }

    const QString path = directory.filePath("input.xml");
    writes_file(path, "<number>42</number>");

    iiXml::parser::FileParser parser;
    bool emitted = false;
    QString tag_name;
    QString value;

    QObject::connect(&parser, &iiXml::parser::FileParser::tagParsed,
        [&](const QString& emitted_tag_name, const QString& emitted_value) {
            emitted = true;
            tag_name = emitted_tag_name;
            value = emitted_value;
        });

    parser.parseFile(path);

    expect(emitted, "FileParser should emit tagParsed");
    expect(tag_name == "number", "FileParser should emit tag name");
    expect(value == "42", "FileParser should emit tag value");
}

void doctype_emits_match_signal() {
    iiXml::elements::DOCTYPE doctype;
    bool matched = false;
    iiXml::elements::DOCTYPE::Kind kind = iiXml::elements::DOCTYPE::Kind::XmlDeclaration;
    QString raw;

    QObject::connect(&doctype, &iiXml::elements::DOCTYPE::doctypeMatched,
        [&](iiXml::elements::DOCTYPE::Kind emitted_kind, const QString& emitted_raw) {
            matched = true;
            kind = emitted_kind;
            raw = emitted_raw;
        });

    doctype.matchTop("<!DOCTYPE XML>\n<XML></XML>");

    expect(matched, "DOCTYPE should emit doctypeMatched");
    expect(kind == iiXml::elements::DOCTYPE::Kind::DoctypeDeclaration,
        "DOCTYPE should emit doctype declaration kind");
    expect(raw == "<!DOCTYPE XML>", "DOCTYPE should emit raw declaration");
}

void doctype_emits_rejection_signal() {
    iiXml::elements::DOCTYPE doctype;
    bool rejected = false;

    QObject::connect(&doctype, &iiXml::elements::DOCTYPE::doctypeRejected,
        [&](const QString& reason) {
            rejected = !reason.isEmpty();
        });

    doctype.matchTop("<XML></XML>");

    expect(rejected, "DOCTYPE should emit doctypeRejected");
}

void input_validator_emits_invalid_xml_file() {
    iiXml::writer::InputValidator validator;
    bool invalid_xml_file = false;
    bool failed = false;
    iiXml::writer::InputValidator::ValidationExit result =
        iiXml::writer::InputValidator::ValidationExit::Valid;

    QObject::connect(&validator, &iiXml::writer::InputValidator::validationFinished,
        [&](iiXml::writer::InputValidator::ValidationExit emitted_result) {
            result = emitted_result;
        });
    QObject::connect(&validator, &iiXml::writer::InputValidator::invalidXmlFile,
        [&]() {
            invalid_xml_file = true;
        });
    QObject::connect(&validator, &iiXml::writer::InputValidator::validationFailed,
        [&](iiXml::writer::InputValidator::ValidationExit, const QString& reason) {
            failed = !reason.isEmpty();
        });

    validator.validateInput("<XML></XML>");

    expect(result == iiXml::writer::InputValidator::ValidationExit::InvalidXmlFile,
        "InputValidator should emit InvalidXmlFile result");
    expect(invalid_xml_file, "InputValidator should emit invalidXmlFile");
    expect(failed, "InputValidator should emit validationFailed with reason");
}

void input_validator_emits_invalid_tag_closure() {
    iiXml::writer::InputValidator validator;
    bool invalid_tag_closure = false;
    bool failed = false;
    iiXml::writer::InputValidator::ValidationExit result =
        iiXml::writer::InputValidator::ValidationExit::Valid;

    QObject::connect(&validator, &iiXml::writer::InputValidator::validationFinished,
        [&](iiXml::writer::InputValidator::ValidationExit emitted_result) {
            result = emitted_result;
        });
    QObject::connect(&validator, &iiXml::writer::InputValidator::invalidTagClosure,
        [&]() {
            invalid_tag_closure = true;
        });
    QObject::connect(&validator, &iiXml::writer::InputValidator::validationFailed,
        [&](iiXml::writer::InputValidator::ValidationExit, const QString& reason) {
            failed = !reason.isEmpty();
        });

    validator.validateInput("<!DOCTYPE XML>\n<XML><number>1</XML>");

    expect(result == iiXml::writer::InputValidator::ValidationExit::InvalidTagClosure,
        "InputValidator should emit InvalidTagClosure result");
    expect(invalid_tag_closure, "InputValidator should emit invalidTagClosure");
    expect(failed, "InputValidator should emit validationFailed with reason");
}

void input_validator_emits_valid_xml() {
    iiXml::writer::InputValidator validator;
    bool valid_xml = false;
    iiXml::writer::InputValidator::ValidationExit result =
        iiXml::writer::InputValidator::ValidationExit::InvalidXmlFile;

    QObject::connect(&validator, &iiXml::writer::InputValidator::validationFinished,
        [&](iiXml::writer::InputValidator::ValidationExit emitted_result) {
            result = emitted_result;
        });
    QObject::connect(&validator, &iiXml::writer::InputValidator::validXml,
        [&]() {
            valid_xml = true;
        });

    validator.validateInput("<!DOCTYPE XML>\n<XML><number>1</number></XML>");

    expect(result == iiXml::writer::InputValidator::ValidationExit::Valid,
        "InputValidator should emit Valid result");
    expect(valid_xml, "InputValidator should emit validXml");
}

void get_file_emits_parsed_signal() {
    iiXml::writer::GetFile input;
    bool emitted = false;
    QString tag_name;
    QString value;

    QObject::connect(&input, &iiXml::writer::GetFile::parsed,
        [&](const QString& emitted_tag_name, const QString& emitted_value) {
            emitted = true;
            tag_name = emitted_tag_name;
            value = emitted_value;
        });

    input.readXml("<!DOCTYPE XML>\n<XML><number>1</number></XML>");

    expect(emitted, "GetFile should emit parsed");
    expect(tag_name == "XML", "GetFile should emit root tag name");
    expect(value == "<number>1</number>", "GetFile should emit root value");
}

void get_file_emits_invalid_tag_closure() {
    iiXml::writer::GetFile input;
    bool invalid_tag_closure = false;
    bool failed = false;
    iiXml::writer::GetFile::Status status = iiXml::writer::GetFile::Status::Parsed;

    QObject::connect(&input, &iiXml::writer::GetFile::invalidTagClosure,
        [&]() {
            invalid_tag_closure = true;
        });
    QObject::connect(&input, &iiXml::writer::GetFile::failed,
        [&](iiXml::writer::GetFile::Status emitted_status, const QString& reason) {
            failed = !reason.isEmpty();
            status = emitted_status;
        });

    input.readXml("<!DOCTYPE XML>\n<XML><number>1</XML>");

    expect(invalid_tag_closure, "GetFile should emit invalidTagClosure");
    expect(failed, "GetFile should emit failed with reason");
    expect(status == iiXml::writer::GetFile::Status::InvalidTagClosure,
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
