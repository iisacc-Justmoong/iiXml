#pragma once

#include <QObject>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iiXml::Elements {

struct OpenTagRange {
    std::string TagName;
    std::size_t RawBegin;
    std::size_t ValueBegin;
    std::size_t ValueEnd;
    std::size_t RawEnd;
};

class OpenTag : public QObject {
    Q_OBJECT

public:
    explicit OpenTag(QObject* parent = nullptr);

    [[nodiscard]] bool CloseOpenTag(
        std::vector<std::string>& OpenTags,
        std::string_view ClosingTagName
    ) const;

    [[nodiscard]] std::optional<std::vector<OpenTagRange>> ParseOpenTags(
        std::string_view Input
    ) const;
};

} // namespace iiXml::Elements

using OpenTag = iiXml::Elements::OpenTag;
