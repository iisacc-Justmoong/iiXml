#include <iiXml>
#include <iiHtmlBlock>
#include <Src/Parser/GetHTML.h>
#include <src/Html/Parser/GetHTML.h>
#include <Src/Modifier/DivideBlock.h>
#include <src/Html/Modifier/DivideBlock.h>
#include <Src/Parser/TagParser.h>
#include <src/Parser/TagParser.h>

int main() {
    iiHtmlBlock::GetHTML html;
    if (!html.Parse("<section><p>Hello</p></section>")) return 1;
    iiHtmlBlock::DivideBlock blocks;
    if (!blocks.Parse(html.GetHTMLText()) || blocks.GetBlockElements().size() != 2) return 2;
    const iiXml::Parser::TagParser parser;
    return parser.Parse("<v>1</v>") ? 0 : 3;
}
