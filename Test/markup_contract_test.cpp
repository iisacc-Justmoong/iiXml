#include <iiXml/Markup/TagParser.h>
#include <iiXml/Markup/BlockRangeTracker.h>
#include <cstdlib>
#include <iostream>
#include <string>

#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " #x "\n"; std::exit(1); } } while(false)
int main() {
    ii::xml::markup::Parser::TagParser parser;
    const auto document=parser.ParseAllDocument("<root id=\"x\"><p>한글</p></root>");
    CHECK(document && document->Nodes.size()==1);
    CHECK(document->Nodes[0].Children.size()==1);
    CHECK(document->ValueView(document->Nodes[0].Children[0])=="한글");
    // The iiXml range parser deliberately admits overlapping ranges; the
    // document XML adapter applies its stricter tree validation separately.
    CHECK(parser.ParseAllDocument("<a><b></a></b>"));
    CHECK(!parser.ParseAllDocument("<root"));
    ii::xml::blocks::BlockRangeTracker blocks;
    const std::string first="<section><p>One</p><p>Two</p></section>";
    CHECK(blocks.Parse(first));
    const auto* old=blocks.GetInnermostBlockAt(first.find("One"));
    CHECK(old && old->element.value=="One");
    const auto id=old->id;
    const std::string second="<section><p>Zero</p><p>One</p><p>Two</p></section>";
    CHECK(blocks.Update(second));
    const auto* shifted=blocks.GetInnermostBlockAt(second.find("One"));
    CHECK(shifted && shifted->id==id);
    CHECK(!blocks.Update("<section><p>broken</section>"));
    CHECK(blocks.GetInnermostBlockAt(second.find("One"))->id==id);
}
