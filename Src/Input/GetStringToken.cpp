#include "GetStringToken.h"

#include "Src/Input/GetFile.h"
#include "Src/Logging/XmlLog.h"
#include "Src/Parser/TagParser.h"

#include <QByteArray>
#include <QDebug>
#include <QMetaType>
#include <QString>

#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace {

std::string to_utf8_string(const QString& value) {
    const QByteArray utf8 = value.toUtf8();
    return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

QString from_utf8_string(const std::string& value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

QString reason_for_status(iiXml::Writer::GetFileStatus status) {
    switch (status) {
        case iiXml::Writer::GetFileStatus::Parsed:
            return "parsed";
        case iiXml::Writer::GetFileStatus::FileReadFailed:
            return "file read failed";
        case iiXml::Writer::GetFileStatus::InvalidXmlFile:
            return "invalid xml file";
        case iiXml::Writer::GetFileStatus::InvalidTagClosure:
            return "invalid tag closure";
        case iiXml::Writer::GetFileStatus::ParserRejected:
            return "parser rejected input";
        case iiXml::Writer::GetFileStatus::ExceptionThrown:
            return "exception thrown";
    }

    return "unknown failure";
}

iiXml::Writer::GetStringTokenStatus to_string_status(iiXml::Writer::GetFileStatus status) {
    switch (status) {
        case iiXml::Writer::GetFileStatus::Parsed:
            return iiXml::Writer::GetStringTokenStatus::Parsed;
        case iiXml::Writer::GetFileStatus::InvalidXmlFile:
            return iiXml::Writer::GetStringTokenStatus::InvalidXmlFile;
        case iiXml::Writer::GetFileStatus::InvalidTagClosure:
            return iiXml::Writer::GetStringTokenStatus::InvalidTagClosure;
        case iiXml::Writer::GetFileStatus::ParserRejected:
            return iiXml::Writer::GetStringTokenStatus::ParserRejected;
        case iiXml::Writer::GetFileStatus::FileReadFailed:
        case iiXml::Writer::GetFileStatus::ExceptionThrown:
            return iiXml::Writer::GetStringTokenStatus::ExceptionThrown;
    }

    return iiXml::Writer::GetStringTokenStatus::ExceptionThrown;
}

iiXml::Writer::GetStringToken::Status to_qt_status(
    iiXml::Writer::GetStringTokenStatus status
) {
    switch (status) {
        case iiXml::Writer::GetStringTokenStatus::Parsed:
            return iiXml::Writer::GetStringToken::Status::Parsed;
        case iiXml::Writer::GetStringTokenStatus::InvalidXmlFile:
            return iiXml::Writer::GetStringToken::Status::InvalidXmlFile;
        case iiXml::Writer::GetStringTokenStatus::InvalidTagClosure:
            return iiXml::Writer::GetStringToken::Status::InvalidTagClosure;
        case iiXml::Writer::GetStringTokenStatus::ParserRejected:
            return iiXml::Writer::GetStringToken::Status::ParserRejected;
        case iiXml::Writer::GetStringTokenStatus::ExceptionThrown:
            return iiXml::Writer::GetStringToken::Status::ExceptionThrown;
    }

    return iiXml::Writer::GetStringToken::Status::ExceptionThrown;
}

const char* status_name(iiXml::Writer::GetStringTokenStatus status) {
    switch (status) {
        case iiXml::Writer::GetStringTokenStatus::Parsed:
            return "parsed";
        case iiXml::Writer::GetStringTokenStatus::InvalidXmlFile:
            return "invalid_xml_file";
        case iiXml::Writer::GetStringTokenStatus::InvalidTagClosure:
            return "invalid_tag_closure";
        case iiXml::Writer::GetStringTokenStatus::ParserRejected:
            return "parser_rejected";
        case iiXml::Writer::GetStringTokenStatus::ExceptionThrown:
            return "exception_thrown";
    }

    return "unknown";
}

iiXml::Writer::GetStringTokenResult make_string_result(
    iiXml::Writer::GetStringTokenStatus status,
    std::optional<iiXml::Parser::TagValue> token,
    std::string reason
) {
    if (reason.empty()) {
        reason = "string token parse failed";
    }

    return iiXml::Writer::GetStringTokenResult{status, std::move(token), std::move(reason)};
}

iiXml::Writer::GetStringTokenResult from_get_file_result(
    const iiXml::Writer::GetFileResult& result
) {
    std::string reason = result.Reason;
    if (reason.empty()) {
        reason = reason_for_status(result.Status).toStdString();
    }

    return make_string_result(to_string_status(result.Status), result.Token, reason);
}

} // namespace

namespace iiXml::Writer {

GetStringToken::GetStringToken(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<GetStringToken::Status>("iiXml::Writer::GetStringToken::Status");
    qDebug() << "iiXml::Writer::GetStringToken::GetStringToken constructed";
}

GetStringTokenResult GetStringToken::ParseString(const QString& input) const {
    qDebug() << "iiXml::Writer::GetStringToken::ParseString begin"
             << "input_size=" << input.size();
    try {
        const std::string bytes = to_utf8_string(input);
        iiXml::Logging::LogInputSummary(
            "iiXml::Writer::GetStringToken::ParseString",
            std::string_view(bytes.data(), bytes.size())
        );
        const GetFile validated_input;
        const GetFileResult result =
            validated_input.ParseXml(std::string_view(bytes.data(), bytes.size()));
        const GetStringTokenResult converted = from_get_file_result(result);
        qDebug() << "iiXml::Writer::GetStringToken::ParseString"
                 << (converted.Status == GetStringTokenStatus::Parsed ? "parsed" : "failed")
                 << "status=" << status_name(converted.Status)
                 << "has_token=" << converted.Token.has_value();
        iiXml::Logging::LogOutputSummary(
            "iiXml::Writer::GetStringToken::ParseString",
            status_name(converted.Status),
            converted.Token.has_value()
                ? std::string("tag=") + converted.Token->TagName
                    + " value_size=" + std::to_string(converted.Token->Value.size())
                : converted.Reason
        );
        return converted;
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Writer::GetStringToken::ParseString exception"
                 << "what=" << exception.what();
        return make_string_result(
            GetStringTokenStatus::ExceptionThrown,
            std::nullopt,
            std::string("QString token input exception: ") + exception.what()
        );
    } catch (...) {
        qDebug() << "iiXml::Writer::GetStringToken::ParseString exception"
                 << "what=unknown";
        return make_string_result(
            GetStringTokenStatus::ExceptionThrown,
            std::nullopt,
            "QString token input exception: unknown"
        );
    }
}

void GetStringToken::ReadString(const QString& input) {
    qDebug() << "iiXml::Writer::GetStringToken::ReadString begin"
             << "input_size=" << input.size();
    try {
        const std::string bytes = to_utf8_string(input);
        iiXml::Logging::LogInputSummary(
            "iiXml::Writer::GetStringToken::ReadString",
            std::string_view(bytes.data(), bytes.size())
        );
        const GetStringTokenResult result = ParseString(input);
        if (result.Status != GetStringTokenStatus::Parsed || !result.Token.has_value()) {
            const QString reason = from_utf8_string(result.Reason);
            qDebug() << "iiXml::Writer::GetStringToken::ReadString failed"
                     << "status=" << status_name(result.Status)
                     << "reason=" << reason;
            iiXml::Logging::LogOutputSummary(
                "iiXml::Writer::GetStringToken::ReadString",
                status_name(result.Status),
                result.Reason
            );
            emit Failed(to_qt_status(result.Status), reason);
            emit ParseFailed(reason);
            return;
        }

        qDebug() << "iiXml::Writer::GetStringToken::ReadString parsed"
                 << "tag=" << from_utf8_string(result.Token->TagName)
                 << "value_size=" << result.Token->Value.size();
        iiXml::Logging::LogOutputSummary(
            "iiXml::Writer::GetStringToken::ReadString",
            "parsed",
            std::string("tag=") + result.Token->TagName
                + " value_size=" + std::to_string(result.Token->Value.size())
        );
        emit Parsed(from_utf8_string(result.Token->TagName), from_utf8_string(result.Token->Value));
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Writer::GetStringToken::ReadString exception"
                 << "what=" << exception.what();
        const QString reason = QString::fromStdString(
            std::string("QString token slot exception: ") + exception.what()
        );
        emit Failed(Status::ExceptionThrown, reason);
        emit ParseFailed(reason);
    } catch (...) {
        qDebug() << "iiXml::Writer::GetStringToken::ReadString exception"
                 << "what=unknown";
        const QString reason = "QString token slot exception: unknown";
        emit Failed(Status::ExceptionThrown, reason);
        emit ParseFailed(reason);
    }
}

} // namespace iiXml::Writer
