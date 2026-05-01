#include <iiXml>

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
    const iiXml::Writer::GetStringToken input;

    const iiXml::Writer::GetStringTokenResult result =
        input.ParseString("<!Doctype XML>\n<XML><number>42</number></XML>");

    expect(result.Status == iiXml::Writer::GetStringTokenStatus::Parsed,
        "validated QString XML should return parsed status");
    expect(!result.Reason.empty(), "validated QString XML should return non-empty reason");
    expect(result.Token.has_value(), "validated QString XML should parse");
    if (!result.Token.has_value()) {
        return;
    }

    expect(result.Token->TagName == "XML", "validated QString XML should preserve root tag name");
    expect(result.Token->Value == "<number>42</number>", "validated QString XML should preserve root value");
}

void preserves_utf8_qstring_value() {
    const iiXml::Writer::GetStringToken input;

    const iiXml::Writer::GetStringTokenResult result =
        input.ParseString("<!Doctype XML>\n<XML><number>숫자</number></XML>");

    expect(result.Status == iiXml::Writer::GetStringTokenStatus::Parsed,
        "UTF-8 validated QString XML should return parsed status");
    expect(!result.Reason.empty(), "UTF-8 validated QString XML should return non-empty reason");
    expect(result.Token.has_value(), "UTF-8 validated QString XML should parse");
    if (!result.Token.has_value()) {
        return;
    }

    expect(result.Token->Value == "<number>숫자</number>", "UTF-8 QString value should be preserved");
}

void rejects_string_without_doctype() {
    const iiXml::Writer::GetStringToken input;

    const iiXml::Writer::GetStringTokenResult result =
        input.ParseString("<number>42</number>");

    expect(result.Status == iiXml::Writer::GetStringTokenStatus::InvalidXmlFile,
        "QString input without Doctype should return invalid_xml_file");
    expect(!result.Reason.empty(), "Doctype failure should return non-empty reason");
    expect(!result.Token.has_value(),
        "QString input without Doctype should fail before tag parser");
}

void rejects_invalid_tag_closure_after_doctype() {
    const iiXml::Writer::GetStringToken input;

    const iiXml::Writer::GetStringTokenResult result =
        input.ParseString("<!Doctype XML>\n<XML><number>42</XML>");

    expect(result.Status == iiXml::Writer::GetStringTokenStatus::InvalidTagClosure,
        "QString input with invalid tag closure should return invalid_tag_closure");
    expect(!result.Reason.empty(), "tag closure failure should return non-empty reason");
    expect(!result.Token.has_value(),
        "QString input with invalid tag closure should fail before tag parser");
}

void emits_parsed_signal() {
    iiXml::Writer::GetStringToken input;
    bool emitted = false;
    QString TagName;
    QString value;

    QObject::connect(&input, &iiXml::Writer::GetStringToken::Parsed,
        [&](const QString& emitted_tag_name, const QString& emitted_value) {
            emitted = true;
            TagName = emitted_tag_name;
            value = emitted_value;
        });

    input.ReadString("<!Doctype XML>\n<XML><number>숫자</number></XML>");

    expect(emitted, "GetStringToken should emit Parsed");
    expect(TagName == "XML", "GetStringToken should emit root tag name");
    expect(value == "<number>숫자</number>", "GetStringToken should emit root value");
}

void emits_failure_signal() {
    iiXml::Writer::GetStringToken input;
    bool failed = false;
    bool status_failed = false;
    iiXml::Writer::GetStringToken::Status status =
        iiXml::Writer::GetStringToken::Status::Parsed;

    QObject::connect(&input, &iiXml::Writer::GetStringToken::ParseFailed,
        [&](const QString& reason) {
            failed = !reason.isEmpty();
        });
    QObject::connect(&input, &iiXml::Writer::GetStringToken::Failed,
        [&](iiXml::Writer::GetStringToken::Status emitted_status, const QString& reason) {
            status_failed = !reason.isEmpty();
            status = emitted_status;
        });

    input.ReadString("<number>42</number>");

    expect(failed, "GetStringToken should emit ParseFailed");
    expect(status_failed, "GetStringToken should emit Failed with reason");
    expect(status == iiXml::Writer::GetStringToken::Status::InvalidXmlFile,
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
