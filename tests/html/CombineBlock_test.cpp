#include "Src/Modifier/CombineBlock.h"
#include "Src/Modifier/DivideBlock.h"

#include <cassert>
#include <string>
#include <vector>

using namespace iiHtmlBlock;

namespace {

std::vector<DivideBlock::ElementInfo> ParseBlockElements() {
    DivideBlock divider;
    assert(divider.Parse(
        "<XML>"
        "<section>"
        "<p>One</p>"
        "<p>Two</p>"
        "<span>tail</span>"
        "</section>"
        "</XML>"
    ));

    return divider.GetBlockElements();
}

void CombinesSelectedBlocksAsOneLogicalBlock() {
    const std::vector<DivideBlock::ElementInfo> blocks = ParseBlockElements();
    assert(blocks.size() == 3);
    assert(blocks[0].tag_name == "section");
    assert(blocks[1].tag_name == "p");
    assert(blocks[2].tag_name == "p");

    CombineBlock combiner;
    combiner.SetBlocks(blocks);
    assert(combiner.GetBlocks().size() == 3);

    assert(combiner.Select(1));
    assert(combiner.Select(2));
    assert(combiner.GetSelectedIndexes().size() == 2);
    assert(combiner.CombineSelected());
    assert(combiner.GetError().empty());
    assert(combiner.GetSelectedIndexes().empty());

    const std::vector<CombineBlock::CombinedBlock>& combined_blocks = combiner.GetCombinedBlocks();
    assert(combined_blocks.size() == 1);

    const CombineBlock::CombinedBlock& combined = combined_blocks.front();
    assert(combined.tag_name == "combined");
    assert(combined.is_block);
    assert(combined.source_indexes.size() == 2);
    assert(combined.source_indexes[0] == 1);
    assert(combined.source_indexes[1] == 2);
    assert(combined.elements.size() == 2);
    assert(combined.elements[0].value == "One");
    assert(combined.elements[1].value == "Two");
    assert(combined.value == "OneTwo");
    assert(combined.raw == "<p>One</p><p>Two</p>");
    assert(combined.raw_begin == blocks[1].raw_begin);
    assert(combined.raw_end == blocks[2].raw_end);
}

void SelectRangeKeepsIndexesSortedAndUnique() {
    const std::vector<DivideBlock::ElementInfo> blocks = ParseBlockElements();
    CombineBlock combiner;
    combiner.SetBlocks(blocks);

    assert(combiner.Select(2));
    assert(combiner.SelectRange(0, 2));

    const std::vector<std::size_t>& selected = combiner.GetSelectedIndexes();
    assert(selected.size() == 3);
    assert(selected[0] == 0);
    assert(selected[1] == 1);
    assert(selected[2] == 2);
}

void RejectsInvalidSelectionsAndTooSmallCombination() {
    const std::vector<DivideBlock::ElementInfo> blocks = ParseBlockElements();
    CombineBlock combiner;
    combiner.SetBlocks(blocks);

    assert(!combiner.Select(3));
    assert(!combiner.GetError().empty());
    assert(combiner.GetSelectedIndexes().empty());

    assert(combiner.Select(0));
    assert(!combiner.CombineSelected());
    assert(!combiner.GetError().empty());
    assert(combiner.GetCombinedBlocks().empty());
}

void ReplacesStateWhenBlocksAreReset() {
    const std::vector<DivideBlock::ElementInfo> blocks = ParseBlockElements();
    CombineBlock combiner;
    combiner.SetBlocks(blocks);

    assert(combiner.SelectRange(1, 2));
    assert(combiner.CombineSelected());
    assert(!combiner.GetCombinedBlocks().empty());

    combiner.SetBlocks({});
    assert(combiner.GetBlocks().empty());
    assert(combiner.GetSelectedIndexes().empty());
    assert(combiner.GetCombinedBlocks().empty());
    assert(combiner.GetError().empty());
}

} // namespace

int main() {
    CombinesSelectedBlocksAsOneLogicalBlock();
    SelectRangeKeepsIndexesSortedAndUnique();
    RejectsInvalidSelectionsAndTooSmallCombination();
    ReplacesStateWhenBlocksAreReset();
    return 0;
}
