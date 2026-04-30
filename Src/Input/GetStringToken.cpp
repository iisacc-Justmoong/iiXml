#include "GetStringToken.h"

#include "Src/Input/GetFile.h"
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

QString reason_for_status(iiXml::writer::get_file_status status) {
    switch (status) {
        case iiXml::writer::get_file_status::parsed:
            return "parsed";
        case iiXml::writer::get_file_status::file_read_failed:
            return "file read failed";
        case iiXml::writer::get_file_status::invalid_xml_file:
            return "invalid xml file";
        case iiXml::writer::get_file_status::invalid_tag_closure:
            return "invalid tag closure";
        case iiXml::writer::get_file_status::parser_rejected:
            return "parser rejected input";
        case iiXml::writer::get_file_status::exception_thrown:
            return "exception thrown";
    }

    return "unknown failure";
}

iiXml::writer::get_string_token_status to_string_status(iiXml::writer::get_file_status status) {
    switch (status) {
        case iiXml::writer::get_file_status::parsed:
            return iiXml::writer::get_string_token_status::parsed;
        case iiXml::writer::get_file_status::invalid_xml_file:
            return iiXml::writer::get_string_token_status::invalid_xml_file;
        case iiXml::writer::get_file_status::invalid_tag_closure:
            return iiXml::writer::get_string_token_status::invalid_tag_closure;
        case iiXml::writer::get_file_status::parser_rejected:
            return iiXml::writer::get_string_token_status::parser_rejected;
        case iiXml::writer::get_file_status::file_read_failed:
        case iiXml::writer::get_file_status::exception_thrown:
            return iiXml::writer::get_string_token_status::exception_thrown;
    }

    return iiXml::writer::get_string_token_status::exception_thrown;
}

iiXml::writer::GetStringToken::Status to_qt_status(
    iiXml::writer::get_string_token_status status
) {
    switch (status) {
        case iiXml::writer::get_string_token_status::parsed:
            return iiXml::writer::GetStringToken::Status::Parsed;
        case iiXml::writer::get_string_token_status::invalid_xml_file:
            return iiXml::writer::GetStringToken::Status::InvalidXmlFile;
        case iiXml::writer::get_string_token_status::invalid_tag_closure:
            return iiXml::writer::GetStringToken::Status::InvalidTagClosure;
        case iiXml::writer::get_string_token_status::parser_rejected:
            return iiXml::writer::GetStringToken::Status::ParserRejected;
        case iiXml::writer::get_string_token_status::exception_thrown:
            return iiXml::writer::GetStringToken::Status::ExceptionThrown;
    }

    return iiXml::writer::GetStringToken::Status::ExceptionThrown;
}

const char* status_name(iiXml::writer::get_string_token_status status) {
    switch (status) {
        case iiXml::writer::get_string_token_status::parsed:
            return "parsed";
        case iiXml::writer::get_string_token_status::invalid_xml_file:
            return "invalid_xml_file";
        case iiXml::writer::get_string_token_status::invalid_tag_closure:
            return "invalid_tag_closure";
        case iiXml::writer::get_string_token_status::parser_rejected:
            return "parser_rejected";
        case iiXml::writer::get_string_token_status::exception_thrown:
            return "exception_thrown";
    }

    return "unknown";
}

iiXml::writer::get_string_token_result make_string_result(
    iiXml::writer::get_string_token_status status,
    std::optional<iiXml::parser::tag_value> token,
    std::string reason
) {
    if (reason.empty()) {
        reason = "string token parse failed";
    }

    return iiXml::writer::get_string_token_result{status, std::move(token), std::move(reason)};
}

iiXml::writer::get_string_token_result from_get_file_result(
    const iiXml::writer::get_file_result& result
) {
    std::string reason = result.reason;
    if (reason.empty()) {
        reason = reason_for_status(result.status).toStdString();
    }

    return make_string_result(to_string_status(result.status), result.token, reason);
}

} // namespace

namespace iiXml::writer {

GetStringToken::GetStringToken(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<GetStringToken::Status>("iiXml::writer::GetStringToken::Status");
    qDebug() << "iiXml::writer::GetStringToken::GetStringToken constructed";
}

get_string_token_result GetStringToken::parse_string(const QString& input) const {
    qDebug() << "iiXml::writer::GetStringToken::parse_string begin"
             << "input_size=" << input.size();
    try {
        const std::string bytes = to_utf8_string(input);
        const GetFile validated_input;
        const get_file_result result =
            validated_input.parse_xml(std::string_view(bytes.data(), bytes.size()));
        const get_string_token_result converted = from_get_file_result(result);
        qDebug() << "iiXml::writer::GetStringToken::parse_string"
                 << (converted.status == get_string_token_status::parsed ? "parsed" : "failed")
                 << "status=" << status_name(converted.status)
                 << "has_token=" << converted.token.has_value();
        return converted;
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::writer::GetStringToken::parse_string exception"
                 << "what=" << exception.what();
        return make_string_result(
            get_string_token_status::exception_thrown,
            std::nullopt,
            std::string("QString token input exception: ") + exception.what()
        );
    } catch (...) {
        qDebug() << "iiXml::writer::GetStringToken::parse_string exception"
                 << "what=unknown";
        return make_string_result(
            get_string_token_status::exception_thrown,
            std::nullopt,
            "QString token input exception: unknown"
        );
    }
}

void GetStringToken::readString(const QString& input) {
    qDebug() << "iiXml::writer::GetStringToken::readString begin"
             << "input_size=" << input.size();
    try {
        const get_string_token_result result = parse_string(input);
        if (result.status != get_string_token_status::parsed || !result.token.has_value()) {
            const QString reason = from_utf8_string(result.reason);
            qDebug() << "iiXml::writer::GetStringToken::readString failed"
                     << "status=" << status_name(result.status)
                     << "reason=" << reason;
            emit failed(to_qt_status(result.status), reason);
            emit parseFailed(reason);
            return;
        }

        qDebug() << "iiXml::writer::GetStringToken::readString parsed"
                 << "tag=" << from_utf8_string(result.token->tag_name)
                 << "value_size=" << result.token->value.size();
        emit parsed(from_utf8_string(result.token->tag_name), from_utf8_string(result.token->value));
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::writer::GetStringToken::readString exception"
                 << "what=" << exception.what();
        const QString reason = QString::fromStdString(
            std::string("QString token slot exception: ") + exception.what()
        );
        emit failed(Status::ExceptionThrown, reason);
        emit parseFailed(reason);
    } catch (...) {
        qDebug() << "iiXml::writer::GetStringToken::readString exception"
                 << "what=unknown";
        const QString reason = "QString token slot exception: unknown";
        emit failed(Status::ExceptionThrown, reason);
        emit parseFailed(reason);
    }
}

} // namespace iiXml::writer
