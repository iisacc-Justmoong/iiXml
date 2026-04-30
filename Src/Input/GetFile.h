#pragma once

#include "Src/Parser/TagParser.h"

#include <QObject>
#include <QString>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace iiXml::writer {

enum class get_file_status {
    parsed,
    file_read_failed,
    invalid_xml_file,
    invalid_tag_closure,
    parser_rejected,
    exception_thrown
};

struct get_file_result {
    get_file_status status;
    std::optional<iiXml::parser::tag_value> token;
    std::string reason;
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

    [[nodiscard]] get_file_result parse_file(const std::filesystem::path& file_path) const;
    [[nodiscard]] get_file_result parse_xml(std::string_view input) const;

public slots:
    void readFile(const QString& file_path);
    void readXml(const QString& input);

signals:
    void parsed(const QString& tag_name, const QString& value);
    void failed(iiXml::writer::GetFile::Status status, const QString& reason);
    void fileReadFailed();
    void invalidXmlFile();
    void invalidTagClosure();
    void parserRejected();
    void exceptionThrown();
};

} // namespace iiXml::writer

Q_DECLARE_METATYPE(iiXml::writer::GetFile::Status)
