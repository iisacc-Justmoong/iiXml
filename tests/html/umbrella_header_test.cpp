#include <iiHtmlBlock>

#include <cassert>

int main() {
    iiHtmlBlock::GetHTML html;
    iiHtmlBlock::GetTagInfo info;
    iiHtmlBlock::iiXmlToHTML converter;
    iiHtmlBlock::DivideBlock divider;
    iiHtmlBlock::BlockRangeTracker tracker;
    iiHtmlBlock::BlockHTMLSerializer serializer;
    iiHtmlBlock::DeleteBlock deleter;
    iiHtmlBlock::CombineBlock combiner;
    iiHtmlBlock::FlattenBlock flattener;

    assert(html.GetHTMLText().empty());
    assert(info.GetTags().empty());
    assert(converter.GetHTMLText().empty());
    assert(divider.GetElements().empty());
    assert(tracker.GetTrackedBlocks().empty());
    assert(serializer.GetHTMLText().empty());
    assert(deleter.GetBlocks().empty());
    assert(combiner.GetBlocks().empty());
    assert(!flattener.HasLayer());
    return 0;
}
