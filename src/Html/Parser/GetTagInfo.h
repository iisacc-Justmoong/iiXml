#pragma once

#include <QObject>
#include <QString>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace iiHtmlBlock {

class GetTagInfo : public QObject {
    Q_OBJECT

public:
    enum class FieldType {
        String,
        Int,
        Float,
        Bool
    };
    Q_ENUM(FieldType)

    struct FieldInfo {
        std::string name;
        std::string value;
        std::size_t name_begin;
        std::size_t name_end;
        bool has_value;
        std::size_t value_begin;
        std::size_t value_end;
        FieldType type;
        bool type_declared;
    };

    struct TagInfo {
        std::string tag_name;
        std::string value;
        std::string raw;
        std::size_t raw_begin;
        std::size_t value_begin;
        std::size_t value_end;
        std::size_t raw_end;
        std::vector<FieldInfo> fields;
        std::vector<TagInfo> children;
    };

    struct RangeInfo {
        std::string tag_name;
        std::string value;
        std::string raw;
        std::size_t raw_begin;
        std::size_t value_begin;
        std::size_t value_end;
        std::size_t raw_end;
        std::vector<FieldInfo> fields;
    };

    explicit GetTagInfo(QObject* parent = nullptr);

    [[nodiscard]] bool Parse(const char* xml);
    [[nodiscard]] bool Parse(std::string_view xml);
    [[nodiscard]] bool Parse(const QString& xml);

    [[nodiscard]] const std::vector<TagInfo>& GetTags() const;
    [[nodiscard]] const std::vector<RangeInfo>& GetRanges() const;
    [[nodiscard]] const TagInfo* GetRoot() const;
    [[nodiscard]] const std::string& GetTagName() const;
    [[nodiscard]] const std::string& GetValue() const;
    [[nodiscard]] const std::string& GetRaw() const;
    [[nodiscard]] const std::vector<FieldInfo>& GetFields() const;
    [[nodiscard]] const std::string& GetError() const;

public slots:
    void ParseXml(const QString& xml);

signals:
    void Parsed(const QString& tag_name, const QString& value);
    void ParseFailed(const QString& reason);

private:
    [[nodiscard]] bool StoreParsedTags(std::string_view xml);
    void Clear();

    std::vector<TagInfo> tags_;
    std::vector<RangeInfo> ranges_;
    std::string tag_name_;
    std::string value_;
    std::string raw_;
    std::vector<FieldInfo> fields_;
    std::string error_;
};

} // namespace iiHtmlBlock
