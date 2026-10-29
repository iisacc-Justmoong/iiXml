#include "Src/Modifier/DivideBlock.h"
#include "Src/Modifier/FlattenBlock.h"

#include <cassert>
#include <cstddef>
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
        "<div><p>Two</p></div>"
        "<span>tail</span>"
        "</section>"
        "</XML>"
    ));

    return divider.GetBlockElements();
}

void LayersAllDocumentBlocksAsOneGroup() {
    FlattenBlock flattener;
    assert(flattener.Parse(
        "<XML>"
        "<section>"
        "<p>One</p>"
        "<div><p>Two</p></div>"
        "<span>tail</span>"
        "</section>"
        "</XML>"
    ));
    assert(flattener.GetError().empty());
    assert(flattener.HasLayer());

    const FlattenBlock::LayerBlock* layer = flattener.GetLayerBlock();
    assert(layer != nullptr);
    assert(layer->tag_name == "layer");
    assert(layer->is_block);
    assert(layer->elements.size() == flattener.GetBlocks().size());
    assert(layer->source_indexes.size() == flattener.GetBlocks().size());
    assert(layer->elements.size() == 4);

    assert(layer->elements[0].tag_name == "section");
    assert(layer->elements[1].tag_name == "p");
    assert(layer->elements[2].tag_name == "div");
    assert(layer->elements[3].tag_name == "p");

    for (std::size_t index = 0; index < layer->source_indexes.size(); ++index) {
        assert(layer->source_indexes[index] == index);
        assert(layer->elements[index].tag_name == flattener.GetBlocks()[index].tag_name);
        assert(layer->elements[index].raw == flattener.GetBlocks()[index].raw);
    }

    assert(layer->raw_begin == flattener.GetBlocks().front().raw_begin);
    assert(layer->raw_end == flattener.GetBlocks().front().raw_end);
}

void LayersExistingBlockListWithoutLosingIndividualBlocks() {
    const std::vector<DivideBlock::ElementInfo> blocks = ParseBlockElements();
    FlattenBlock flattener;

    flattener.SetBlocks(blocks);
    assert(!flattener.HasLayer());
    assert(flattener.LayerAll());
    assert(flattener.GetBlocks().size() == blocks.size());

    const FlattenBlock::LayerBlock* layer = flattener.GetLayerBlock();
    assert(layer != nullptr);
    assert(layer->elements.size() == blocks.size());
    assert(layer->source_indexes.front() == 0);
    assert(layer->source_indexes.back() == blocks.size() - 1);
    assert(layer->raw.find(blocks[0].raw) != std::string::npos);
    assert(layer->raw.find(blocks[1].raw) != std::string::npos);
    assert(layer->value.find("One") != std::string::npos);
    assert(layer->value.find("Two") != std::string::npos);
}

void RejectsEmptyBlockListAndClearsPreviousLayer() {
    const std::vector<DivideBlock::ElementInfo> blocks = ParseBlockElements();
    FlattenBlock flattener;
    flattener.SetBlocks(blocks);
    assert(flattener.LayerAll());
    assert(flattener.HasLayer());

    flattener.SetBlocks({});
    assert(!flattener.LayerAll());
    assert(!flattener.HasLayer());
    assert(flattener.GetLayerBlock() == nullptr);
    assert(!flattener.GetError().empty());
}

void ClearsStateOnParseFailure() {
    FlattenBlock flattener;
    assert(flattener.Parse("<XML><p>ok</p></XML>"));
    assert(flattener.HasLayer());
    assert(!flattener.GetBlocks().empty());

    assert(!flattener.Parse("<XML><p>broken</XML>"));
    assert(flattener.GetBlocks().empty());
    assert(!flattener.HasLayer());
    assert(flattener.GetLayerBlock() == nullptr);
    assert(!flattener.GetError().empty());
}

} // namespace

int main() {
    LayersAllDocumentBlocksAsOneGroup();
    LayersExistingBlockListWithoutLosingIndividualBlocks();
    RejectsEmptyBlockListAndClearsPreviousLayer();
    ClearsStateOnParseFailure();
    return 0;
}
