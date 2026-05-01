#pragma once

#include <QObject>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iiXml::elements {

enum class inline_property_type {
    string_type,
    int_type,
    float_type,
    bool_type
};

struct inline_property {
    std::string name;
    std::size_t name_begin;
    std::size_t name_end;
    bool has_value;
    std::size_t value_begin;
    std::size_t value_end;
    inline_property_type value_type;
    bool type_declared;
};

class InlineProperties : public QObject {
    Q_OBJECT

public:
    explicit InlineProperties(QObject* parent = nullptr);

    [[nodiscard]] std::optional<std::vector<inline_property>> parse(
        std::string_view opening_tag,
        std::size_t source_offset = 0
    ) const;
};

} // namespace iiXml::elements

using InlineProperties = iiXml::elements::InlineProperties;
