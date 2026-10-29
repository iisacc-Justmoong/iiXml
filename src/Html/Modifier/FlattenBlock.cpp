#include "FlattenBlock.h"

#include <QByteArray>
#include <QDebug>
#include <QString>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <limits>
#include <string>
#include <string_view>

namespace iiHtmlBlock {

namespace {

std::string ToUtf8String(const QString& value) {
    const QByteArray utf8 = value.toUtf8();
    return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

QString FromUtf8String(const std::string& value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

FlattenBlock::LayerBlock BuildLayerBlock(const std::vector<DivideBlock::ElementInfo>& blocks) {
    FlattenBlock::LayerBlock layer{
        {},
        blocks,
        "layer",
        {},
        {},
        std::numeric_limits<std::size_t>::max(),
        std::numeric_limits<std::size_t>::max(),
        0,
        0,
        true
    };

    layer.source_indexes.reserve(blocks.size());

    for (std::size_t index = 0; index < blocks.size(); ++index) {
        const DivideBlock::ElementInfo& block = blocks[index];
        layer.source_indexes.push_back(index);
        layer.value += block.value;
        layer.raw += block.raw;
        layer.raw_begin = std::min(layer.raw_begin, block.raw_begin);
        layer.value_begin = std::min(layer.value_begin, block.value_begin);
        layer.value_end = std::max(layer.value_end, block.value_end);
        layer.raw_end = std::max(layer.raw_end, block.raw_end);
    }

    if (blocks.empty()) {
        layer.raw_begin = 0;
        layer.value_begin = 0;
    }

    return layer;
}

} // namespace

FlattenBlock::FlattenBlock(QObject* parent)
    : QObject(parent)
    , has_layer_(false) {
    qDebug() << "FlattenBlock::FlattenBlock constructed";
}

bool FlattenBlock::Parse(const char* html) {
    if (html == nullptr) {
        Clear();
        error_ = "html input is null";
        qDebug() << "FlattenBlock::Parse failed"
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    return Parse(std::string_view(html));
}

bool FlattenBlock::Parse(std::string_view html) {
    qDebug() << "FlattenBlock::Parse begin"
             << "input_size=" << html.size();
    try {
        Clear();

        DivideBlock divider;
        if (!divider.Parse(html)) {
            error_ = divider.GetError();
            if (error_.empty()) {
                error_ = "block layer parse failed";
            }
            qDebug() << "FlattenBlock::Parse failed"
                     << "reason=" << QString::fromStdString(error_);
            return false;
        }

        blocks_ = divider.GetBlockElements();
        emit BlocksChanged(static_cast<int>(blocks_.size()));
        return LayerAll();
    } catch (const std::exception& exception) {
        Clear();
        error_ = std::string("block layer exception: ") + exception.what();
        qDebug() << "FlattenBlock::Parse exception"
                 << "what=" << exception.what();
        return false;
    } catch (...) {
        Clear();
        error_ = "block layer exception: unknown";
        qDebug() << "FlattenBlock::Parse exception"
                 << "what=unknown";
        return false;
    }
}

bool FlattenBlock::Parse(const QString& html) {
    const std::string bytes = ToUtf8String(html);
    return Parse(std::string_view(bytes.data(), bytes.size()));
}

void FlattenBlock::SetBlocks(const std::vector<DivideBlock::ElementInfo>& blocks) {
    blocks_ = blocks;
    ClearLayer();
    error_.clear();

    qDebug() << "FlattenBlock::SetBlocks"
             << "block_count=" << blocks_.size();
    emit BlocksChanged(static_cast<int>(blocks_.size()));
}

bool FlattenBlock::LayerAll() {
    if (blocks_.empty()) {
        ClearLayer();
        SetError("at least one block is required");
        qDebug() << "FlattenBlock::LayerAll failed"
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    layer_block_ = BuildLayerBlock(blocks_);
    has_layer_ = true;
    error_.clear();

    qDebug() << "FlattenBlock::LayerAll layered"
             << "block_count=" << blocks_.size();
    emit Layered(static_cast<int>(blocks_.size()));
    return true;
}

void FlattenBlock::ClearLayer() {
    layer_block_ = LayerBlock{};
    has_layer_ = false;
}

const std::vector<DivideBlock::ElementInfo>& FlattenBlock::GetBlocks() const {
    return blocks_;
}

const FlattenBlock::LayerBlock* FlattenBlock::GetLayerBlock() const {
    if (!has_layer_) {
        return nullptr;
    }

    return &layer_block_;
}

bool FlattenBlock::HasLayer() const {
    return has_layer_;
}

const std::string& FlattenBlock::GetError() const {
    return error_;
}

void FlattenBlock::ParseHtml(const QString& html) {
    if (!Parse(html)) {
        emit LayerFailed(FromUtf8String(error_));
    }
}

void FlattenBlock::LayerBlocks() {
    if (!LayerAll()) {
        emit LayerFailed(FromUtf8String(error_));
    }
}

void FlattenBlock::Clear() {
    blocks_.clear();
    ClearLayer();
    error_.clear();
}

void FlattenBlock::SetError(const std::string& error) {
    error_ = error;
}

} // namespace iiHtmlBlock
