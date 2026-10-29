#pragma once

#include "src/Html/Modifier/DivideBlock.h"

#include <QObject>
#include <QString>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace iiHtmlBlock {

class BlockRangeTracker : public QObject {
    Q_OBJECT

public:
    struct TrackedBlock {
        std::size_t id;
        std::size_t source_index;
        DivideBlock::ElementInfo element;
        std::size_t raw_begin;
        std::size_t value_begin;
        std::size_t value_end;
        std::size_t raw_end;
    };

    explicit BlockRangeTracker(QObject* parent = nullptr);

    [[nodiscard]] bool Parse(const char* html);
    [[nodiscard]] bool Parse(std::string_view html);
    [[nodiscard]] bool Parse(const QString& html);

    [[nodiscard]] bool Update(const char* html);
    [[nodiscard]] bool Update(std::string_view html);
    [[nodiscard]] bool Update(const QString& html);

    void SetBlocks(const std::vector<DivideBlock::ElementInfo>& blocks);

    [[nodiscard]] const std::string& GetHTMLText() const;
    [[nodiscard]] const std::vector<TrackedBlock>& GetTrackedBlocks() const;
    [[nodiscard]] const TrackedBlock* GetBlockById(std::size_t id) const;
    [[nodiscard]] const TrackedBlock* GetBlockByRange(std::size_t raw_begin, std::size_t raw_end) const;
    [[nodiscard]] const TrackedBlock* GetInnermostBlockAt(std::size_t offset) const;
    [[nodiscard]] std::vector<TrackedBlock> GetBlocksOverlapping(std::size_t raw_begin, std::size_t raw_end) const;
    [[nodiscard]] const std::string& GetError() const;

public slots:
    void ParseHtml(const QString& html);
    void UpdateHtml(const QString& html);

signals:
    void Parsed(int block_count);
    void Updated(int block_count);
    void TrackFailed(const QString& reason);

private:
    [[nodiscard]] bool BuildFromHtml(std::string_view html, bool preserve_ids);
    [[nodiscard]] std::vector<TrackedBlock> BuildTrackedBlocks(
        const std::vector<DivideBlock::ElementInfo>& blocks,
        std::size_t document_offset,
        bool preserve_ids
    );
    [[nodiscard]] const TrackedBlock* FindPreservedBlock(
        const DivideBlock::ElementInfo& element,
        std::size_t raw_begin,
        std::size_t raw_end,
        const std::vector<bool>& used
    ) const;
    void Clear();
    void SetError(const std::string& error);

    std::string html_;
    std::vector<TrackedBlock> tracked_blocks_;
    std::string error_;
    std::size_t next_id_;
};

} // namespace iiHtmlBlock
