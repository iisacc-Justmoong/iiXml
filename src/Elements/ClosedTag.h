#pragma once

#include <QObject>
#include <QString>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace iiXml::Elements {

struct ClosedTagMatch {
    std::string TagName;
    std::string Raw;
    std::size_t RawBegin;
    std::size_t RawEnd;
    bool Flag;
};

class ClosedTag : public QObject {
    Q_OBJECT

public:
    explicit ClosedTag(QObject* parent = nullptr);

    [[nodiscard]] bool IsImmediateClosedTag(std::string_view Input) const;
    [[nodiscard]] std::optional<ClosedTagMatch> MatchImmediate(
        std::string_view Input,
        std::size_t SourceOffset = 0
    ) const;

public slots:
    void ParseClosedTag(const QString& Input);

signals:
    void ClosedTagFlagged(bool Flag);
    void ClosedTagParsed(const QString& TagName, const QString& Raw);
    void ClosedTagRejected(const QString& Reason);
};

} // namespace iiXml::Elements

using ClosedTag = iiXml::Elements::ClosedTag;
