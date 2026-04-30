#include "iiXml.h"

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

void matches_xml_declaration_at_top() {
    const iiXml::elements::DOCTYPE doctype;

    const iiXml::elements::doctype_result matched =
        doctype.match_top("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root></root>");
    const iiXml::elements::doctype_result result =
        doctype.match_top_result("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root></root>");

    expect(matched.match.has_value(), "xml declaration should match at document top");
    expect(!matched.reason.empty(), "xml declaration should return non-empty reason");
    expect(result.status == iiXml::elements::doctype_status::matched,
        "xml declaration should return matched status");
    expect(result.match.has_value(), "xml declaration result should include match");
    expect(!result.reason.empty(), "xml declaration result should include non-empty reason");
    if (!matched.match.has_value()) {
        return;
    }

    expect(matched.match->kind == iiXml::elements::doctype_kind::xml_declaration,
        "xml declaration kind should be returned");
    expect(matched.match->raw == "<?xml version=\"1.0\" encoding=\"UTF-8\"?>",
        "xml declaration raw text should be preserved");
    expect(doctype.is_top_xml_declaration("<?xml version=\"1.0\"?>"),
        "xml declaration predicate should return true");
}

void matches_doctype_at_top() {
    const iiXml::elements::DOCTYPE doctype;

    const iiXml::elements::doctype_result matched =
        doctype.match_top("<!DOCTYPE XML SYSTEM \"iixml.dtd\">\n<XML></XML>");
    const iiXml::elements::doctype_result result =
        doctype.match_top_result("<!DOCTYPE XML SYSTEM \"iixml.dtd\">\n<XML></XML>");

    expect(matched.match.has_value(), "doctype declaration should match at document top");
    expect(!matched.reason.empty(), "doctype declaration should return non-empty reason");
    expect(result.status == iiXml::elements::doctype_status::matched,
        "doctype declaration should return matched status");
    expect(result.match.has_value(), "doctype declaration result should include match");
    expect(!result.reason.empty(), "doctype declaration result should include non-empty reason");
    if (!matched.match.has_value()) {
        return;
    }

    expect(matched.match->kind == iiXml::elements::doctype_kind::doctype_declaration,
        "doctype declaration kind should be returned");
    expect(matched.match->raw == "<!DOCTYPE XML SYSTEM \"iixml.dtd\">",
        "doctype declaration raw text should be preserved");
    expect(doctype.is_top_doctype("<!DOCTYPE XML>"),
        "doctype predicate should return true");
}

void matches_arbitrary_doctype_name() {
    const iiXml::elements::DOCTYPE doctype;

    const iiXml::elements::doctype_result matched =
        doctype.match_top("<!DOCTYPE ABCD>\n<ABCD></ABCD>");

    expect(matched.status == iiXml::elements::doctype_status::matched,
        "doctype name should not be limited to XML");
    expect(!matched.reason.empty(), "arbitrary doctype should return non-empty reason");
    if (!matched.match.has_value()) {
        return;
    }

    expect(matched.match->kind == iiXml::elements::doctype_kind::doctype_declaration,
        "arbitrary doctype should return doctype declaration kind");
    expect(matched.match->raw == "<!DOCTYPE ABCD>",
        "arbitrary doctype raw text should be preserved");
}

void matches_doctype_with_internal_subset() {
    const iiXml::elements::DOCTYPE doctype;

    const iiXml::elements::doctype_result matched =
        doctype.match_top("<!DOCTYPE XML [<!ELEMENT XML ANY>]>\n<XML></XML>");

    expect(matched.status == iiXml::elements::doctype_status::matched,
        "doctype declaration with internal subset should match");
    expect(!matched.reason.empty(), "doctype internal subset should return non-empty reason");
    if (!matched.match.has_value()) {
        return;
    }

    expect(matched.match->raw == "<!DOCTYPE XML [<!ELEMENT XML ANY>]>",
        "doctype internal subset should be included in raw text");
}

void rejects_non_top_declaration() {
    const iiXml::elements::DOCTYPE doctype;
    const iiXml::elements::doctype_result result =
        doctype.match_top_result("<root></root>\n<!DOCTYPE XML>");

    expect(!doctype.match_top("<root></root>\n<!DOCTYPE XML>").match.has_value(),
        "doctype after root tag should not match as top declaration");
    expect(result.status == iiXml::elements::doctype_status::no_top_declaration,
        "doctype after root tag should return no_top_declaration status");
    expect(!result.reason.empty(), "doctype rejection should return non-empty reason");
}

void rejects_malformed_declarations() {
    const iiXml::elements::DOCTYPE doctype;
    const iiXml::elements::doctype_result xml_result =
        doctype.match_top_result("<?xml version=\"1.0\">");
    const iiXml::elements::doctype_result doctype_result =
        doctype.match_top_result("<!DOCTYPE >");

    expect(!doctype.match_top("<?xml version=\"1.0\">").match.has_value(),
        "xml declaration without processing instruction end should fail");
    expect(xml_result.status == iiXml::elements::doctype_status::malformed_xml_declaration,
        "malformed XML declaration should return malformed_xml_declaration status");
    expect(!xml_result.reason.empty(), "malformed XML declaration should return non-empty reason");
    expect(!doctype.match_top("<!DOCTYPE >").match.has_value(),
        "doctype without root name should fail");
    expect(doctype_result.status == iiXml::elements::doctype_status::malformed_doctype_declaration,
        "malformed DOCTYPE declaration should return malformed_doctype_declaration status");
    expect(!doctype_result.reason.empty(), "malformed DOCTYPE declaration should return non-empty reason");
}

} // namespace

int main() {
    matches_xml_declaration_at_top();
    matches_doctype_at_top();
    matches_arbitrary_doctype_name();
    matches_doctype_with_internal_subset();
    rejects_non_top_declaration();
    rejects_malformed_declarations();

    return failures == 0 ? 0 : 1;
}
