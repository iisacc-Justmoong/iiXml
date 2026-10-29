#include "CombineBlock.h"

#include <QDebug>
#include <QString>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace iiHtmlBlock {

namespace {

QString FromUtf8String(const std::string& value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

std::vector<std::size_t> SortedUnique(std::vector<std::size_t> indexes) {
    std::sort(indexes.begin(), indexes.end());
    indexes.erase(std::unique(indexes.begin(), indexes.end()), indexes.end());
    return indexes;
}

CombineBlock::CombinedBlock BuildCombinedBlock(
    const std::vector<DivideBlock::ElementInfo>& blocks,
    const std::vector<std::size_t>& selected_indexes
) {
    CombineBlock::CombinedBlock combined{
        selected_indexes,
        {},
        "combined",
        {},
        {},
        std::numeric_limits<std::size_t>::max(),
        std::numeric_limits<std::size_t>::max(),
        0,
        0,
        true
    };

    combined.elements.reserve(selected_indexes.size());

    for (std::size_t index : selected_indexes) {
        const DivideBlock::ElementInfo& element = blocks[index];
        combined.elements.push_back(element);
        combined.value += element.value;
        combined.raw += element.raw;
        combined.raw_begin = std::min(combined.raw_begin, element.raw_begin);
        combined.value_begin = std::min(combined.value_begin, element.value_begin);
        combined.value_end = std::max(combined.value_end, element.value_end);
        combined.raw_end = std::max(combined.raw_end, element.raw_end);
    }

    if (combined.elements.empty()) {
        combined.raw_begin = 0;
        combined.value_begin = 0;
    }

    return combined;
}

} // namespace

CombineBlock::CombineBlock(QObject* parent)
    : QObject(parent) {
    qDebug() << "CombineBlock::CombineBlock constructed";
}

void CombineBlock::SetBlocks(const std::vector<DivideBlock::ElementInfo>& blocks) {
    blocks_ = blocks;
    selected_indexes_.clear();
    combined_blocks_.clear();
    error_.clear();

    qDebug() << "CombineBlock::SetBlocks"
             << "block_count=" << blocks_.size();
    emit BlocksChanged(static_cast<int>(blocks_.size()));
    emit SelectionChanged(0);
    emit Combined(0);
}

bool CombineBlock::Select(std::size_t index) {
    if (!IsValidIndex(index)) {
        SetError("block index is out of range");
        qDebug() << "CombineBlock::Select failed"
                 << "index=" << index
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    if (std::find(selected_indexes_.begin(), selected_indexes_.end(), index) == selected_indexes_.end()) {
        selected_indexes_.push_back(index);
        selected_indexes_ = SortedUnique(selected_indexes_);
        emit SelectionChanged(static_cast<int>(selected_indexes_.size()));
    }

    error_.clear();
    return true;
}

bool CombineBlock::SelectRange(std::size_t first, std::size_t last) {
    if (first > last) {
        SetError("block range is reversed");
        qDebug() << "CombineBlock::SelectRange failed"
                 << "first=" << first
                 << "last=" << last
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    if (!IsValidIndex(first) || !IsValidIndex(last)) {
        SetError("block range is out of range");
        qDebug() << "CombineBlock::SelectRange failed"
                 << "first=" << first
                 << "last=" << last
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    for (std::size_t index = first; index <= last; ++index) {
        if (std::find(selected_indexes_.begin(), selected_indexes_.end(), index) == selected_indexes_.end()) {
            selected_indexes_.push_back(index);
        }
    }

    selected_indexes_ = SortedUnique(selected_indexes_);
    error_.clear();
    emit SelectionChanged(static_cast<int>(selected_indexes_.size()));
    return true;
}

bool CombineBlock::Deselect(std::size_t index) {
    const auto found = std::find(selected_indexes_.begin(), selected_indexes_.end(), index);
    if (found == selected_indexes_.end()) {
        SetError("block index is not selected");
        qDebug() << "CombineBlock::Deselect failed"
                 << "index=" << index
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    selected_indexes_.erase(found);
    error_.clear();
    emit SelectionChanged(static_cast<int>(selected_indexes_.size()));
    return true;
}

void CombineBlock::ClearSelection() {
    selected_indexes_.clear();
    error_.clear();
    emit SelectionChanged(0);
}

void CombineBlock::ClearCombinedBlocks() {
    combined_blocks_.clear();
    error_.clear();
    emit Combined(0);
}

bool CombineBlock::CombineSelected() {
    if (selected_indexes_.size() < 2) {
        SetError("at least two blocks must be selected");
        qDebug() << "CombineBlock::CombineSelected failed"
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    selected_indexes_ = SortedUnique(selected_indexes_);
    for (std::size_t index : selected_indexes_) {
        if (!IsValidIndex(index)) {
            SetError("selected block index is out of range");
            qDebug() << "CombineBlock::CombineSelected failed"
                     << "index=" << index
                     << "reason=" << QString::fromStdString(error_);
            return false;
        }
    }

    combined_blocks_.push_back(BuildCombinedBlock(blocks_, selected_indexes_));
    selected_indexes_.clear();
    error_.clear();

    qDebug() << "CombineBlock::CombineSelected"
             << "combined_count=" << combined_blocks_.size();
    emit SelectionChanged(0);
    emit Combined(static_cast<int>(combined_blocks_.size()));
    return true;
}

const std::vector<DivideBlock::ElementInfo>& CombineBlock::GetBlocks() const {
    return blocks_;
}

const std::vector<std::size_t>& CombineBlock::GetSelectedIndexes() const {
    return selected_indexes_;
}

const std::vector<CombineBlock::CombinedBlock>& CombineBlock::GetCombinedBlocks() const {
    return combined_blocks_;
}

const std::string& CombineBlock::GetError() const {
    return error_;
}

void CombineBlock::SelectBlock(int index) {
    if (index < 0 || !Select(static_cast<std::size_t>(index))) {
        if (index < 0) {
            SetError("block index is out of range");
        }
        emit CombineFailed(FromUtf8String(error_));
    }
}

void CombineBlock::DeselectBlock(int index) {
    if (index < 0 || !Deselect(static_cast<std::size_t>(index))) {
        if (index < 0) {
            SetError("block index is not selected");
        }
        emit CombineFailed(FromUtf8String(error_));
    }
}

void CombineBlock::ClearSelectedBlocks() {
    ClearSelection();
}

void CombineBlock::CombineSelectedBlocks() {
    if (!CombineSelected()) {
        emit CombineFailed(FromUtf8String(error_));
    }
}

bool CombineBlock::IsValidIndex(std::size_t index) const {
    return index < blocks_.size();
}

void CombineBlock::SetError(const std::string& error) {
    error_ = error;
}

} // namespace iiHtmlBlock
