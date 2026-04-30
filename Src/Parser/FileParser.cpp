#include "FileParser.h"

#include <QByteArray>
#include <QDebug>
#include <QString>

#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

namespace iiXml::parser {

FileParser::FileParser(QObject* parent)
    : QObject(parent) {
    qDebug() << "iiXml::parser::FileParser::FileParser constructed";
}

std::optional<tag_value> FileParser::parse_file(const std::filesystem::path& file_path) const {
    qDebug() << "iiXml::parser::FileParser::parse_file begin"
             << "path=" << QString::fromStdString(file_path.string());
    try {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            qDebug() << "iiXml::parser::FileParser::parse_file failed"
                     << "reason=file open failed";
            return std::nullopt;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();

        const tag_parser parser;
        const std::optional<tag_value> parsed = parser.parse(buffer.str());
        if (!parsed.has_value()) {
            qDebug() << "iiXml::parser::FileParser::parse_file failed"
                     << "reason=tag parser rejected file content";
            return std::nullopt;
        }

        qDebug() << "iiXml::parser::FileParser::parse_file parsed"
                 << "tag=" << QString::fromStdString(parsed->tag_name)
                 << "value_size=" << parsed->value.size();
        return parsed;
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::parser::FileParser::parse_file exception"
                 << "what=" << exception.what();
        return std::nullopt;
    } catch (...) {
        qDebug() << "iiXml::parser::FileParser::parse_file exception"
                 << "what=unknown";
        return std::nullopt;
    }
}

void FileParser::parseFile(const QString& file_path) {
    qDebug() << "iiXml::parser::FileParser::parseFile begin"
             << "path=" << file_path;
    try {
        const QByteArray utf8 = file_path.toUtf8();
        const std::string path(utf8.constData(), static_cast<std::size_t>(utf8.size()));
        const std::optional<tag_value> parsed = parse_file(std::filesystem::path(path));

        if (!parsed.has_value()) {
            qDebug() << "iiXml::parser::FileParser::parseFile failed"
                     << "reason=file parse failed";
            emit parseFailed("file parse failed");
            return;
        }

        qDebug() << "iiXml::parser::FileParser::parseFile parsed"
                 << "tag=" << QString::fromStdString(parsed->tag_name)
                 << "value_size=" << parsed->value.size();
        emit tagParsed(QString::fromStdString(parsed->tag_name), QString::fromStdString(parsed->value));
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::parser::FileParser::parseFile exception"
                 << "what=" << exception.what();
        emit parseFailed(QString::fromStdString(
            std::string("file parse exception: ") + exception.what()
        ));
    } catch (...) {
        qDebug() << "iiXml::parser::FileParser::parseFile exception"
                 << "what=unknown";
        emit parseFailed("file parse exception: unknown");
    }
}

} // namespace iiXml::parser
