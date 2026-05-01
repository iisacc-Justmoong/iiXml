#pragma once

#include "Src/Parser/TagParser.h"

#include <QObject>
#include <QString>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace iiXml::Writer {

enum class GetFileStatus {
    Parsed,
    FileReadFailed,
    InvalidXmlFile,
    InvalidTagClosure,
    ParserRejected,
    ExceptionThrown
};

struct GetFileResult {
    GetFileStatus Status;
    std::optional<iiXml::Parser::TagValue> Token;
    std::string Reason;
};

class GetFile : public QObject {
    Q_OBJECT

public:
    enum class Status {
        Parsed,
        FileReadFailed,
        InvalidXmlFile,
        InvalidTagClosure,
        ParserRejected,
        ExceptionThrown
    };
    Q_ENUM(Status)

    explicit GetFile(QObject* parent = nullptr);

    [[nodiscard]] GetFileResult ParseFile(const std::filesystem::path& FilePath) const;
    [[nodiscard]] GetFileResult ParseXml(std::string_view Input) const;

public slots:
    void ReadFile(const QString& FilePath);
    void ReadXml(const QString& Input);

signals:
    void Parsed(const QString& TagName, const QString& Value);
    void Failed(iiXml::Writer::GetFile::Status Status, const QString& Reason);
    void FileReadFailed();
    void InvalidXmlFile();
    void InvalidTagClosure();
    void ParserRejected();
    void ExceptionThrown();
};

} // namespace iiXml::Writer

Q_DECLARE_METATYPE(iiXml::Writer::GetFile::Status)
