#include "GetFile.h"

#include "Src/Elements/Doctype.h"
#include "Src/Input/InputValidator.h"
#include "Src/Logging/XmlLog.h"
#include "Src/Parser/TagParser.h"

#include <QByteArray>
#include <QDebug>
#include <QMetaType>
#include <QString>

#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace {

constexpr std::string_view utf8_bom = "\xEF\xBB\xBF";

bool is_space(char value) {
    return value == ' ' || value == '\n' || value == '\r' || value == '\t';
}

std::string to_utf8_string(const QString& value) {
    const QByteArray utf8 = value.toUtf8();
    return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

std::optional<std::string> read_file(const std::filesystem::path& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string_view consume_matched_declaration(
    std::string_view input,
    const iiXml::Elements::DoctypeMatch& matched
) {
    std::size_t offset = 0;
    if (input.starts_with(utf8_bom)) {
        offset += utf8_bom.size();
    }

    while (offset < input.size() && is_space(input[offset])) {
        ++offset;
    }

    offset += matched.Raw.size();
    return offset <= input.size() ? input.substr(offset) : std::string_view{};
}

std::string_view trim_outer(std::string_view input) {
    std::size_t begin = 0;
    while (begin < input.size() && is_space(input[begin])) {
        ++begin;
    }

    std::size_t end = input.size();
    while (end > begin && is_space(input[end - 1])) {
        --end;
    }

    return input.substr(begin, end - begin);
}

std::string_view parser_body_after_declarations(std::string_view input) {
    const iiXml::Elements::Doctype doctype;
    while (true) {
        const iiXml::Elements::DoctypeResult matched = doctype.MatchTop(input);
        if (!matched.Match.has_value()) {
            break;
        }

        input = consume_matched_declaration(input, *matched.Match);
    }

    return trim_outer(input);
}

iiXml::Writer::GetFile::Status to_qt_status(iiXml::Writer::GetFileStatus status) {
    switch (status) {
        case iiXml::Writer::GetFileStatus::Parsed:
            return iiXml::Writer::GetFile::Status::Parsed;
        case iiXml::Writer::GetFileStatus::FileReadFailed:
            return iiXml::Writer::GetFile::Status::FileReadFailed;
        case iiXml::Writer::GetFileStatus::InvalidXmlFile:
            return iiXml::Writer::GetFile::Status::InvalidXmlFile;
        case iiXml::Writer::GetFileStatus::InvalidTagClosure:
            return iiXml::Writer::GetFile::Status::InvalidTagClosure;
        case iiXml::Writer::GetFileStatus::ParserRejected:
            return iiXml::Writer::GetFile::Status::ParserRejected;
        case iiXml::Writer::GetFileStatus::ExceptionThrown:
            return iiXml::Writer::GetFile::Status::ExceptionThrown;
    }

    return iiXml::Writer::GetFile::Status::ExceptionThrown;
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

const char* status_name(iiXml::Writer::GetFileStatus status) {
    switch (status) {
        case iiXml::Writer::GetFileStatus::Parsed:
            return "parsed";
        case iiXml::Writer::GetFileStatus::FileReadFailed:
            return "file_read_failed";
        case iiXml::Writer::GetFileStatus::InvalidXmlFile:
            return "invalid_xml_file";
        case iiXml::Writer::GetFileStatus::InvalidTagClosure:
            return "invalid_tag_closure";
        case iiXml::Writer::GetFileStatus::ParserRejected:
            return "parser_rejected";
        case iiXml::Writer::GetFileStatus::ExceptionThrown:
            return "exception_thrown";
    }

    return "unknown";
}

iiXml::Writer::GetFileResult make_result(
    iiXml::Writer::GetFileStatus status,
    std::optional<iiXml::Parser::TagValue> token,
    std::string reason
) {
    if (reason.empty()) {
        reason = reason_for_status(status).toStdString();
    }

    return iiXml::Writer::GetFileResult{status, std::move(token), std::move(reason)};
}

} // namespace

namespace iiXml::Writer {

GetFile::GetFile(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<GetFile::Status>("iiXml::Writer::GetFile::Status");
    qDebug() << "iiXml::Writer::GetFile::GetFile constructed";
}

GetFileResult GetFile::ParseFile(const std::filesystem::path& file_path) const {
    qDebug() << "iiXml::Writer::GetFile::ParseFile begin"
             << "path=" << QString::fromStdString(file_path.string());
    try {
        const std::optional<std::string> content = read_file(file_path);
        if (!content.has_value()) {
            qDebug() << "iiXml::Writer::GetFile::ParseFile failed"
                     << "status=" << status_name(GetFileStatus::FileReadFailed);
            return make_result(
                GetFileStatus::FileReadFailed,
                std::nullopt,
                "file read failed"
            );
        }
        iiXml::Logging::LogInputSummary(
            "iiXml::Writer::GetFile::ParseFile",
            *content
        );

        const GetFileResult result = ParseXml(*content);
        qDebug() << "iiXml::Writer::GetFile::ParseFile"
                 << (result.Status == GetFileStatus::Parsed ? "parsed" : "failed")
                 << "status=" << status_name(result.Status);
        iiXml::Logging::LogOutputSummary(
            "iiXml::Writer::GetFile::ParseFile",
            status_name(result.Status),
            result.Token.has_value()
                ? std::string("tag=") + result.Token->TagName
                    + " value_size=" + std::to_string(result.Token->Value.size())
                : result.Reason
        );
        return result;
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Writer::GetFile::ParseFile exception"
                 << "what=" << exception.what();
        return make_result(
            GetFileStatus::ExceptionThrown,
            std::nullopt,
            std::string("file input exception: ") + exception.what()
        );
    } catch (...) {
        qDebug() << "iiXml::Writer::GetFile::ParseFile exception"
                 << "what=unknown";
        return make_result(
            GetFileStatus::ExceptionThrown,
            std::nullopt,
            "file input exception: unknown"
        );
    }
}

GetFileResult GetFile::ParseXml(std::string_view input) const {
    qDebug() << "iiXml::Writer::GetFile::ParseXml begin"
             << "input_size=" << input.size();
    iiXml::Logging::LogInputSummary("iiXml::Writer::GetFile::ParseXml", input);
    try {
        const iiXml::Elements::Doctype doctype;
        const iiXml::Elements::DoctypeResult declaration = doctype.MatchTopResult(input);
        if (!declaration.Match.has_value()) {
            qDebug() << "iiXml::Writer::GetFile::ParseXml failed"
                     << "status=" << status_name(GetFileStatus::InvalidXmlFile);
            return make_result(
                GetFileStatus::InvalidXmlFile,
                std::nullopt,
                declaration.Reason
            );
        }

        const InputValidator validator;
        const ValidationResult validation = validator.ValidateResult(input);
        if (validation.Exit == ValidationExit::InvalidXmlFile) {
            qDebug() << "iiXml::Writer::GetFile::ParseXml failed"
                     << "status=" << status_name(GetFileStatus::InvalidXmlFile);
            return make_result(
                GetFileStatus::InvalidXmlFile,
                std::nullopt,
                validation.Reason
            );
        }

        if (validation.Exit == ValidationExit::InvalidTagClosure) {
            qDebug() << "iiXml::Writer::GetFile::ParseXml failed"
                     << "status=" << status_name(GetFileStatus::InvalidTagClosure);
            return make_result(
                GetFileStatus::InvalidTagClosure,
                std::nullopt,
                validation.Reason
            );
        }

        if (validation.Exit == ValidationExit::ExceptionThrown) {
            qDebug() << "iiXml::Writer::GetFile::ParseXml failed"
                     << "status=" << status_name(GetFileStatus::ExceptionThrown);
            return make_result(
                GetFileStatus::ExceptionThrown,
                std::nullopt,
                validation.Reason
            );
        }

        const std::string_view parser_input = parser_body_after_declarations(input);
        iiXml::Logging::LogParseEvent(
            "iiXml::Writer::GetFile::ParseXml",
            "parser_body",
            input,
            input.size() - parser_input.size()
        );
        const iiXml::Parser::TagParser parser;
        const std::optional<iiXml::Parser::TagValue> token = parser.Parse(parser_input);
        if (!token.has_value()) {
            qDebug() << "iiXml::Writer::GetFile::ParseXml failed"
                     << "status=" << status_name(GetFileStatus::ParserRejected);
            return make_result(
                GetFileStatus::ParserRejected,
                std::nullopt,
                "tag parser rejected validated XML body"
            );
        }

        qDebug() << "iiXml::Writer::GetFile::ParseXml parsed"
                 << "tag=" << QString::fromStdString(token->TagName)
                 << "value_size=" << token->Value.size();
        iiXml::Logging::LogOutputSummary(
            "iiXml::Writer::GetFile::ParseXml",
            "parsed",
            std::string("tag=") + token->TagName
                + " value_size=" + std::to_string(token->Value.size())
        );
        return make_result(GetFileStatus::Parsed, token, "parsed");
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Writer::GetFile::ParseXml exception"
                 << "what=" << exception.what();
        return make_result(
            GetFileStatus::ExceptionThrown,
            std::nullopt,
            std::string("XML input exception: ") + exception.what()
        );
    } catch (...) {
        qDebug() << "iiXml::Writer::GetFile::ParseXml exception"
                 << "what=unknown";
        return make_result(
            GetFileStatus::ExceptionThrown,
            std::nullopt,
            "XML input exception: unknown"
        );
    }
}

void GetFile::ReadFile(const QString& file_path) {
    qDebug() << "iiXml::Writer::GetFile::ReadFile begin"
             << "path=" << file_path;
    try {
        const std::string path = to_utf8_string(file_path);
        const GetFileResult result = ParseFile(std::filesystem::path(path));
        if (result.Status == GetFileStatus::Parsed && result.Token.has_value()) {
            qDebug() << "iiXml::Writer::GetFile::ReadFile parsed"
                     << "tag=" << QString::fromStdString(result.Token->TagName)
                     << "value_size=" << result.Token->Value.size();
            iiXml::Logging::LogOutputSummary(
                "iiXml::Writer::GetFile::ReadFile",
                "parsed",
                std::string("tag=") + result.Token->TagName
                    + " value_size=" + std::to_string(result.Token->Value.size())
            );
            emit Parsed(
                QString::fromStdString(result.Token->TagName),
                QString::fromStdString(result.Token->Value)
            );
            return;
        }

        const Status status = to_qt_status(result.Status);
        const QString reason = result.Reason.empty()
            ? reason_for_status(result.Status)
            : QString::fromStdString(result.Reason);
        qDebug() << "iiXml::Writer::GetFile::ReadFile failed"
                 << "status=" << status_name(result.Status)
                 << "reason=" << reason;
        iiXml::Logging::LogOutputSummary(
            "iiXml::Writer::GetFile::ReadFile",
            status_name(result.Status),
            result.Reason
        );
        emit Failed(status, reason);

        switch (status) {
            case Status::FileReadFailed:
                emit FileReadFailed();
                return;
            case Status::InvalidXmlFile:
                emit InvalidXmlFile();
                return;
            case Status::InvalidTagClosure:
                emit InvalidTagClosure();
                return;
            case Status::ParserRejected:
                emit ParserRejected();
                return;
            case Status::ExceptionThrown:
                emit ExceptionThrown();
                return;
            case Status::Parsed:
                return;
        }
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Writer::GetFile::ReadFile exception"
                 << "what=" << exception.what();
        emit Failed(Status::ExceptionThrown, QString::fromStdString(
            std::string("file input slot exception: ") + exception.what()
        ));
        emit ExceptionThrown();
    } catch (...) {
        qDebug() << "iiXml::Writer::GetFile::ReadFile exception"
                 << "what=unknown";
        emit Failed(Status::ExceptionThrown, "file input slot exception: unknown");
        emit ExceptionThrown();
    }
}

void GetFile::ReadXml(const QString& input) {
    qDebug() << "iiXml::Writer::GetFile::ReadXml begin"
             << "input_size=" << input.size();
    try {
        const std::string bytes = to_utf8_string(input);
        iiXml::Logging::LogInputSummary(
            "iiXml::Writer::GetFile::ReadXml",
            std::string_view(bytes.data(), bytes.size())
        );
        const GetFileResult result = ParseXml(std::string_view(bytes.data(), bytes.size()));
        if (result.Status == GetFileStatus::Parsed && result.Token.has_value()) {
            qDebug() << "iiXml::Writer::GetFile::ReadXml parsed"
                     << "tag=" << QString::fromStdString(result.Token->TagName)
                     << "value_size=" << result.Token->Value.size();
            iiXml::Logging::LogOutputSummary(
                "iiXml::Writer::GetFile::ReadXml",
                "parsed",
                std::string("tag=") + result.Token->TagName
                    + " value_size=" + std::to_string(result.Token->Value.size())
            );
            emit Parsed(
                QString::fromStdString(result.Token->TagName),
                QString::fromStdString(result.Token->Value)
            );
            return;
        }

        const Status status = to_qt_status(result.Status);
        const QString reason = result.Reason.empty()
            ? reason_for_status(result.Status)
            : QString::fromStdString(result.Reason);
        qDebug() << "iiXml::Writer::GetFile::ReadXml failed"
                 << "status=" << status_name(result.Status)
                 << "reason=" << reason;
        iiXml::Logging::LogOutputSummary(
            "iiXml::Writer::GetFile::ReadXml",
            status_name(result.Status),
            result.Reason
        );
        emit Failed(status, reason);

        switch (status) {
            case Status::FileReadFailed:
                emit FileReadFailed();
                return;
            case Status::InvalidXmlFile:
                emit InvalidXmlFile();
                return;
            case Status::InvalidTagClosure:
                emit InvalidTagClosure();
                return;
            case Status::ParserRejected:
                emit ParserRejected();
                return;
            case Status::ExceptionThrown:
                emit ExceptionThrown();
                return;
            case Status::Parsed:
                return;
        }
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Writer::GetFile::ReadXml exception"
                 << "what=" << exception.what();
        emit Failed(Status::ExceptionThrown, QString::fromStdString(
            std::string("XML input slot exception: ") + exception.what()
        ));
        emit ExceptionThrown();
    } catch (...) {
        qDebug() << "iiXml::Writer::GetFile::ReadXml exception"
                 << "what=unknown";
        emit Failed(Status::ExceptionThrown, "XML input slot exception: unknown");
        emit ExceptionThrown();
    }
}

} // namespace iiXml::Writer
