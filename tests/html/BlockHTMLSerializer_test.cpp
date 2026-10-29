#include "Src/Modifier/BlockHTMLSerializer.h"
#include "Src/Modifier/CombineBlock.h"
#include "Src/Modifier/DivideBlock.h"

#include <cassert>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#ifdef NDEBUG
#error "iiHtmlBlock executable checks must keep assertions enabled in Release."
#endif

using namespace iiHtmlBlock;

namespace {

std::vector<DivideBlock::ElementInfo> ParseBlocks() {
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

CombineBlock::CombinedBlock CombineParagraphs(const std::vector<DivideBlock::ElementInfo>& blocks) {
    CombineBlock combiner;
    combiner.SetBlocks(blocks);
    assert(combiner.SelectRange(1, 2));
    assert(combiner.CombineSelected());
    assert(combiner.GetCombinedBlocks().size() == 1);
    return combiner.GetCombinedBlocks().front();
}

void SerializesSplitBlocksWithoutDuplicatingNestedRanges() {
    const std::vector<DivideBlock::ElementInfo> blocks = ParseBlocks();

    BlockHTMLSerializer serializer;
    assert(serializer.SerializeBlocks(blocks));
    assert(serializer.GetError().empty());
    assert(serializer.GetHTMLText() ==
        "<section><p>One</p><p>Two</p><span>tail</span></section>");
}

void SerializesCombinedBlocksAsLogicalFragments() {
    const std::vector<DivideBlock::ElementInfo> blocks = ParseBlocks();
    const CombineBlock::CombinedBlock combined = CombineParagraphs(blocks);

    BlockHTMLSerializer serializer;
    assert(serializer.SerializeCombinedBlocks({combined}));
    assert(serializer.GetError().empty());
    assert(serializer.GetHTMLText() == "<p>One</p><p>Two</p>");
}

void SerializesMergedBlocksWithCombinedGroupsApplied() {
    const std::vector<DivideBlock::ElementInfo> blocks = ParseBlocks();
    const CombineBlock::CombinedBlock combined = CombineParagraphs(blocks);

    BlockHTMLSerializer serializer;
    assert(serializer.SerializeMergedBlocks(blocks, {combined}));
    assert(serializer.GetError().empty());
    assert(serializer.GetHTMLText() ==
        "<section><p>One</p><p>Two</p><span>tail</span></section>");
}

void SerializesSiblingBlocksInDocumentOrder() {
    DivideBlock divider;
    assert(divider.Parse("<XML><p>One</p><p>Two</p></XML>"));

    BlockHTMLSerializer serializer;
    assert(serializer.SerializeBlocks(divider.GetBlockElements()));
    assert(serializer.GetHTMLText() == "<p>One</p><p>Two</p>");
}

void RejectsCombinedSourceIndexOutsideBlocks() {
    const std::vector<DivideBlock::ElementInfo> blocks = ParseBlocks();
    CombineBlock::CombinedBlock combined = CombineParagraphs(blocks);
    combined.source_indexes.push_back(99);

    BlockHTMLSerializer serializer;
    assert(!serializer.SerializeMergedBlocks(blocks, {combined}));
    assert(serializer.GetHTMLText().empty());
    assert(!serializer.GetError().empty());
}
void RangeProjectionMatchesIndependentContainment() {
    std::uint32_t seed=123;
    for (int round=0;round<20;++round) {
        std::vector<CombineBlock::CombinedBlock> blocks;
        for (int i=0;i<100;++i) {
            seed=seed*1664525U+1013904223U;
            CombineBlock::CombinedBlock block{};
            block.raw_begin=seed%200;
            seed=seed*1664525U+1013904223U;
            block.raw_end=block.raw_begin+seed%30;
            block.raw="["+std::to_string(i)+"]";
            blocks.push_back(std::move(block));
        }
        auto ordered=blocks;
        std::sort(ordered.begin(),ordered.end(),[](const auto& a,const auto& b) {
            return a.raw_begin==b.raw_begin ? a.raw_end>b.raw_end : a.raw_begin<b.raw_begin;
        });
        std::vector<CombineBlock::CombinedBlock> emitted;
        std::string expected;
        for (const auto& block:ordered) {
            const auto contained=std::any_of(emitted.begin(),emitted.end(),[&](const auto& prior) {
                return prior.raw_begin<=block.raw_begin && block.raw_end<=prior.raw_end;
            });
            if (!contained) {expected+=block.raw;emitted.push_back(block);}
        }
        BlockHTMLSerializer serializer;
        assert(serializer.SerializeCombinedBlocks(blocks));
        assert(serializer.GetHTMLText()==expected);
    }
    std::vector<CombineBlock::CombinedBlock> adjacent(10000);
    for (std::size_t i=0;i<adjacent.size();++i) {
        adjacent[i].raw_begin=i;adjacent[i].raw_end=i+1;adjacent[i].raw="x";
    }
    BlockHTMLSerializer serializer;
    assert(serializer.SerializeCombinedBlocks(adjacent));
    assert(serializer.GetHTMLText()==std::string(adjacent.size(),'x'));
}

} // namespace

int main() {
    SerializesSplitBlocksWithoutDuplicatingNestedRanges();
    SerializesCombinedBlocksAsLogicalFragments();
    SerializesMergedBlocksWithCombinedGroupsApplied();
    SerializesSiblingBlocksInDocumentOrder();
    RejectsCombinedSourceIndexOutsideBlocks();
    RangeProjectionMatchesIndependentContainment();
    return 0;
}
