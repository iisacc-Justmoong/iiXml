#include "DeleteBlock.h"

#include <QByteArray>
#include <QDebug>
#include <QString>

#include <cstddef>
#include <exception>
#include <string>
#include <string_view>
#include <utility>

namespace iiHtmlBlock {

namespace {

struct ParserInputParts {
    std::string prefix;
    std::string body;
};

bool IsAsciiSpace(char value) {
    return value == ' ' || value == '\n' || value == '\r' || value == '\t';
}

std::size_t SkipLeadingAscii(std::string_view source, std::size_t offset) {
    while (offset < source.size() && IsAsciiSpace(source[offset])) {
        ++offset;
    }

    return offset;
}

ParserInputParts SplitParserInput(std::string_view html) {
    std::size_t body_begin = SkipLeadingAscii(html, 0);

    bool consumed = true;
    while (consumed) {
        consumed = false;

        if (html.substr(body_begin).rfind("<?xml", 0) == 0) {
            const std::size_t end = html.find("?>", body_begin);
            if (end != std::string_view::npos) {
                body_begin = SkipLeadingAscii(html, end + 2);
                consumed = true;
            }
        }

        if (html.substr(body_begin).rfind("<!DOCTYPE", 0) == 0
            || html.substr(body_begin).rfind("<!doctype", 0) == 0) {
            const std::size_t end = html.find('>', body_begin);
            if (end != std::string_view::npos) {
                body_begin = SkipLeadingAscii(html, end + 1);
                consumed = true;
            }
        }
    }

    return ParserInputParts{
        std::string(html.substr(0, body_begin)),
        std::string(html.substr(body_begin))
    };
}

std::string ToUtf8String(const QString& value) {
    const QByteArray utf8 = value.toUtf8();
    return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

QString FromUtf8String(const std::string& value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

} // namespace

DeleteBlock::DeleteBlock(QObject* parent)
    : QObject(parent) {
    qDebug() << "DeleteBlock::DeleteBlock constructed";
}

bool DeleteBlock::Parse(const char* html) {
    if (html == nullptr) {
        Clear();
        error_ = "html input is null";
        qDebug() << "DeleteBlock::Parse failed"
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    return Parse(std::string_view(html));
}

bool DeleteBlock::Parse(std::string_view html) {
    qDebug() << "DeleteBlock::Parse begin"
             << "input_size=" << html.size();
    try {
        Clear();

        ParserInputParts parts = SplitParserInput(html);
        prefix_ = std::move(parts.prefix);
        body_ = std::move(parts.body);
        RefreshHTMLText();

        if (!RefreshBlocks()) {
            prefix_.clear();
            body_.clear();
            html_.clear();
            qDebug() << "DeleteBlock::Parse failed"
                     << "reason=" << QString::fromStdString(error_);
            return false;
        }

        qDebug() << "DeleteBlock::Parse parsed"
                 << "block_count=" << blocks_.size();
        return true;
    } catch (const std::exception& exception) {
        Clear();
        error_ = std::string("block deletion parse exception: ") + exception.what();
        qDebug() << "DeleteBlock::Parse exception"
                 << "what=" << exception.what();
        return false;
    } catch (...) {
        Clear();
        error_ = "block deletion parse exception: unknown";
        qDebug() << "DeleteBlock::Parse exception"
                 << "what=unknown";
        return false;
    }
}

bool DeleteBlock::Parse(const QString& html) {
    const std::string bytes = ToUtf8String(html);
    return Parse(std::string_view(bytes.data(), bytes.size()));
}

bool DeleteBlock::Delete(std::size_t index) {
    if (index >= blocks_.size()) {
        SetError("block index is out of range");
        qDebug() << "DeleteBlock::Delete failed"
                 << "index=" << index
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    const DivideBlock::ElementInfo deleted = blocks_[index];
    if (deleted.raw_begin > deleted.raw_end || deleted.raw_end > body_.size()) {
        SetError("block range is out of source range");
        qDebug() << "DeleteBlock::Delete failed"
                 << "index=" << index
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    const std::string previous_body = body_;
    const std::string previous_html = html_;
    const std::vector<DivideBlock::ElementInfo> previous_blocks = blocks_;
    const std::vector<DivideBlock::ElementInfo> previous_deleted_blocks = deleted_blocks_;

    body_.erase(deleted.raw_begin, deleted.raw_end - deleted.raw_begin);
    RefreshHTMLText();
    deleted_blocks_.push_back(deleted);

    if (!RefreshBlocks()) {
        const std::string refresh_error = error_;
        body_ = previous_body;
        html_ = previous_html;
        blocks_ = previous_blocks;
        deleted_blocks_ = previous_deleted_blocks;
        SetError(std::string("deleted html parse failed: ") + refresh_error);
        qDebug() << "DeleteBlock::Delete rollback"
                 << "index=" << index
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    error_.clear();
    qDebug() << "DeleteBlock::Delete"
             << "deleted_count=" << deleted_blocks_.size()
             << "block_count=" << blocks_.size();
    emit Deleted(static_cast<int>(deleted_blocks_.size()), FromUtf8String(html_));
    return true;
}

void DeleteBlock::ClearDeletedBlocks() {
    deleted_blocks_.clear();
    error_.clear();
    emit Deleted(0, FromUtf8String(html_));
}

const std::string& DeleteBlock::GetHTMLText() const {
    return html_;
}

const std::vector<DivideBlock::ElementInfo>& DeleteBlock::GetBlocks() const {
    return blocks_;
}

const std::vector<DivideBlock::ElementInfo>& DeleteBlock::GetDeletedBlocks() const {
    return deleted_blocks_;
}

const std::string& DeleteBlock::GetError() const {
    return error_;
}

void DeleteBlock::ParseHtml(const QString& html) {
    if (Parse(html)) {
        emit Parsed(static_cast<int>(blocks_.size()));
        return;
    }

    emit DeleteFailed(FromUtf8String(error_));
}

void DeleteBlock::DeleteBlockAt(int index) {
    if (index < 0 || !Delete(static_cast<std::size_t>(index))) {
        if (index < 0) {
            SetError("block index is out of range");
        }
        emit DeleteFailed(FromUtf8String(error_));
    }
}

bool DeleteBlock::RefreshBlocks() {
    DivideBlock divider;
    if (!divider.Parse(body_)) {
        blocks_.clear();
        error_ = divider.GetError();
        if (error_.empty()) {
            error_ = "block deletion parse failed";
        }
        return false;
    }

    blocks_ = divider.GetBlockElements();
    error_.clear();
    return true;
}

void DeleteBlock::RefreshHTMLText() {
    html_ = prefix_ + body_;
}

void DeleteBlock::Clear() {
    prefix_.clear();
    body_.clear();
    html_.clear();
    blocks_.clear();
    deleted_blocks_.clear();
    error_.clear();
}

void DeleteBlock::SetError(const std::string& error) {
    error_ = error;
}

} // namespace iiHtmlBlock
