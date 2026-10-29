#pragma once

#include "src/Html/Parser/GetTagInfo.h"

#include <QObject>
#include <QString>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace iiHtmlBlock {

class DivideBlock : public QObject {
    Q_OBJECT

public:
    struct ElementInfo {
        std::string tag_name;
        std::string value;
        std::string raw;
        std::size_t raw_begin;
        std::size_t value_begin;
        std::size_t value_end;
        std::size_t raw_end;
        bool is_block;
        bool has_display_override;
        std::string display_value;
    };

    explicit DivideBlock(QObject* parent = nullptr);

    [[nodiscard]] bool Parse(const char* html);
    [[nodiscard]] bool Parse(std::string_view html);
    [[nodiscard]] bool Parse(const QString& html);

    [[nodiscard]] const std::vector<ElementInfo>& GetElements() const;
    [[nodiscard]] const std::vector<ElementInfo>& GetBlockElements() const;
    [[nodiscard]] const std::string& GetError() const;

    [[nodiscard]] static bool IsBlockTag(std::string_view tag_name);
    [[nodiscard]] static bool IsBlockElement(const GetTagInfo::RangeInfo& element);

public slots:
    void ParseHtml(const QString& html);

signals:
    void Parsed(int element_count, int block_count);
    void ParseFailed(const QString& reason);

private:
    void Clear();

    std::vector<ElementInfo> elements_;
    std::vector<ElementInfo> block_elements_;
    std::string error_;
};

} // namespace iiHtmlBlock
