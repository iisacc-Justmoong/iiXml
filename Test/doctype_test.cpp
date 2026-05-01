#include <iiXml>

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
    const iiXml::Elements::Doctype doctype;

    const iiXml::Elements::DoctypeResult matched =
        doctype.MatchTop("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root></root>");
    const iiXml::Elements::DoctypeResult result =
        doctype.MatchTopResult("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root></root>");

    expect(matched.Match.has_value(), "xml declaration should match at document top");
    expect(!matched.Reason.empty(), "xml declaration should return non-empty reason");
    expect(result.Status == iiXml::Elements::DoctypeStatus::Matched,
        "xml declaration should return matched status");
    expect(result.Match.has_value(), "xml declaration result should include match");
    expect(!result.Reason.empty(), "xml declaration result should include non-empty reason");
    if (!matched.Match.has_value()) {
        return;
    }

    expect(matched.Match->Kind == iiXml::Elements::DoctypeKind::XmlDeclaration,
        "xml declaration kind should be returned");
    expect(matched.Match->Raw == "<?xml version=\"1.0\" encoding=\"UTF-8\"?>",
        "xml declaration raw text should be preserved");
    expect(doctype.IsTopXmlDeclaration("<?xml version=\"1.0\"?>"),
        "xml declaration predicate should return true");
}

void matches_doctype_at_top() {
    const iiXml::Elements::Doctype doctype;

    const iiXml::Elements::DoctypeResult matched =
        doctype.MatchTop("<!Doctype XML SYSTEM \"iixml.dtd\">\n<XML></XML>");
    const iiXml::Elements::DoctypeResult result =
        doctype.MatchTopResult("<!Doctype XML SYSTEM \"iixml.dtd\">\n<XML></XML>");

    expect(matched.Match.has_value(), "doctype declaration should match at document top");
    expect(!matched.Reason.empty(), "doctype declaration should return non-empty reason");
    expect(result.Status == iiXml::Elements::DoctypeStatus::Matched,
        "doctype declaration should return matched status");
    expect(result.Match.has_value(), "doctype declaration result should include match");
    expect(!result.Reason.empty(), "doctype declaration result should include non-empty reason");
    if (!matched.Match.has_value()) {
        return;
    }

    expect(matched.Match->Kind == iiXml::Elements::DoctypeKind::DoctypeDeclaration,
        "doctype declaration kind should be returned");
    expect(matched.Match->Raw == "<!Doctype XML SYSTEM \"iixml.dtd\">",
        "doctype declaration raw text should be preserved");
    expect(doctype.IsTopDoctype("<!Doctype XML>"),
        "doctype predicate should return true");
}

void matches_arbitrary_doctype_name() {
    const iiXml::Elements::Doctype doctype;

    const iiXml::Elements::DoctypeResult matched =
        doctype.MatchTop("<!Doctype ABCD>\n<ABCD></ABCD>");

    expect(matched.Status == iiXml::Elements::DoctypeStatus::Matched,
        "doctype name should not be limited to XML");
    expect(!matched.Reason.empty(), "arbitrary doctype should return non-empty reason");
    if (!matched.Match.has_value()) {
        return;
    }

    expect(matched.Match->Kind == iiXml::Elements::DoctypeKind::DoctypeDeclaration,
        "arbitrary doctype should return doctype declaration kind");
    expect(matched.Match->Raw == "<!Doctype ABCD>",
        "arbitrary doctype raw text should be preserved");
}

void matches_doctype_with_internal_subset() {
    const iiXml::Elements::Doctype doctype;

    const iiXml::Elements::DoctypeResult matched =
        doctype.MatchTop("<!Doctype XML [<!ELEMENT XML ANY>]>\n<XML></XML>");

    expect(matched.Status == iiXml::Elements::DoctypeStatus::Matched,
        "doctype declaration with internal subset should match");
    expect(!matched.Reason.empty(), "doctype internal subset should return non-empty reason");
    if (!matched.Match.has_value()) {
        return;
    }

    expect(matched.Match->Raw == "<!Doctype XML [<!ELEMENT XML ANY>]>",
        "doctype internal subset should be included in raw text");
}

void rejects_non_top_declaration() {
    const iiXml::Elements::Doctype doctype;
    const iiXml::Elements::DoctypeResult result =
        doctype.MatchTopResult("<root></root>\n<!Doctype XML>");

    expect(!doctype.MatchTop("<root></root>\n<!Doctype XML>").Match.has_value(),
        "doctype after root tag should not match as top declaration");
    expect(result.Status == iiXml::Elements::DoctypeStatus::NoTopDeclaration,
        "doctype after root tag should return no_top_declaration status");
    expect(!result.Reason.empty(), "doctype rejection should return non-empty reason");
}

void rejects_malformed_declarations() {
    const iiXml::Elements::Doctype doctype;
    const iiXml::Elements::DoctypeResult xml_result =
        doctype.MatchTopResult("<?xml version=\"1.0\">");
    const iiXml::Elements::DoctypeResult DoctypeResult =
        doctype.MatchTopResult("<!Doctype >");

    expect(!doctype.MatchTop("<?xml version=\"1.0\">").Match.has_value(),
        "xml declaration without processing instruction end should fail");
    expect(xml_result.Status == iiXml::Elements::DoctypeStatus::MalformedXmlDeclaration,
        "malformed XML declaration should return malformed_xml_declaration status");
    expect(!xml_result.Reason.empty(), "malformed XML declaration should return non-empty reason");
    expect(!doctype.MatchTop("<!Doctype >").Match.has_value(),
        "doctype without root name should fail");
    expect(DoctypeResult.Status == iiXml::Elements::DoctypeStatus::MalformedDoctypeDeclaration,
        "malformed Doctype declaration should return malformed_doctype_declaration status");
    expect(!DoctypeResult.Reason.empty(), "malformed Doctype declaration should return non-empty reason");
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
