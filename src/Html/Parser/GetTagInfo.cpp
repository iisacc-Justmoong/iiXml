#include "GetTagInfo.h"

#include <iiXml.h>

#include <QByteArray>
#include <QDebug>

#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iiHtmlBlock {

namespace {

GetTagInfo::FieldType ConvertFieldType(iiXml::Elements::InlinePropertyType type) {
    switch (type) {
        case iiXml::Elements::InlinePropertyType::StringType:
            return GetTagInfo::FieldType::String;
        case iiXml::Elements::InlinePropertyType::IntType:
            return GetTagInfo::FieldType::Int;
        case iiXml::Elements::InlinePropertyType::FloatType:
            return GetTagInfo::FieldType::Float;
        case iiXml::Elements::InlinePropertyType::BoolType:
            return GetTagInfo::FieldType::Bool;
    }

    return GetTagInfo::FieldType::String;
}

std::string Slice(std::string_view source, std::size_t begin, std::size_t end) {
    if (begin > end || end > source.size()) {
        return {};
    }

    return std::string(source.substr(begin, end - begin));
}

bool IsAsciiSpace(char value) {
    return value == ' ' || value == '\n' || value == '\r' || value == '\t';
}

std::string_view TrimLeadingAscii(std::string_view source) {
    std::size_t begin = 0;
    while (begin < source.size() && IsAsciiSpace(source[begin])) {
        ++begin;
    }

    return source.substr(begin);
}

std::string NormalizeParserInput(std::string_view xml) {
    xml = TrimLeadingAscii(xml);

    bool consumed = true;
    while (consumed) {
        consumed = false;

        if (xml.rfind("<?xml", 0) == 0) {
            const std::size_t end = xml.find("?>");
            if (end != std::string_view::npos) {
                xml = TrimLeadingAscii(xml.substr(end + 2));
                consumed = true;
            }
        }

        if (xml.rfind("<!DOCTYPE", 0) == 0 || xml.rfind("<!doctype", 0) == 0) {
            const std::size_t end = xml.find('>');
            if (end != std::string_view::npos) {
                xml = TrimLeadingAscii(xml.substr(end + 1));
                consumed = true;
            }
        }
    }

    return std::string(xml);
}

std::vector<GetTagInfo::FieldInfo> ConvertFields(
    std::string_view xml,
    const std::vector<iiXml::Parser::TagField>& fields
) {
    std::vector<GetTagInfo::FieldInfo> converted;
    converted.reserve(fields.size());

    for (const iiXml::Parser::TagField& field : fields) {
        converted.push_back(GetTagInfo::FieldInfo{
            field.Name,
            field.HasValue ? Slice(xml, field.ValueBegin, field.ValueEnd) : std::string{},
            field.NameBegin,
            field.NameEnd,
            field.HasValue,
            field.ValueBegin,
            field.ValueEnd,
            ConvertFieldType(field.ValueType),
            field.TypeDeclared
        });
    }

    return converted;
}

GetTagInfo::TagInfo ConvertNode(std::string_view xml, const iiXml::Parser::TagNode& node) {
    GetTagInfo::TagInfo converted{
        node.Range.TagName,
        Slice(xml, node.Range.ValueBegin, node.Range.ValueEnd),
        Slice(xml, node.Range.RawBegin, node.Range.RawEnd),
        node.Range.RawBegin,
        node.Range.ValueBegin,
        node.Range.ValueEnd,
        node.Range.RawEnd,
        ConvertFields(xml, node.Fields),
        {}
    };

    converted.children.reserve(node.Children.size());
    for (const iiXml::Parser::TagNode& child : node.Children) {
        converted.children.push_back(ConvertNode(xml, child));
    }

    return converted;
}

GetTagInfo::RangeInfo ConvertRange(std::string_view xml, const iiXml::Parser::TagNode& node) {
    return GetTagInfo::RangeInfo{
        node.Range.TagName,
        Slice(xml, node.Range.ValueBegin, node.Range.ValueEnd),
        Slice(xml, node.Range.RawBegin, node.Range.RawEnd),
        node.Range.RawBegin,
        node.Range.ValueBegin,
        node.Range.ValueEnd,
        node.Range.RawEnd,
        ConvertFields(xml, node.Fields)
    };
}

void AppendRanges(
    std::string_view xml,
    const iiXml::Parser::TagNode& node,
    std::vector<GetTagInfo::RangeInfo>& ranges
) {
    ranges.push_back(ConvertRange(xml, node));
    for (const iiXml::Parser::TagNode& child : node.Children) {
        AppendRanges(xml, child, ranges);
    }
}

std::string ToUtf8String(const QString& value) {
    const QByteArray utf8 = value.toUtf8();
    return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

QString FromUtf8String(const std::string& value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

} // namespace

GetTagInfo::GetTagInfo(QObject* parent)
    : QObject(parent) {
    qDebug() << "GetTagInfo::GetTagInfo constructed";
}

bool GetTagInfo::Parse(const char* xml) {
    if (xml == nullptr) {
        Clear();
        error_ = "xml input is null";
        qDebug() << "GetTagInfo::Parse failed"
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    return Parse(std::string_view(xml));
}

bool GetTagInfo::Parse(std::string_view xml) {
    qDebug() << "GetTagInfo::Parse begin"
             << "input_size=" << xml.size();
    try {
        Clear();

        const std::string normalized = NormalizeParserInput(xml);
        if (!StoreParsedTags(normalized)) {
            qDebug() << "GetTagInfo::Parse failed"
                     << "reason=" << QString::fromStdString(error_);
            return false;
        }

        qDebug() << "GetTagInfo::Parse parsed"
                 << "tag=" << QString::fromStdString(tag_name_)
                 << "value_size=" << value_.size()
                 << "field_count=" << fields_.size();
        return true;
    } catch (const std::exception& exception) {
        Clear();
        error_ = std::string("tag info parse exception: ") + exception.what();
        qDebug() << "GetTagInfo::Parse exception"
                 << "what=" << exception.what();
        return false;
    } catch (...) {
        Clear();
        error_ = "tag info parse exception: unknown";
        qDebug() << "GetTagInfo::Parse exception"
                 << "what=unknown";
        return false;
    }
}

bool GetTagInfo::Parse(const QString& xml) {
    const std::string bytes = ToUtf8String(xml);
    return Parse(std::string_view(bytes.data(), bytes.size()));
}

const std::vector<GetTagInfo::TagInfo>& GetTagInfo::GetTags() const {
    return tags_;
}

const std::vector<GetTagInfo::RangeInfo>& GetTagInfo::GetRanges() const {
    return ranges_;
}

const GetTagInfo::TagInfo* GetTagInfo::GetRoot() const {
    if (tags_.empty()) {
        return nullptr;
    }

    return &tags_.front();
}

const std::string& GetTagInfo::GetTagName() const {
    return tag_name_;
}

const std::string& GetTagInfo::GetValue() const {
    return value_;
}

const std::string& GetTagInfo::GetRaw() const {
    return raw_;
}

const std::vector<GetTagInfo::FieldInfo>& GetTagInfo::GetFields() const {
    return fields_;
}

const std::string& GetTagInfo::GetError() const {
    return error_;
}

void GetTagInfo::ParseXml(const QString& xml) {
    if (Parse(xml)) {
        emit Parsed(FromUtf8String(tag_name_), FromUtf8String(value_));
        return;
    }

    emit ParseFailed(FromUtf8String(error_));
}

bool GetTagInfo::StoreParsedTags(std::string_view xml) {
    const iiXml::Parser::TagParser parser;
    const std::optional<std::vector<iiXml::Parser::TagNode>> parsed = parser.ParseAll(xml);
    if (!parsed.has_value()) {
        error_ = "iiXml tag parser rejected input";
        return false;
    }

    if (parsed->empty()) {
        error_ = "iiXml tag parser returned no tag";
        return false;
    }

    tags_.reserve(parsed->size());
    for (const iiXml::Parser::TagNode& node : *parsed) {
        tags_.push_back(ConvertNode(xml, node));
        AppendRanges(xml, node, ranges_);
    }

    const TagInfo& root = tags_.front();
    tag_name_ = root.tag_name;
    value_ = root.value;
    raw_ = root.raw;
    fields_ = root.fields;
    error_.clear();
    return true;
}

void GetTagInfo::Clear() {
    tags_.clear();
    ranges_.clear();
    tag_name_.clear();
    value_.clear();
    raw_.clear();
    fields_.clear();
    error_.clear();
}

} // namespace iiHtmlBlock
