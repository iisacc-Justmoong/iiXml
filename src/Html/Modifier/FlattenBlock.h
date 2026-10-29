#pragma once

#include "src/Html/Modifier/DivideBlock.h"

#include <QObject>
#include <QString>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace iiHtmlBlock {

class FlattenBlock : public QObject {
    Q_OBJECT

public:
    struct LayerBlock {
        std::vector<std::size_t> source_indexes;
        std::vector<DivideBlock::ElementInfo> elements;
        std::string tag_name;
        std::string value;
        std::string raw;
        std::size_t raw_begin;
        std::size_t value_begin;
        std::size_t value_end;
        std::size_t raw_end;
        bool is_block;
    };

    explicit FlattenBlock(QObject* parent = nullptr);

    [[nodiscard]] bool Parse(const char* html);
    [[nodiscard]] bool Parse(std::string_view html);
    [[nodiscard]] bool Parse(const QString& html);

    void SetBlocks(const std::vector<DivideBlock::ElementInfo>& blocks);
    [[nodiscard]] bool LayerAll();
    void ClearLayer();

    [[nodiscard]] const std::vector<DivideBlock::ElementInfo>& GetBlocks() const;
    [[nodiscard]] const LayerBlock* GetLayerBlock() const;
    [[nodiscard]] bool HasLayer() const;
    [[nodiscard]] const std::string& GetError() const;

public slots:
    void ParseHtml(const QString& html);
    void LayerBlocks();

signals:
    void BlocksChanged(int block_count);
    void Layered(int block_count);
    void LayerFailed(const QString& reason);

private:
    void Clear();
    void SetError(const std::string& error);

    std::vector<DivideBlock::ElementInfo> blocks_;
    LayerBlock layer_block_;
    bool has_layer_;
    std::string error_;
};

} // namespace iiHtmlBlock
