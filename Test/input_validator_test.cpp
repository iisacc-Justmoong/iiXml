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

    expect(result == iiXml::writer::validation_exit::valid,
        "valid document should return valid");
}

void returns_invalid_xml_file_when_doctype_fails() {
    const iiXml::writer::InputValidator validator;

    const iiXml::writer::validation_exit result =
        validator.validate("<XML><number>1</number></XML>");

    expect(result == iiXml::writer::validation_exit::invalid_xml_file,
        "document without top DOCTYPE/XML declaration should return invalid_xml_file");
}

void returns_invalid_tag_closure_when_tag_closure_fails() {
    const iiXml::writer::InputValidator validator;

    const iiXml::writer::validation_exit result =
        validator.validate("<!DOCTYPE XML>\n<XML><number>1</XML>");

    expect(result == iiXml::writer::validation_exit::invalid_tag_closure,
        "mismatched close tag should return invalid_tag_closure");
}

void returns_invalid_tag_closure_for_unclosed_tag() {
    const iiXml::writer::InputValidator validator;

    const iiXml::writer::validation_exit result =
        validator.validate("<!DOCTYPE XML>\n<XML><number>1</number>");

    expect(result == iiXml::writer::validation_exit::invalid_tag_closure,
        "unclosed tag should return invalid_tag_closure");
}

void validates_self_closing_tags() {
    const iiXml::writer::InputValidator validator;

    expect(validator.has_valid_tag_closure("<XML><empty /></XML>"),
        "self-closing tags should not require a close tag");
}

} // namespace

int main() {
    returns_valid_for_valid_document();
    returns_invalid_xml_file_when_doctype_fails();
    returns_invalid_tag_closure_when_tag_closure_fails();
    returns_invalid_tag_closure_for_unclosed_tag();
    validates_self_closing_tags();

    return failures == 0 ? 0 : 1;
}
