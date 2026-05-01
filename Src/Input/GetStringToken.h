#pragma once

#include "Src/Parser/TagParser.h"

#include <QObject>
#include <QString>

#include <optional>
#include <string>

namespace iiXml::Writer {

enum class GetStringTokenStatus {
    Parsed,
    InvalidXmlFile,
    InvalidTagClosure,
    ParserRejected,
    ExceptionThrown
};

struct GetStringTokenResult {
    GetStringTokenStatus Status;
    std::optional<iiXml::Parser::TagValue> Token;
    std::string Reason;
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

    [[nodiscard]] GetStringTokenResult ParseString(const QString& Input) const;

public slots:
    void ReadString(const QString& Input);

signals:
    void Parsed(const QString& TagName, const QString& Value);
    void Failed(iiXml::Writer::GetStringToken::Status Status, const QString& Reason);
    void ParseFailed(const QString& Reason);
};

} // namespace iiXml::Writer

Q_DECLARE_METATYPE(iiXml::Writer::GetStringToken::Status)
