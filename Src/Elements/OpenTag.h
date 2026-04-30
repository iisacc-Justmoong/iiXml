#pragma once

#include <QObject>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iiXml::elements {

struct open_tag_value {
    std::string tag_name;
    std::string value;
    std::string raw;
};

class OpenTag : public QObject {
    Q_OBJECT

public:
    explicit OpenTag(QObject* parent = nullptr);

    [[nodiscard]] bool close_open_tag(
        std::vector<std::string>& open_tags,
        std::string_view closing_tag_name
    ) const;

    [[nodiscard]] std::optional<std::vector<open_tag_value>> parse_open_tags(
        std::string_view input
    ) const;
};

} // namespace iiXml::elements

using OpenTag = iiXml::elements::OpenTag;
