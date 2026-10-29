#include <iiHtmlBlock>

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

const iiHtmlBlock::GetTagInfo::RangeInfo* FindRange(
    const std::vector<iiHtmlBlock::GetTagInfo::RangeInfo>& ranges,
    const std::string& tag_name
) {
    for (const iiHtmlBlock::GetTagInfo::RangeInfo& range : ranges) {
        if (range.tag_name == tag_name) {
            return &range;
        }
    }

    return nullptr;
}

} // namespace

int main() {
    const std::string document =
        "<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE XML>\n"
        "<XML>"
        "<section id=\"intro\">"
        "<p order=1>Hello <strong>world</strong></p>"
        "<Custom display=block>Custom block</Custom>"
        "<span>tail</span>"
        "</section>"
        "</XML>";

    iiHtmlBlock::GetTagInfo tag_info;
    assert(tag_info.Parse(document));
    assert(tag_info.GetTagName() == "XML");
    assert(FindRange(tag_info.GetRanges(), "section") != nullptr);
    assert(FindRange(tag_info.GetRanges(), "strong") != nullptr);

    iiHtmlBlock::GetHTML html;
    assert(html.Parse(document));
    const std::string html_text = html.GetHTMLText();
    assert(html_text.find("<section id=\"intro\">") != std::string::npos);
    assert(html_text.find("<Custom display=\"block\">Custom block</Custom>") != std::string::npos);

    iiHtmlBlock::DivideBlock divider;
    assert(divider.Parse(html_text));
    const std::vector<iiHtmlBlock::DivideBlock::ElementInfo>& blocks = divider.GetBlockElements();
    assert(blocks.size() == 3);
    assert(blocks[0].tag_name == "section");
    assert(blocks[1].tag_name == "p");
    assert(blocks[2].tag_name == "Custom");

    iiHtmlBlock::BlockRangeTracker tracker;
    assert(tracker.Parse(html_text));
    const iiHtmlBlock::BlockRangeTracker::TrackedBlock* hello_block =
        tracker.GetInnermostBlockAt(html_text.find("Hello"));
    assert(hello_block != nullptr);
    assert(hello_block->element.tag_name == "p");

    iiHtmlBlock::BlockHTMLSerializer serializer;
    assert(serializer.SerializeBlocks(blocks));
    assert(serializer.GetHTMLText().find("<section id=\"intro\">") != std::string::npos);

    iiHtmlBlock::DeleteBlock deleter;
    assert(deleter.Parse(html_text));
    assert(deleter.Delete(1));
    assert(deleter.GetDeletedBlocks().front().tag_name == "p");
    assert(deleter.GetHTMLText().find("<p order=\"1\">") == std::string::npos);

    iiHtmlBlock::CombineBlock combiner;
    combiner.SetBlocks(blocks);
    assert(combiner.SelectRange(1, 2));
    assert(combiner.CombineSelected());
    assert(combiner.GetCombinedBlocks().size() == 1);
    assert(combiner.GetCombinedBlocks().front().elements.size() == 2);
    assert(serializer.SerializeCombinedBlocks(combiner.GetCombinedBlocks()));
    const std::string combined_html = serializer.GetHTMLText();
    assert(combined_html.find("<p order=\"1\">") != std::string::npos);
    assert(combined_html.find("<Custom display=\"block\">") != std::string::npos);

    iiHtmlBlock::FlattenBlock flattener;
    flattener.SetBlocks(blocks);
    assert(flattener.LayerAll());
    assert(flattener.GetLayerBlock() != nullptr);
    assert(flattener.GetLayerBlock()->elements.size() == blocks.size());

    std::cout << "root_tag=" << tag_info.GetTagName() << '\n';
    std::cout << "html=" << html_text << '\n';
    std::cout << "block_count=" << blocks.size() << '\n';
    std::cout << "tracked_hello_block_id=" << hello_block->id << '\n';
    std::cout << "combined_html=" << combined_html << '\n';
    std::cout << "deleted_html=" << deleter.GetHTMLText() << '\n';
    std::cout << "combined_count=" << combiner.GetCombinedBlocks().size() << '\n';
    std::cout << "layer_block_count=" << flattener.GetLayerBlock()->elements.size() << '\n';
    return 0;
}
