#pragma once

#include "Src/Parser/TagParser.h"

#include <QObject>
#include <QString>

#include <optional>
#include <string>

namespace iiXml::writer {

enum class get_string_token_status {
    parsed,
    invalid_xml_file,
    invalid_tag_closure,
    parser_rejected,
    exception_thrown
};

struct get_string_token_result {
    get_string_token_status status;
    std::optional<iiXml::parser::tag_value> token;
    std::string reason;
};

class GetStringToken : public QObject {
    Q_OBJECT

public:
    enum class Status {
        Parsed,
        InvalidXmlFile,
        InvalidTagClosure,
        ParserRejected,
        ExceptionThrown
    };
    Q_ENUM(Status)

    explicit GetStringToken(QObject* parent = nullptr);

    [[nodiscard]] get_string_token_result parse_string(const QString& input) const;

public slots:
    void readString(const QString& input);

signals:
    void parsed(const QString& tag_name, const QString& value);
    void failed(iiXml::writer::GetStringToken::Status status, const QString& reason);
    void parseFailed(const QString& reason);
};

} // namespace iiXml::writer

Q_DECLARE_METATYPE(iiXml::writer::GetStringToken::Status)
