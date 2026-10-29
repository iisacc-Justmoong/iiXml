#include "Src/Parser/GetHTML.h"

#include <cassert>
#include <string>

using namespace iiHtmlBlock;

namespace {

void KeepsKnownHtmlTags() {
    const std::string xml =
        "<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE XML>\n"
        "<section id=\"intro\"><p>Hello <strong>HTML</strong></p></section>";

    GetHTML html;
    assert(html.Parse(xml));
    assert(html.GetError().empty());
    assert(html.GetSourceTagName() == "section");
    assert(html.GetTagName() == "section");
    assert(html.GetHTMLText() == "<section id=\"intro\"><p>Hello <strong>HTML</strong></p></section>");
}

void KeepsIiXmlSemanticTagsAsCustomTags() {
    GetHTML html;
    assert(html.Parse("<paragraph order=1 visible=true>Hello <bold>world</bold></paragraph>"));
    assert(html.GetSourceTagName() == "paragraph");
    assert(html.GetTagName() == "paragraph");
    assert(html.GetHTMLText()
        == "<paragraph order=\"1\" visible=\"true\">Hello <bold>world</bold></paragraph>");
}

void KeepsUnknownXmlTagsAsCustomTags() {
    GetHTML html;
    assert(html.Parse("<XML id=\"root\"><CustomTag value=\"x\">Text</CustomTag></XML>"));
    assert(html.GetSourceTagName() == "XML");
    assert(html.GetTagName() == "XML");
    assert(html.GetHTMLText()
        == "<XML id=\"root\"><CustomTag value=\"x\">Text</CustomTag></XML>");
}

void KeepsArbitraryUnknownTagNamesExactly() {
    GetHTML html;

    assert(html.Parse("<unknownName data-key=\"x\">value</unknownName>"));
    assert(html.GetSourceTagName() == "unknownName");
    assert(html.GetTagName() == "unknownName");
    assert(html.GetHTMLText() == "<unknownName data-key=\"x\">value</unknownName>");

    assert(html.Parse("<OtherWidget count=2>value</OtherWidget>"));
    assert(html.GetSourceTagName() == "OtherWidget");
    assert(html.GetTagName() == "OtherWidget");
    assert(html.GetHTMLText() == "<OtherWidget count=\"2\">value</OtherWidget>");
}

void ClearsStateOnFailure() {
    GetHTML html;
    assert(html.Parse("<p>ok</p>"));
    assert(!html.GetHTMLText().empty());

    assert(!html.Parse("<p>broken"));
    assert(html.GetHTMLText().empty());
    assert(html.GetTagName().empty());
    assert(html.GetSourceTagName().empty());
    assert(!html.GetError().empty());
}

} // namespace

int main() {
    KeepsKnownHtmlTags();
    KeepsIiXmlSemanticTagsAsCustomTags();
    KeepsUnknownXmlTagsAsCustomTags();
    KeepsArbitraryUnknownTagNamesExactly();
    ClearsStateOnFailure();
    return 0;
}
