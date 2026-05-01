#include "FileParser.h"

#include "Src/Logging/XmlLog.h"

#include <QByteArray>
#include <QDebug>
#include <QString>

#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

namespace iiXml::Parser {

FileParser::FileParser(QObject* parent)
    : QObject(parent) {
    qDebug() << "iiXml::Parser::FileParser::FileParser constructed";
}

std::optional<TagValue> FileParser::ParseFile(const std::filesystem::path& file_path) const {
    qDebug() << "iiXml::Parser::FileParser::ParseFile begin"
             << "path=" << QString::fromStdString(file_path.string());
    try {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            qDebug() << "iiXml::Parser::FileParser::ParseFile failed"
                     << "reason=file open failed";
            return std::nullopt;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        const std::string content = buffer.str();
        iiXml::Logging::LogInputSummary(
            "iiXml::Parser::FileParser::ParseFile",
            content
        );

        const TagParser parser;
        const std::optional<TagValue> parsed = parser.Parse(content);
        if (!parsed.has_value()) {
            qDebug() << "iiXml::Parser::FileParser::ParseFile failed"
                     << "reason=tag parser rejected file content";
            return std::nullopt;
        }

        qDebug() << "iiXml::Parser::FileParser::ParseFile parsed"
                 << "tag=" << QString::fromStdString(parsed->TagName)
                 << "value_size=" << parsed->Value.size();
        iiXml::Logging::LogOutputSummary(
            "iiXml::Parser::FileParser::ParseFile",
            "parsed",
            std::string("tag=") + parsed->TagName
                + " value_size=" + std::to_string(parsed->Value.size())
        );
        return parsed;
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Parser::FileParser::ParseFile exception"
                 << "what=" << exception.what();
        return std::nullopt;
    } catch (...) {
        qDebug() << "iiXml::Parser::FileParser::ParseFile exception"
                 << "what=unknown";
        return std::nullopt;
    }
}

void FileParser::ParseFileInput(const QString& file_path) {
    qDebug() << "iiXml::Parser::FileParser::ParseFileInput begin"
             << "path=" << file_path;
    try {
        const QByteArray utf8 = file_path.toUtf8();
        const std::string path(utf8.constData(), static_cast<std::size_t>(utf8.size()));
        const std::optional<TagValue> parsed = ParseFile(std::filesystem::path(path));

        if (!parsed.has_value()) {
            qDebug() << "iiXml::Parser::FileParser::ParseFileInput failed"
                     << "reason=file parse failed";
            emit ParseFailed("file parse failed");
            return;
        }

        qDebug() << "iiXml::Parser::FileParser::ParseFileInput parsed"
                 << "tag=" << QString::fromStdString(parsed->TagName)
                 << "value_size=" << parsed->Value.size();
        iiXml::Logging::LogOutputSummary(
            "iiXml::Parser::FileParser::ParseFileInput",
            "parsed",
            std::string("tag=") + parsed->TagName
                + " value_size=" + std::to_string(parsed->Value.size())
        );
        emit TagParsed(QString::fromStdString(parsed->TagName), QString::fromStdString(parsed->Value));
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Parser::FileParser::ParseFileInput exception"
                 << "what=" << exception.what();
        emit ParseFailed(QString::fromStdString(
            std::string("file parse exception: ") + exception.what()
        ));
    } catch (...) {
        qDebug() << "iiXml::Parser::FileParser::ParseFileInput exception"
                 << "what=unknown";
        emit ParseFailed("file parse exception: unknown");
    }
}

} // namespace iiXml::Parser
