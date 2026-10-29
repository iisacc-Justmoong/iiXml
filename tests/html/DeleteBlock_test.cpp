#include "Src/Modifier/DeleteBlock.h"

#include <cassert>
#include <string>
#include <vector>

using namespace iiHtmlBlock;

namespace {

void DeletesSelectedBlockAndRefreshesBlockList() {
    DeleteBlock deleter;
    assert(deleter.Parse("<XML><section><p>One</p><p>Two</p><span>tail</span></section></XML>"));
    assert(deleter.GetError().empty());
    assert(deleter.GetBlocks().size() == 3);
    assert(deleter.GetBlocks()[0].tag_name == "section");
    assert(deleter.GetBlocks()[1].tag_name == "p");
    assert(deleter.GetBlocks()[2].tag_name == "p");

    assert(deleter.Delete(1));
    assert(deleter.GetError().empty());
    assert(deleter.GetHTMLText() == "<XML><section><p>Two</p><span>tail</span></section></XML>");
    assert(deleter.GetDeletedBlocks().size() == 1);
    assert(deleter.GetDeletedBlocks()[0].raw == "<p>One</p>");

    const std::vector<DivideBlock::ElementInfo>& blocks = deleter.GetBlocks();
    assert(blocks.size() == 2);
    assert(blocks[0].tag_name == "section");
    assert(blocks[1].tag_name == "p");
    assert(blocks[1].value == "Two");
}

void DeletesOuterBlockWhenNestedBlockIsSelected() {
    DeleteBlock deleter;
    assert(deleter.Parse("<XML><section><p>One</p></section><p>After</p></XML>"));
    assert(deleter.GetBlocks().size() == 3);

    assert(deleter.Delete(0));
    assert(deleter.GetHTMLText() == "<XML><p>After</p></XML>");
    assert(deleter.GetBlocks().size() == 1);
    assert(deleter.GetBlocks()[0].value == "After");
}

void PreservesDeclarationPrefixAndUsesBodyBlockOffsets() {
    DeleteBlock deleter;
    const std::string document =
        "<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE XML>\n"
        "<XML><p>One</p><p>Two</p></XML>";

    assert(deleter.Parse(document));
    assert(deleter.GetBlocks().size() == 2);
    assert(deleter.Delete(0));
    assert(deleter.GetHTMLText() ==
        "<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE XML>\n"
        "<XML><p>Two</p></XML>");
}

void RejectsInvalidIndexWithoutChangingState() {
    DeleteBlock deleter;
    assert(deleter.Parse("<XML><p>One</p></XML>"));
    const std::string before = deleter.GetHTMLText();

    assert(!deleter.Delete(1));
    assert(!deleter.GetError().empty());
    assert(deleter.GetHTMLText() == before);
    assert(deleter.GetBlocks().size() == 1);
    assert(deleter.GetDeletedBlocks().empty());
}

void ClearsStateOnParseFailure() {
    DeleteBlock deleter;
    assert(deleter.Parse("<XML><p>One</p></XML>"));
    assert(!deleter.GetBlocks().empty());

    assert(!deleter.Parse("<XML><p>broken</XML>"));
    assert(deleter.GetHTMLText().empty());
    assert(deleter.GetBlocks().empty());
    assert(deleter.GetDeletedBlocks().empty());
    assert(!deleter.GetError().empty());
}

} // namespace

int main() {
    DeletesSelectedBlockAndRefreshesBlockList();
    DeletesOuterBlockWhenNestedBlockIsSelected();
    PreservesDeclarationPrefixAndUsesBodyBlockOffsets();
    RejectsInvalidIndexWithoutChangingState();
    ClearsStateOnParseFailure();
    return 0;
}
