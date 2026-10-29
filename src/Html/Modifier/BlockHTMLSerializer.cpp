#include "BlockHTMLSerializer.h"

#include <QDebug>
#include <QString>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace iiHtmlBlock {

namespace {

struct SerializationUnit {
    std::size_t raw_begin;
    std::size_t raw_end;
    std::string raw;
};

QString FromUtf8String(const std::string& value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

bool HasInvalidRange(std::size_t begin, std::size_t end) {
    return begin > end;
}

std::string SerializeNonOverlappingUnits(std::vector<SerializationUnit> units) {
    std::sort(
        units.begin(),
        units.end(),
        [](const SerializationUnit& left, const SerializationUnit& right) {
            if (left.raw_begin != right.raw_begin) {
                return left.raw_begin < right.raw_begin;
            }

            return left.raw_end > right.raw_end;
        }
    );

    std::string html;
    std::optional<std::size_t> furthestEnd;

    for (const SerializationUnit& unit : units) {
        // Starts are sorted, so the largest emitted end decides containment.
        if (furthestEnd && unit.raw_end <= *furthestEnd) {
            continue;
        }

        html += unit.raw;
        furthestEnd = unit.raw_end;
    }

    return html;
}

bool ContainsIndex(const std::vector<std::size_t>& indexes, std::size_t index) {
    return std::binary_search(indexes.begin(), indexes.end(), index);
}

std::vector<std::size_t> CollectCombinedSourceIndexes(
    const std::vector<CombineBlock::CombinedBlock>& combined_blocks
) {
    std::vector<std::size_t> indexes;
    for (const CombineBlock::CombinedBlock& combined : combined_blocks) {
        indexes.insert(indexes.end(), combined.source_indexes.begin(), combined.source_indexes.end());
    }

    std::sort(indexes.begin(), indexes.end());
    indexes.erase(std::unique(indexes.begin(), indexes.end()), indexes.end());
    return indexes;
}

} // namespace

BlockHTMLSerializer::BlockHTMLSerializer(QObject* parent)
    : QObject(parent) {
    qDebug() << "BlockHTMLSerializer::BlockHTMLSerializer constructed";
}

bool BlockHTMLSerializer::SerializeBlocks(const std::vector<DivideBlock::ElementInfo>& blocks) {
    std::vector<SerializationUnit> units;
    units.reserve(blocks.size());

    for (const DivideBlock::ElementInfo& block : blocks) {
        if (HasInvalidRange(block.raw_begin, block.raw_end)) {
            SetError("block range is reversed");
            qDebug() << "BlockHTMLSerializer::SerializeBlocks failed"
                     << "reason=" << QString::fromStdString(error_);
            return false;
        }

        units.push_back(SerializationUnit{block.raw_begin, block.raw_end, block.raw});
    }

    SetHTML(SerializeNonOverlappingUnits(std::move(units)));
    qDebug() << "BlockHTMLSerializer::SerializeBlocks"
             << "block_count=" << blocks.size()
             << "html_size=" << html_.size();
    emit Serialized(FromUtf8String(html_));
    return true;
}

bool BlockHTMLSerializer::SerializeCombinedBlocks(
    const std::vector<CombineBlock::CombinedBlock>& combined_blocks
) {
    std::vector<SerializationUnit> units;
    units.reserve(combined_blocks.size());

    for (const CombineBlock::CombinedBlock& combined : combined_blocks) {
        if (HasInvalidRange(combined.raw_begin, combined.raw_end)) {
            SetError("combined block range is reversed");
            qDebug() << "BlockHTMLSerializer::SerializeCombinedBlocks failed"
                     << "reason=" << QString::fromStdString(error_);
            return false;
        }

        units.push_back(SerializationUnit{combined.raw_begin, combined.raw_end, combined.raw});
    }

    SetHTML(SerializeNonOverlappingUnits(std::move(units)));
    qDebug() << "BlockHTMLSerializer::SerializeCombinedBlocks"
             << "combined_count=" << combined_blocks.size()
             << "html_size=" << html_.size();
    emit Serialized(FromUtf8String(html_));
    return true;
}

bool BlockHTMLSerializer::SerializeMergedBlocks(
    const std::vector<DivideBlock::ElementInfo>& blocks,
    const std::vector<CombineBlock::CombinedBlock>& combined_blocks
) {
    const std::vector<std::size_t> combined_indexes = CollectCombinedSourceIndexes(combined_blocks);
    std::vector<SerializationUnit> units;
    units.reserve(blocks.size() + combined_blocks.size());

    for (std::size_t index = 0; index < blocks.size(); ++index) {
        const DivideBlock::ElementInfo& block = blocks[index];
        if (HasInvalidRange(block.raw_begin, block.raw_end)) {
            SetError("block range is reversed");
            qDebug() << "BlockHTMLSerializer::SerializeMergedBlocks failed"
                     << "reason=" << QString::fromStdString(error_);
            return false;
        }

        if (!ContainsIndex(combined_indexes, index)) {
            units.push_back(SerializationUnit{block.raw_begin, block.raw_end, block.raw});
        }
    }

    for (const CombineBlock::CombinedBlock& combined : combined_blocks) {
        if (HasInvalidRange(combined.raw_begin, combined.raw_end)) {
            SetError("combined block range is reversed");
            qDebug() << "BlockHTMLSerializer::SerializeMergedBlocks failed"
                     << "reason=" << QString::fromStdString(error_);
            return false;
        }

        for (std::size_t index : combined.source_indexes) {
            if (index >= blocks.size()) {
                SetError("combined source index is out of range");
                qDebug() << "BlockHTMLSerializer::SerializeMergedBlocks failed"
                         << "index=" << index
                         << "reason=" << QString::fromStdString(error_);
                return false;
            }
        }

        units.push_back(SerializationUnit{combined.raw_begin, combined.raw_end, combined.raw});
    }

    SetHTML(SerializeNonOverlappingUnits(std::move(units)));
    qDebug() << "BlockHTMLSerializer::SerializeMergedBlocks"
             << "block_count=" << blocks.size()
             << "combined_count=" << combined_blocks.size()
             << "html_size=" << html_.size();
    emit Serialized(FromUtf8String(html_));
    return true;
}

void BlockHTMLSerializer::Clear() {
    html_.clear();
    error_.clear();
}

const std::string& BlockHTMLSerializer::GetHTMLText() const {
    return html_;
}

const std::string& BlockHTMLSerializer::GetError() const {
    return error_;
}

void BlockHTMLSerializer::SetHTML(std::string html) {
    html_ = std::move(html);
    error_.clear();
}

void BlockHTMLSerializer::SetError(const std::string& error) {
    html_.clear();
    error_ = error;
    emit SerializeFailed(FromUtf8String(error_));
}

} // namespace iiHtmlBlock
