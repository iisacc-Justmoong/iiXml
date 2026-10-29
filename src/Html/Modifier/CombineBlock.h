#pragma once

#include "src/Html/Modifier/DivideBlock.h"

#include <QObject>
#include <QString>

#include <cstddef>
#include <string>
#include <vector>

namespace iiHtmlBlock {

class CombineBlock : public QObject {
    Q_OBJECT

public:
    struct CombinedBlock {
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

    explicit CombineBlock(QObject* parent = nullptr);

    void SetBlocks(const std::vector<DivideBlock::ElementInfo>& blocks);
    [[nodiscard]] bool Select(std::size_t index);
    [[nodiscard]] bool SelectRange(std::size_t first, std::size_t last);
    [[nodiscard]] bool Deselect(std::size_t index);
    void ClearSelection();
    void ClearCombinedBlocks();
    [[nodiscard]] bool CombineSelected();

    [[nodiscard]] const std::vector<DivideBlock::ElementInfo>& GetBlocks() const;
    [[nodiscard]] const std::vector<std::size_t>& GetSelectedIndexes() const;
    [[nodiscard]] const std::vector<CombinedBlock>& GetCombinedBlocks() const;
    [[nodiscard]] const std::string& GetError() const;

public slots:
    void SelectBlock(int index);
    void DeselectBlock(int index);
    void ClearSelectedBlocks();
    void CombineSelectedBlocks();

signals:
    void BlocksChanged(int block_count);
    void SelectionChanged(int selected_count);
    void Combined(int combined_count);
    void CombineFailed(const QString& reason);

private:
    [[nodiscard]] bool IsValidIndex(std::size_t index) const;
    void SetError(const std::string& error);

    std::vector<DivideBlock::ElementInfo> blocks_;
    std::vector<std::size_t> selected_indexes_;
    std::vector<CombinedBlock> combined_blocks_;
    std::string error_;
};

} // namespace iiHtmlBlock
