#include "Src/Modifier/DivideBlock.h"

#include <cassert>
#include <string>
#include <vector>

using namespace iiHtmlBlock;

namespace {

const DivideBlock::ElementInfo* FindElement(
    const std::vector<DivideBlock::ElementInfo>& elements,
    const std::string& name
) {
    for (const DivideBlock::ElementInfo& element : elements) {
        if (element.tag_name == name) {
            return &element;
        }
    }

    return nullptr;
}

void ClassifiesDefaultHtmlBlockTags() {
    DivideBlock divider;
    assert(divider.Parse("<XML><section><p>Hello <strong>world</strong></p><span>tail</span></section></XML>"));
    assert(divider.GetError().empty());

    const DivideBlock::ElementInfo* root = FindElement(divider.GetElements(), "XML");
    const DivideBlock::ElementInfo* section = FindElement(divider.GetElements(), "section");
    const DivideBlock::ElementInfo* paragraph = FindElement(divider.GetElements(), "p");
    const DivideBlock::ElementInfo* strong = FindElement(divider.GetElements(), "strong");
    const DivideBlock::ElementInfo* span = FindElement(divider.GetElements(), "span");

    assert(root != nullptr);
    assert(section != nullptr);
    assert(paragraph != nullptr);
    assert(strong != nullptr);
    assert(span != nullptr);

    assert(!root->is_block);
    assert(section->is_block);
    assert(paragraph->is_block);
    assert(!strong->is_block);
    assert(!span->is_block);
    assert(divider.GetBlockElements().size() == 2);
}

void HonorsDisplayOverrides() {
    DivideBlock divider;
    assert(divider.Parse(
        "<XML>"
        "<Custom style=\"color: red; display: block\">one</Custom>"
        "<div style=\"display: inline\">two</div>"
        "<Inline display=block>three</Inline>"
        "</XML>"
    ));

    const DivideBlock::ElementInfo* custom = FindElement(divider.GetElements(), "Custom");
    const DivideBlock::ElementInfo* div = FindElement(divider.GetElements(), "div");
    const DivideBlock::ElementInfo* inline_tag = FindElement(divider.GetElements(), "Inline");

    assert(custom != nullptr);
    assert(div != nullptr);
    assert(inline_tag != nullptr);

    assert(custom->is_block);
    assert(custom->has_display_override);
    assert(custom->display_value == "block");

    assert(!div->is_block);
    assert(div->has_display_override);
    assert(div->display_value == "inline");

    assert(inline_tag->is_block);
    assert(inline_tag->has_display_override);
    assert(inline_tag->display_value == "block");
}

void ClassifiesTagsCaseInsensitively() {
    assert(DivideBlock::IsBlockTag("SECTION"));
    assert(DivideBlock::IsBlockTag("  Div  "));
    assert(!DivideBlock::IsBlockTag("Strong"));
}

void ClearsStateOnFailure() {
    DivideBlock divider;
    assert(divider.Parse("<XML><section>ok</section></XML>"));
    assert(!divider.GetElements().empty());
    assert(!divider.GetBlockElements().empty());

    assert(!divider.Parse("<XML><section>broken</XML>"));
    assert(divider.GetElements().empty());
    assert(divider.GetBlockElements().empty());
    assert(!divider.GetError().empty());
}

} // namespace

int main() {
    ClassifiesDefaultHtmlBlockTags();
    HonorsDisplayOverrides();
    ClassifiesTagsCaseInsensitively();
    ClearsStateOnFailure();
    return 0;
}
