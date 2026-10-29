#pragma once

#include "src/Html/Modifier/DivideBlock.h"

#include <QObject>
#include <QString>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace iiHtmlBlock {

class DeleteBlock : public QObject {
    Q_OBJECT

public:
    explicit DeleteBlock(QObject* parent = nullptr);

    [[nodiscard]] bool Parse(const char* html);
    [[nodiscard]] bool Parse(std::string_view html);
    [[nodiscard]] bool Parse(const QString& html);

    [[nodiscard]] bool Delete(std::size_t index);

    void ClearDeletedBlocks();

    [[nodiscard]] const std::string& GetHTMLText() const;
    [[nodiscard]] const std::vector<DivideBlock::ElementInfo>& GetBlocks() const;
    [[nodiscard]] const std::vector<DivideBlock::ElementInfo>& GetDeletedBlocks() const;
    [[nodiscard]] const std::string& GetError() const;

public slots:
    void ParseHtml(const QString& html);
    void DeleteBlockAt(int index);

signals:
    void Parsed(int block_count);
    void Deleted(int deleted_count, const QString& html);
    void DeleteFailed(const QString& reason);

private:
    [[nodiscard]] bool RefreshBlocks();
    void RefreshHTMLText();
    void Clear();
    void SetError(const std::string& error);

    std::string prefix_;
    std::string body_;
    std::string html_;
    std::vector<DivideBlock::ElementInfo> blocks_;
    std::vector<DivideBlock::ElementInfo> deleted_blocks_;
    std::string error_;
};

} // namespace iiHtmlBlock
