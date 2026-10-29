#include <iiXml>
#include <iiHtmlBlock>
#include <string>

int main() {
    const iiXml::Parser::TagParser parser;
    const auto xml = parser.Parse("<number>123</number>");
    if (!xml || xml->TagName != "number" || xml->Value != "123") return 1;
    iiHtmlBlock::GetHTML html;
    if (!html.Parse("<section><p>Hello</p></section>")) return 2;
    if (html.GetHTMLText() != "<section><p>Hello</p></section>") return 3;
    iiHtmlBlock::DivideBlock divider;
    if (!divider.Parse(html.GetHTMLText()) || divider.GetBlockElements().size() != 2) return 4;
    return 0;
}
