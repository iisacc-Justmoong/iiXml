#include <iiXml>

#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <iostream>

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

bool saw_diagnostic(
    const QString& scope,
    const QString& reason,
    const QString& context
) {
    for (const QString& message : messages) {
        if (message.contains(scope)
            && message.contains("failed")
            && message.contains(reason)
            && message.contains("offset=")
            && message.contains("line=")
            && message.contains("column=")
            && message.contains("context=")
            && message.contains(context)) {
            return true;
        }
    }

    return false;
}

bool saw_log_kind(const QString& scope, const QString& kind) {
    const QString token = scope + " " + kind + " ";
    for (const QString& message : messages) {
        if (message.contains(token)) {
            return true;
        }
    }

    return false;
}

void exercises_general_input_parse_output_logging() {
    iiXml::Parser::TagParser parser;
    const auto parsed_tag = parser.Parse("<number>42</number>");
    const auto parsed_nodes = parser.ParseAll("<root><child>v</child></root>");

    iiXml::Elements::OpenTag open_tag;
    const auto parsed_ranges = open_tag.ParseOpenTags("<root><child>v</child></root>");

    iiXml::Elements::InlineProperties properties;
    const auto parsed_properties =
        properties.Parse("<resource title=\"text\" count=1 enabled=true>");

    iiXml::Writer::InputValidator validator;
    const auto validation =
        validator.Validate("<!Doctype XML>\n<XML><number>1</number></XML>");

    iiXml::Writer::GetStringToken string_token;
    const auto parsed_string =
        string_token.ParseString("<!Doctype XML>\n<XML><number>1</number></XML>");

    expect(parsed_tag.has_value(), "tag parser should parse valid tag for logging test");
    expect(parsed_nodes.has_value(), "tag parser should parse all valid tags for logging test");
    expect(parsed_ranges.has_value(), "open tag parser should parse valid ranges for logging test");
    expect(parsed_properties.has_value(), "inline properties should parse valid fields for logging test");
    expect(validation == iiXml::Writer::ValidationExit::Valid,
        "input validator should accept valid XML for logging test");
    expect(parsed_string.Status == iiXml::Writer::GetStringTokenStatus::Parsed,
        "string token should parse valid XML for logging test");
}

void exercises_enhanced_failure_diagnostics() {
    iiXml::Parser::TagParser parser;
    const auto rejected_tag = parser.Parse("<number>42</text>");

    iiXml::Elements::InlineProperties properties;
    const auto rejected_properties = properties.Parse("<resource title=\"hello>");

    iiXml::Elements::OpenTag open_tag;
    const auto rejected_open_tags = open_tag.ParseOpenTags("<a>\n<b></a>");

    iiXml::Writer::InputValidator validator;
    const bool rejected_closure = validator.HasValidTagClosure("<XML>\n<number>1</missing>");

    iiXml::Elements::Doctype doctype;
    const auto rejected_doctype = doctype.MatchTopResult("<XML></XML>");

    expect(!rejected_tag.has_value(), "tag parser should reject mismatched closing tag");
    expect(!rejected_properties.has_value(), "inline properties should reject unclosed value");
    expect(!rejected_open_tags.has_value(), "open tag parser should reject unclosed open tag");
    expect(!rejected_closure, "input validator should reject mismatched closure");
    expect(!rejected_doctype.Match.has_value(), "doctype parser should reject missing declaration");
}

} // namespace

int main() {
    const QtMessageHandler previous_handler = qInstallMessageHandler(capture_debug);
    exercises_general_input_parse_output_logging();
    exercises_enhanced_failure_diagnostics();
    qInstallMessageHandler(previous_handler);

    expect(saw_log_kind("iiXml::Parser::TagParser::Parse", "input"),
        "TagParser::Parse should log input summary");
    expect(saw_log_kind("iiXml::Parser::TagParser::Parse", "parsing"),
        "TagParser::Parse should log parsing events");
    expect(saw_log_kind("iiXml::Parser::TagParser::Parse", "output"),
        "TagParser::Parse should log output summary");

    expect(saw_log_kind("iiXml::Elements::OpenTag::ParseOpenTags", "input"),
        "OpenTag::ParseOpenTags should log input summary");
    expect(saw_log_kind("iiXml::Elements::OpenTag::ParseOpenTags", "parsing"),
        "OpenTag::ParseOpenTags should log parsing events");
    expect(saw_log_kind("iiXml::Elements::OpenTag::ParseOpenTags", "output"),
        "OpenTag::ParseOpenTags should log output summary");

    expect(saw_log_kind("iiXml::Elements::InlineProperties::Parse", "input"),
        "InlineProperties::Parse should log input summary");
    expect(saw_log_kind("iiXml::Elements::InlineProperties::Parse", "parsing"),
        "InlineProperties::Parse should log parsing events");
    expect(saw_log_kind("iiXml::Elements::InlineProperties::Parse", "output"),
        "InlineProperties::Parse should log output summary");

    expect(saw_log_kind("iiXml::Writer::GetStringToken::ParseString", "input"),
        "GetStringToken::ParseString should log input summary");
    expect(saw_log_kind("iiXml::Writer::GetStringToken::ParseString", "output"),
        "GetStringToken::ParseString should log output summary");

    expect(saw_diagnostic(
        "iiXml::Parser::TagParser::Parse",
        "closing tag mismatch",
        "</text>"
    ), "TagParser::Parse failure should include reason, offset, line, column, and context");

    expect(saw_diagnostic(
        "iiXml::Elements::InlineProperties::Parse",
        "unclosed quoted property value",
        "title=\"hello"
    ), "InlineProperties::Parse failure should include reason, offset, line, column, and context");

    expect(saw_diagnostic(
        "iiXml::Elements::OpenTag::ParseOpenTags",
        "unclosed open tags",
        "<b>"
    ), "OpenTag::ParseOpenTags failure should include reason, offset, line, column, and context");

    expect(saw_diagnostic(
        "iiXml::Writer::InputValidator::HasValidTagClosure",
        "closing tag mismatch",
        "</missing>"
    ), "InputValidator::HasValidTagClosure failure should include reason, offset, line, column, and context");

    expect(saw_diagnostic(
        "iiXml::Elements::Doctype::MatchTopResult",
        "top declaration is missing",
        "<XML>"
    ), "Doctype::MatchTopResult failure should include reason, offset, line, column, and context");

    return failures == 0 ? 0 : 1;
}
