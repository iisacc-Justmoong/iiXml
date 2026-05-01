#include <iiXml>

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
    const iiXml::Writer::InputValidator validator;

    const iiXml::Writer::ValidationExit result =
        validator.Validate("<!Doctype XML>\n<XML><number>1</number></XML>");
    const iiXml::Writer::ValidationResult detailed =
        validator.ValidateResult("<!Doctype XML>\n<XML><number>1</number></XML>");

    expect(result == iiXml::Writer::ValidationExit::Valid,
        "valid document should return valid");
    expect(detailed.Exit == iiXml::Writer::ValidationExit::Valid,
        "valid document should return detailed valid exit");
    expect(!detailed.Reason.empty(), "valid document should return non-empty reason");
}

void returns_invalid_xml_file_when_doctype_fails() {
    const iiXml::Writer::InputValidator validator;

    const iiXml::Writer::ValidationExit result =
        validator.Validate("<XML><number>1</number></XML>");
    const iiXml::Writer::ValidationResult detailed =
        validator.ValidateResult("<XML><number>1</number></XML>");

    expect(result == iiXml::Writer::ValidationExit::InvalidXmlFile,
        "document without top Doctype/XML declaration should return invalid_xml_file");
    expect(detailed.Exit == iiXml::Writer::ValidationExit::InvalidXmlFile,
        "document without top Doctype/XML declaration should return detailed invalid_xml_file");
    expect(!detailed.Reason.empty(), "invalid XML file should return non-empty reason");
}

void returns_invalid_tag_closure_when_tag_closure_fails() {
    const iiXml::Writer::InputValidator validator;

    const iiXml::Writer::ValidationExit result =
        validator.Validate("<!Doctype XML>\n<XML><number>1</XML>");
    const iiXml::Writer::ValidationResult detailed =
        validator.ValidateResult("<!Doctype XML>\n<XML><number>1</XML>");

    expect(result == iiXml::Writer::ValidationExit::InvalidTagClosure,
        "mismatched close tag should return invalid_tag_closure");
    expect(detailed.Exit == iiXml::Writer::ValidationExit::InvalidTagClosure,
        "mismatched close tag should return detailed invalid_tag_closure");
    expect(!detailed.Reason.empty(), "invalid tag closure should return non-empty reason");
}

void returns_invalid_tag_closure_for_unclosed_tag() {
    const iiXml::Writer::InputValidator validator;

    const iiXml::Writer::ValidationExit result =
        validator.Validate("<!Doctype XML>\n<XML><number>1</number>");
    const iiXml::Writer::ValidationResult detailed =
        validator.ValidateResult("<!Doctype XML>\n<XML><number>1</number>");

    expect(result == iiXml::Writer::ValidationExit::InvalidTagClosure,
        "unclosed tag should return invalid_tag_closure");
    expect(detailed.Exit == iiXml::Writer::ValidationExit::InvalidTagClosure,
        "unclosed tag should return detailed invalid_tag_closure");
    expect(!detailed.Reason.empty(), "unclosed tag should return non-empty reason");
}

void validates_self_closing_tags() {
    const iiXml::Writer::InputValidator validator;

    expect(validator.HasValidTagClosure("<XML><empty /></XML>"),
        "self-closing tags should not require a close tag");
}

void validates_cross_nested_tags() {
    const iiXml::Writer::InputValidator validator;

    expect(validator.HasValidTagClosure("<a><b></a></b>"),
        "cross nested tags should be accepted by the open tag policy");

    const iiXml::Writer::ValidationResult detailed =
        validator.ValidateResult("<!Doctype XML>\n<XML><a><b></a></b></XML>");

    expect(detailed.Exit == iiXml::Writer::ValidationExit::Valid,
        "document with cross nested tags should validate");
}

void validates_many_cross_nested_tags() {
    const iiXml::Writer::InputValidator validator;

    const iiXml::Writer::ValidationResult detailed = validator.ValidateResult(
        "<!Doctype XML>\n<XML><p><bold><italic>text</p><p>really</bold> useful</italic></p></XML>"
    );

    expect(detailed.Exit == iiXml::Writer::ValidationExit::Valid,
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
