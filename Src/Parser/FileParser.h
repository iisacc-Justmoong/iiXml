#pragma once

#include "TagParser.h"

#include <QObject>
#include <QString>

#include <filesystem>
#include <optional>

namespace iiXml::parser {

class FileParser : public QObject {
    Q_OBJECT

public:
    explicit FileParser(QObject* parent = nullptr);

    [[nodiscard]] std::optional<tag_value> parse_file(const std::filesystem::path& file_path) const;

public slots:
    void parseFile(const QString& file_path);

signals:
    void tagParsed(const QString& tag_name, const QString& value);
    void parseFailed(const QString& reason);
};

} // namespace iiXml::parser
