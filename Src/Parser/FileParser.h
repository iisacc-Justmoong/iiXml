#pragma once

#include "TagParser.h"

#include <QObject>
#include <QString>

#include <filesystem>
#include <optional>

namespace iiXml::Parser {

class FileParser : public QObject {
    Q_OBJECT

public:
    explicit FileParser(QObject* parent = nullptr);

    [[nodiscard]] std::optional<TagValue> ParseFile(const std::filesystem::path& FilePath) const;

public slots:
    void ParseFileInput(const QString& FilePath);

signals:
    void TagParsed(const QString& TagName, const QString& Value);
    void ParseFailed(const QString& Reason);
};

} // namespace iiXml::Parser
