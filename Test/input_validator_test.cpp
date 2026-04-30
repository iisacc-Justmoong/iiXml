#include "iiXml.h"

#include <iostream>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

void returns_valid_for_valid_document() {
    const iiXml::writer::InputValidator validator;

    const iiXml::writer::validation_exit result =
        validator.validate("<!DOCTYPE XML>\n<XML><number>1</number></XML>");
    const iiXml::writer::validation_result detailed =
        validator.validate_result("<!DOCTYPE XML>\n<XML><number>1</number></XML>");

    expect(result == iiXml::writer::validation_exit::valid,
        "valid document should return valid");
    expect(detailed.exit == iiXml::writer::validation_exit::valid,
        "valid document should return detailed valid exit");
    expect(!detailed.reason.empty(), "valid document should return non-empty reason");
}

void returns_invalid_xml_file_when_doctype_fails() {
    const iiXml::writer::InputValidator validator;

    const iiXml::writer::validation_exit result =
        validator.validate("<XML><number>1</number></XML>");
    const iiXml::writer::validation_result detailed =
        validator.validate_result("<XML><number>1</number></XML>");

    expect(result == iiXml::writer::validation_exit::invalid_xml_file,
        "document without top DOCTYPE/XML declaration should return invalid_xml_file");
    expect(detailed.exit == iiXml::writer::validation_exit::invalid_xml_file,
        "document without top DOCTYPE/XML declaration should return detailed invalid_xml_file");
    expect(!detailed.reason.empty(), "invalid XML file should return non-empty reason");
}

void returns_invalid_tag_closure_when_tag_closure_fails() {
    const iiXml::writer::InputValidator validator;

    const iiXml::writer::validation_exit result =
        validator.validate("<!DOCTYPE XML>\n<XML><number>1</XML>");
    const iiXml::writer::validation_result detailed =
        validator.validate_result("<!DOCTYPE XML>\n<XML><number>1</XML>");

    expect(result == iiXml::writer::validation_exit::invalid_tag_closure,
        "mismatched close tag should return invalid_tag_closure");
    expect(detailed.exit == iiXml::writer::validation_exit::invalid_tag_closure,
        "mismatched close tag should return detailed invalid_tag_closure");
    expect(!detailed.reason.empty(), "invalid tag closure should return non-empty reason");
}

void returns_invalid_tag_closure_for_unclosed_tag() {
    const iiXml::writer::InputValidator validator;

    const iiXml::writer::validation_exit result =
        validator.validate("<!DOCTYPE XML>\n<XML><number>1</number>");
    const iiXml::writer::validation_result detailed =
        validator.validate_result("<!DOCTYPE XML>\n<XML><number>1</number>");

    expect(result == iiXml::writer::validation_exit::invalid_tag_closure,
        "unclosed tag should return invalid_tag_closure");
    expect(detailed.exit == iiXml::writer::validation_exit::invalid_tag_closure,
        "unclosed tag should return detailed invalid_tag_closure");
    expect(!detailed.reason.empty(), "unclosed tag should return non-empty reason");
}

void validates_self_closing_tags() {
    const iiXml::writer::InputValidator validator;

    expect(validator.has_valid_tag_closure("<XML><empty /></XML>"),
        "self-closing tags should not require a close tag");
}

void validates_cross_nested_tags() {
    const iiXml::writer::InputValidator validator;

    expect(validator.has_valid_tag_closure("<a><b></a></b>"),
        "cross nested tags should be accepted by the open tag policy");

    const iiXml::writer::validation_result detailed =
        validator.validate_result("<!DOCTYPE XML>\n<XML><a><b></a></b></XML>");

    expect(detailed.exit == iiXml::writer::validation_exit::valid,
        "document with cross nested tags should validate");
}

void validates_many_cross_nested_tags() {
    const iiXml::writer::InputValidator validator;

    const iiXml::writer::validation_result detailed = validator.validate_result(
        "<!DOCTYPE XML>\n<XML><p><bold><italic>text</p><p>really</bold> useful</italic></p></XML>"
    );

    expect(detailed.exit == iiXml::writer::validation_exit::valid,
        "document with many cross nested tags should validate");
}

} // namespace

int main() {
    returns_valid_for_valid_document();
    returns_invalid_xml_file_when_doctype_fails();
    returns_invalid_tag_closure_when_tag_closure_fails();
    returns_invalid_tag_closure_for_unclosed_tag();
    validates_self_closing_tags();
    validates_cross_nested_tags();
    validates_many_cross_nested_tags();

    return failures == 0 ? 0 : 1;
}
