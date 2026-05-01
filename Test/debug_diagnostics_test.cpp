#include "iiXml.h"

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
    iiXml::parser::tag_parser parser;
    const auto parsed_tag = parser.parse("<number>42</number>");
    const auto parsed_nodes = parser.parse_all("<root><child>v</child></root>");

    iiXml::elements::OpenTag open_tag;
    const auto parsed_ranges = open_tag.parse_open_tags("<root><child>v</child></root>");

    iiXml::elements::InlineProperties properties;
    const auto parsed_properties =
        properties.parse("<resource title=\"text\" count=1 enabled=true>");

    iiXml::writer::InputValidator validator;
    const auto validation =
        validator.validate("<!DOCTYPE XML>\n<XML><number>1</number></XML>");

    iiXml::writer::GetStringToken string_token;
    const auto parsed_string =
        string_token.parse_string("<!DOCTYPE XML>\n<XML><number>1</number></XML>");

    expect(parsed_tag.has_value(), "tag parser should parse valid tag for logging test");
    expect(parsed_nodes.has_value(), "tag parser should parse all valid tags for logging test");
    expect(parsed_ranges.has_value(), "open tag parser should parse valid ranges for logging test");
    expect(parsed_properties.has_value(), "inline properties should parse valid fields for logging test");
    expect(validation == iiXml::writer::validation_exit::valid,
        "input validator should accept valid XML for logging test");
    expect(parsed_string.status == iiXml::writer::get_string_token_status::parsed,
        "string token should parse valid XML for logging test");
}

void exercises_enhanced_failure_diagnostics() {
    iiXml::parser::tag_parser parser;
    const auto rejected_tag = parser.parse("<number>42</text>");

    iiXml::elements::InlineProperties properties;
    const auto rejected_properties = properties.parse("<resource title=\"hello>");

    iiXml::elements::OpenTag open_tag;
    const auto rejected_open_tags = open_tag.parse_open_tags("<a>\n<b></a>");

    iiXml::writer::InputValidator validator;
    const bool rejected_closure = validator.has_valid_tag_closure("<XML>\n<number>1</missing>");

    iiXml::elements::DOCTYPE doctype;
    const auto rejected_doctype = doctype.match_top_result("<XML></XML>");

    expect(!rejected_tag.has_value(), "tag parser should reject mismatched closing tag");
    expect(!rejected_properties.has_value(), "inline properties should reject unclosed value");
    expect(!rejected_open_tags.has_value(), "open tag parser should reject unclosed open tag");
    expect(!rejected_closure, "input validator should reject mismatched closure");
    expect(!rejected_doctype.match.has_value(), "doctype parser should reject missing declaration");
}

} // namespace

int main() {
    const QtMessageHandler previous_handler = qInstallMessageHandler(capture_debug);
    exercises_general_input_parse_output_logging();
    exercises_enhanced_failure_diagnostics();
    qInstallMessageHandler(previous_handler);

    expect(saw_log_kind("iiXml::parser::tag_parser::parse", "input"),
        "tag_parser::parse should log input summary");
    expect(saw_log_kind("iiXml::parser::tag_parser::parse", "parsing"),
        "tag_parser::parse should log parsing events");
    expect(saw_log_kind("iiXml::parser::tag_parser::parse", "output"),
        "tag_parser::parse should log output summary");

    expect(saw_log_kind("iiXml::elements::OpenTag::parse_open_tags", "input"),
        "OpenTag::parse_open_tags should log input summary");
    expect(saw_log_kind("iiXml::elements::OpenTag::parse_open_tags", "parsing"),
        "OpenTag::parse_open_tags should log parsing events");
    expect(saw_log_kind("iiXml::elements::OpenTag::parse_open_tags", "output"),
        "OpenTag::parse_open_tags should log output summary");

    expect(saw_log_kind("iiXml::elements::InlineProperties::parse", "input"),
        "InlineProperties::parse should log input summary");
    expect(saw_log_kind("iiXml::elements::InlineProperties::parse", "parsing"),
        "InlineProperties::parse should log parsing events");
    expect(saw_log_kind("iiXml::elements::InlineProperties::parse", "output"),
        "InlineProperties::parse should log output summary");

    expect(saw_log_kind("iiXml::writer::GetStringToken::parse_string", "input"),
        "GetStringToken::parse_string should log input summary");
    expect(saw_log_kind("iiXml::writer::GetStringToken::parse_string", "output"),
        "GetStringToken::parse_string should log output summary");

    expect(saw_diagnostic(
        "iiXml::parser::tag_parser::parse",
        "closing tag mismatch",
        "</text>"
    ), "tag_parser::parse failure should include reason, offset, line, column, and context");

    expect(saw_diagnostic(
        "iiXml::elements::InlineProperties::parse",
        "unclosed quoted property value",
        "title=\"hello"
    ), "InlineProperties::parse failure should include reason, offset, line, column, and context");

    expect(saw_diagnostic(
        "iiXml::elements::OpenTag::parse_open_tags",
        "unclosed open tags",
        "<b>"
    ), "OpenTag::parse_open_tags failure should include reason, offset, line, column, and context");

    expect(saw_diagnostic(
        "iiXml::writer::InputValidator::has_valid_tag_closure",
        "closing tag mismatch",
        "</missing>"
    ), "InputValidator::has_valid_tag_closure failure should include reason, offset, line, column, and context");

    expect(saw_diagnostic(
        "iiXml::elements::DOCTYPE::match_top_result",
        "top declaration is missing",
        "<XML>"
    ), "DOCTYPE::match_top_result failure should include reason, offset, line, column, and context");

    return failures == 0 ? 0 : 1;
}
