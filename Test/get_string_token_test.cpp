#include "iiXml.h"

#include <QObject>
#include <QString>

#include <iostream>
namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

void parses_qstring_token() {
    const iiXml::writer::GetStringToken input;

    const iiXml::writer::get_string_token_result result =
        input.parse_string("<!DOCTYPE XML>\n<XML><number>42</number></XML>");

    expect(result.status == iiXml::writer::get_string_token_status::parsed,
        "validated QString XML should return parsed status");
    expect(!result.reason.empty(), "validated QString XML should return non-empty reason");
    expect(result.token.has_value(), "validated QString XML should parse");
    if (!result.token.has_value()) {
        return;
    }

    expect(result.token->tag_name == "XML", "validated QString XML should preserve root tag name");
    expect(result.token->value == "<number>42</number>", "validated QString XML should preserve root value");
}

void preserves_utf8_qstring_value() {
    const iiXml::writer::GetStringToken input;

    const iiXml::writer::get_string_token_result result =
        input.parse_string("<!DOCTYPE XML>\n<XML><number>숫자</number></XML>");

    expect(result.status == iiXml::writer::get_string_token_status::parsed,
        "UTF-8 validated QString XML should return parsed status");
    expect(!result.reason.empty(), "UTF-8 validated QString XML should return non-empty reason");
    expect(result.token.has_value(), "UTF-8 validated QString XML should parse");
    if (!result.token.has_value()) {
        return;
    }

    expect(result.token->value == "<number>숫자</number>", "UTF-8 QString value should be preserved");
}

void rejects_string_without_doctype() {
    const iiXml::writer::GetStringToken input;

    const iiXml::writer::get_string_token_result result =
        input.parse_string("<number>42</number>");

    expect(result.status == iiXml::writer::get_string_token_status::invalid_xml_file,
        "QString input without DOCTYPE should return invalid_xml_file");
    expect(!result.reason.empty(), "DOCTYPE failure should return non-empty reason");
    expect(!result.token.has_value(),
        "QString input without DOCTYPE should fail before tag parser");
}

void rejects_invalid_tag_closure_after_doctype() {
    const iiXml::writer::GetStringToken input;

    const iiXml::writer::get_string_token_result result =
        input.parse_string("<!DOCTYPE XML>\n<XML><number>42</XML>");

    expect(result.status == iiXml::writer::get_string_token_status::invalid_tag_closure,
        "QString input with invalid tag closure should return invalid_tag_closure");
    expect(!result.reason.empty(), "tag closure failure should return non-empty reason");
    expect(!result.token.has_value(),
        "QString input with invalid tag closure should fail before tag parser");
}

void emits_parsed_signal() {
    iiXml::writer::GetStringToken input;
    bool emitted = false;
    QString tag_name;
    QString value;

    QObject::connect(&input, &iiXml::writer::GetStringToken::parsed,
        [&](const QString& emitted_tag_name, const QString& emitted_value) {
            emitted = true;
            tag_name = emitted_tag_name;
            value = emitted_value;
        });

    input.readString("<!DOCTYPE XML>\n<XML><number>숫자</number></XML>");

    expect(emitted, "GetStringToken should emit parsed");
    expect(tag_name == "XML", "GetStringToken should emit root tag name");
    expect(value == "<number>숫자</number>", "GetStringToken should emit root value");
}

void emits_failure_signal() {
    iiXml::writer::GetStringToken input;
    bool failed = false;
    bool status_failed = false;
    iiXml::writer::GetStringToken::Status status =
        iiXml::writer::GetStringToken::Status::Parsed;

    QObject::connect(&input, &iiXml::writer::GetStringToken::parseFailed,
        [&](const QString& reason) {
            failed = !reason.isEmpty();
        });
    QObject::connect(&input, &iiXml::writer::GetStringToken::failed,
        [&](iiXml::writer::GetStringToken::Status emitted_status, const QString& reason) {
            status_failed = !reason.isEmpty();
            status = emitted_status;
        });

    input.readString("<number>42</number>");

    expect(failed, "GetStringToken should emit parseFailed");
    expect(status_failed, "GetStringToken should emit failed with reason");
    expect(status == iiXml::writer::GetStringToken::Status::InvalidXmlFile,
        "GetStringToken failed signal should emit InvalidXmlFile status");
}

} // namespace

int main() {
    parses_qstring_token();
    preserves_utf8_qstring_value();
    rejects_string_without_doctype();
    rejects_invalid_tag_closure_after_doctype();
    emits_parsed_signal();
    emits_failure_signal();

    return failures == 0 ? 0 : 1;
}
