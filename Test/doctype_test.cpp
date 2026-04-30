#include "iiXml.h"

#include <iostream>
#include <optional>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

void matches_xml_declaration_at_top() {
    const iiXml::elements::DOCTYPE doctype;

    const std::optional<iiXml::elements::doctype_match> matched =
        doctype.match_top("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root></root>");

    expect(matched.has_value(), "xml declaration should match at document top");
    if (!matched.has_value()) {
        return;
    }

    expect(matched->kind == iiXml::elements::doctype_kind::xml_declaration,
        "xml declaration kind should be returned");
    expect(matched->raw == "<?xml version=\"1.0\" encoding=\"UTF-8\"?>",
        "xml declaration raw text should be preserved");
    expect(doctype.is_top_xml_declaration("<?xml version=\"1.0\"?>"),
        "xml declaration predicate should return true");
}

void matches_doctype_at_top() {
    const iiXml::elements::DOCTYPE doctype;

    const std::optional<iiXml::elements::doctype_match> matched =
        doctype.match_top("<!DOCTYPE XML SYSTEM \"iixml.dtd\">\n<XML></XML>");

    expect(matched.has_value(), "doctype declaration should match at document top");
    if (!matched.has_value()) {
        return;
    }

    expect(matched->kind == iiXml::elements::doctype_kind::doctype_declaration,
        "doctype declaration kind should be returned");
    expect(matched->raw == "<!DOCTYPE XML SYSTEM \"iixml.dtd\">",
        "doctype declaration raw text should be preserved");
    expect(doctype.is_top_doctype("<!DOCTYPE XML>"),
        "doctype predicate should return true");
}

void matches_doctype_with_internal_subset() {
    const iiXml::elements::DOCTYPE doctype;

    const std::optional<iiXml::elements::doctype_match> matched =
        doctype.match_top("<!DOCTYPE XML [<!ELEMENT XML ANY>]>\n<XML></XML>");

    expect(matched.has_value(), "doctype declaration with internal subset should match");
    if (!matched.has_value()) {
        return;
    }

    expect(matched->raw == "<!DOCTYPE XML [<!ELEMENT XML ANY>]>",
        "doctype internal subset should be included in raw text");
}

void rejects_non_top_declaration() {
    const iiXml::elements::DOCTYPE doctype;

    expect(!doctype.match_top("<root></root>\n<!DOCTYPE XML>").has_value(),
        "doctype after root tag should not match as top declaration");
}

void rejects_malformed_declarations() {
    const iiXml::elements::DOCTYPE doctype;

    expect(!doctype.match_top("<?xml version=\"1.0\">").has_value(),
        "xml declaration without processing instruction end should fail");
    expect(!doctype.match_top("<!DOCTYPE >").has_value(),
        "doctype without root name should fail");
}

} // namespace

int main() {
    matches_xml_declaration_at_top();
    matches_doctype_at_top();
    matches_doctype_with_internal_subset();
    rejects_non_top_declaration();
    rejects_malformed_declarations();

    return failures == 0 ? 0 : 1;
}
