#include "FileParser.h"

#include <QByteArray>
#include <QString>

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

namespace iiXml::parser {

FileParser::FileParser(QObject* parent)
    : QObject(parent) {
}

std::optional<tag_value> FileParser::parse_file(const std::filesystem::path& file_path) const {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();

    const tag_parser parser;
    return parser.parse(buffer.str());
}

void FileParser::parseFile(const QString& file_path) {
    const QByteArray utf8 = file_path.toUtf8();
    const std::string path(utf8.constData(), static_cast<std::size_t>(utf8.size()));
    const std::optional<tag_value> parsed = parse_file(std::filesystem::path(path));

    if (!parsed.has_value()) {
        emit parseFailed("file parse failed");
        return;
    }

    emit tagParsed(QString::fromStdString(parsed->tag_name), QString::fromStdString(parsed->value));
}

} // namespace iiXml::parser
