#include "Src/Modifier/BlockRangeTracker.h"
#include "Src/Modifier/DivideBlock.h"

#include <cassert>
#include <string>
#include <vector>

using namespace iiHtmlBlock;

namespace {

const BlockRangeTracker::TrackedBlock* FindByValue(
    const std::vector<BlockRangeTracker::TrackedBlock>& blocks,
    const std::string& value
) {
    for (const BlockRangeTracker::TrackedBlock& block : blocks) {
        if (block.element.value == value) {
            return &block;
        }
    }

    return nullptr;
}

void TracksDocumentRangesIncludingDeclarationPrefix() {
    const std::string document =
        "<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE XML>\n"
        "<XML><section><p>One</p><p>Two</p><span>tail</span></section></XML>";

    BlockRangeTracker tracker;
    assert(tracker.Parse(document));
    assert(tracker.GetError().empty());

    const std::vector<BlockRangeTracker::TrackedBlock>& blocks = tracker.GetTrackedBlocks();
    assert(blocks.size() == 3);
    assert(blocks[0].id == 1);
    assert(blocks[0].source_index == 0);
    assert(blocks[0].element.tag_name == "section");

    const BlockRangeTracker::TrackedBlock* paragraph = FindByValue(blocks, "One");
    assert(paragraph != nullptr);
    assert(paragraph->raw_begin == document.find("<p>One</p>"));
    assert(paragraph->raw_end == paragraph->raw_begin + std::string("<p>One</p>").size());
    assert(tracker.GetBlockByRange(paragraph->raw_begin, paragraph->raw_end) == paragraph);
}

void FindsInnermostAndOverlappingBlocksByRange() {
    const std::string document =
        "<XML><section><p>One</p><span>tail</span></section></XML>";

    BlockRangeTracker tracker;
    assert(tracker.Parse(document));

    const std::size_t one_offset = document.find("One");
    const BlockRangeTracker::TrackedBlock* innermost = tracker.GetInnermostBlockAt(one_offset);
    assert(innermost != nullptr);
    assert(innermost->element.tag_name == "p");

    const std::size_t tail_offset = document.find("tail");
    const BlockRangeTracker::TrackedBlock* containing_tail = tracker.GetInnermostBlockAt(tail_offset);
    assert(containing_tail != nullptr);
    assert(containing_tail->element.tag_name == "section");

    const std::vector<BlockRangeTracker::TrackedBlock> overlapping =
        tracker.GetBlocksOverlapping(one_offset, one_offset + 1);
    assert(overlapping.size() == 2);
    assert(overlapping[0].element.tag_name == "section");
    assert(overlapping[1].element.tag_name == "p");
}

void PreservesIdsWhenRangesMoveAfterUpdate() {
    BlockRangeTracker tracker;
    assert(tracker.Parse("<XML><p>One</p><p>Two</p></XML>"));

    const BlockRangeTracker::TrackedBlock* one = FindByValue(tracker.GetTrackedBlocks(), "One");
    const BlockRangeTracker::TrackedBlock* two = FindByValue(tracker.GetTrackedBlocks(), "Two");
    assert(one != nullptr);
    assert(two != nullptr);
    const std::size_t one_id = one->id;
    const std::size_t two_id = two->id;

    const std::string updated = "<XML><p>Zero</p><p>One</p><p>Two</p></XML>";
    assert(tracker.Update(updated));
    assert(tracker.GetTrackedBlocks().size() == 3);

    const BlockRangeTracker::TrackedBlock* moved_one = FindByValue(tracker.GetTrackedBlocks(), "One");
    const BlockRangeTracker::TrackedBlock* moved_two = FindByValue(tracker.GetTrackedBlocks(), "Two");
    const BlockRangeTracker::TrackedBlock* zero = FindByValue(tracker.GetTrackedBlocks(), "Zero");
    assert(moved_one != nullptr);
    assert(moved_two != nullptr);
    assert(zero != nullptr);
    assert(moved_one->id == one_id);
    assert(moved_two->id == two_id);
    assert(zero->id != one_id);
    assert(zero->id != two_id);
    assert(moved_one->raw_begin == updated.find("<p>One</p>"));
}

void KeepsStateWhenUpdateFails() {
    BlockRangeTracker tracker;
    assert(tracker.Parse("<XML><p>One</p></XML>"));

    const std::string previous_html = tracker.GetHTMLText();
    const std::vector<BlockRangeTracker::TrackedBlock> previous_blocks = tracker.GetTrackedBlocks();

    assert(!tracker.Update("<XML><p>broken</XML>"));
    assert(!tracker.GetError().empty());
    assert(tracker.GetHTMLText() == previous_html);
    assert(tracker.GetTrackedBlocks().size() == previous_blocks.size());
    assert(tracker.GetTrackedBlocks().front().id == previous_blocks.front().id);
}

void TracksProvidedBlocksWithoutSourceDocument() {
    DivideBlock divider;
    assert(divider.Parse("<XML><section><p>One</p></section></XML>"));

    BlockRangeTracker tracker;
    tracker.SetBlocks(divider.GetBlockElements());
    assert(tracker.GetHTMLText().empty());
    assert(tracker.GetTrackedBlocks().size() == 2);
    assert(tracker.GetTrackedBlocks()[0].raw_begin == divider.GetBlockElements()[0].raw_begin);
    assert(tracker.GetTrackedBlocks()[1].element.value == "One");
}

} // namespace

int main() {
    TracksDocumentRangesIncludingDeclarationPrefix();
    FindsInnermostAndOverlappingBlocksByRange();
    PreservesIdsWhenRangesMoveAfterUpdate();
    KeepsStateWhenUpdateFails();
    TracksProvidedBlocksWithoutSourceDocument();
    return 0;
}
