#pragma once

#include <QObject>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iiXml::elements {

struct open_tag_range {
    std::string tag_name;
    std::size_t raw_begin;
    std::size_t value_begin;
    std::size_t value_end;
    std::size_t raw_end;
};

class OpenTag : public QObject {
    Q_OBJECT

public:
    explicit OpenTag(QObject* parent = nullptr);

    [[nodiscard]] bool close_open_tag(
        std::vector<std::string>& open_tags,
        std::string_view closing_tag_name
    ) const;

    [[nodiscard]] std::optional<std::vector<open_tag_range>> parse_open_tags(
        std::string_view input
    ) const;
};

} // namespace iiXml::elements

using OpenTag = iiXml::elements::OpenTag;
