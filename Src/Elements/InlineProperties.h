#pragma once

#include <QObject>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iiXml::Elements {

enum class InlinePropertyType {
    StringType,
    IntType,
    FloatType,
    BoolType
};

struct InlineProperty {
    std::string Name;
    std::size_t NameBegin;
    std::size_t NameEnd;
    bool HasValue;
    std::size_t ValueBegin;
    std::size_t ValueEnd;
    InlinePropertyType ValueType;
    bool TypeDeclared;
};

class InlineProperties : public QObject {
    Q_OBJECT

public:
    explicit InlineProperties(QObject* parent = nullptr);

    [[nodiscard]] std::optional<std::vector<InlineProperty>> Parse(
        std::string_view OpeningTag,
        std::size_t SourceOffset = 0
    ) const;
};

} // namespace iiXml::Elements

using InlineProperties = iiXml::Elements::InlineProperties;
