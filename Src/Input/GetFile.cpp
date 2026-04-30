#include "GetFile.h"

#include "Src/Elements/DOCTYPE.h"
#include "Src/Parser/TagParser.h"
#include "Src/Input/InputValidator.h"

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
    const iiXml::elements::doctype_match& matched
) {
    std::size_t offset = 0;
    if (input.starts_with(utf8_bom)) {
        offset += utf8_bom.size();
    }

    while (offset < input.size() && is_space(input[offset])) {
        ++offset;
    }

    offset += matched.raw.size();
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
    const iiXml::elements::DOCTYPE doctype;
    while (true) {
        const iiXml::elements::doctype_result matched = doctype.match_top(input);
        if (!matched.match.has_value()) {
            break;
        }

        input = consume_matched_declaration(input, *matched.match);
    }

    return trim_outer(input);
}

iiXml::writer::GetFile::Status to_qt_status(iiXml::writer::get_file_status status) {
    switch (status) {
        case iiXml::writer::get_file_status::parsed:
            return iiXml::writer::GetFile::Status::Parsed;
        case iiXml::writer::get_file_status::file_read_failed:
            return iiXml::writer::GetFile::Status::FileReadFailed;
        case iiXml::writer::get_file_status::invalid_xml_file:
            return iiXml::writer::GetFile::Status::InvalidXmlFile;
        case iiXml::writer::get_file_status::invalid_tag_closure:
            return iiXml::writer::GetFile::Status::InvalidTagClosure;
        case iiXml::writer::get_file_status::parser_rejected:
            return iiXml::writer::GetFile::Status::ParserRejected;
        case iiXml::writer::get_file_status::exception_thrown:
            return iiXml::writer::GetFile::Status::ExceptionThrown;
    }

    return iiXml::writer::GetFile::Status::ExceptionThrown;
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

const char* status_name(iiXml::writer::get_file_status status) {
    switch (status) {
        case iiXml::writer::get_file_status::parsed:
            return "parsed";
        case iiXml::writer::get_file_status::file_read_failed:
            return "file_read_failed";
        case iiXml::writer::get_file_status::invalid_xml_file:
            return "invalid_xml_file";
        case iiXml::writer::get_file_status::invalid_tag_closure:
            return "invalid_tag_closure";
        case iiXml::writer::get_file_status::parser_rejected:
            return "parser_rejected";
        case iiXml::writer::get_file_status::exception_thrown:
            return "exception_thrown";
    }

    return "unknown";
}

iiXml::writer::get_file_result make_result(
    iiXml::writer::get_file_status status,
    std::optional<iiXml::parser::tag_value> token,
    std::string reason
) {
    if (reason.empty()) {
        reason = reason_for_status(status).toStdString();
    }

    return iiXml::writer::get_file_result{status, std::move(token), std::move(reason)};
}

} // namespace

namespace iiXml::writer {

GetFile::GetFile(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<GetFile::Status>("iiXml::writer::GetFile::Status");
    qDebug() << "iiXml::writer::GetFile::GetFile constructed";
}

get_file_result GetFile::parse_file(const std::filesystem::path& file_path) const {
    qDebug() << "iiXml::writer::GetFile::parse_file begin"
             << "path=" << QString::fromStdString(file_path.string());
    try {
        const std::optional<std::string> content = read_file(file_path);
        if (!content.has_value()) {
            qDebug() << "iiXml::writer::GetFile::parse_file failed"
                     << "status=" << status_name(get_file_status::file_read_failed);
            return make_result(
                get_file_status::file_read_failed,
                std::nullopt,
                "file read failed"
            );
        }

        const get_file_result result = parse_xml(*content);
        qDebug() << "iiXml::writer::GetFile::parse_file"
                 << (result.status == get_file_status::parsed ? "parsed" : "failed")
                 << "status=" << status_name(result.status);
        return result;
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::writer::GetFile::parse_file exception"
                 << "what=" << exception.what();
        return make_result(
            get_file_status::exception_thrown,
            std::nullopt,
            std::string("file input exception: ") + exception.what()
        );
    } catch (...) {
        qDebug() << "iiXml::writer::GetFile::parse_file exception"
                 << "what=unknown";
        return make_result(
            get_file_status::exception_thrown,
            std::nullopt,
            "file input exception: unknown"
        );
    }
}

get_file_result GetFile::parse_xml(std::string_view input) const {
    qDebug() << "iiXml::writer::GetFile::parse_xml begin"
             << "input_size=" << input.size();
    try {
        const iiXml::elements::DOCTYPE doctype;
        const iiXml::elements::doctype_result declaration = doctype.match_top_result(input);
        if (!declaration.match.has_value()) {
            qDebug() << "iiXml::writer::GetFile::parse_xml failed"
                     << "status=" << status_name(get_file_status::invalid_xml_file);
            return make_result(
                get_file_status::invalid_xml_file,
                std::nullopt,
                declaration.reason
            );
        }

        const InputValidator validator;
        const validation_result validation = validator.validate_result(input);
        if (validation.exit == validation_exit::invalid_xml_file) {
            qDebug() << "iiXml::writer::GetFile::parse_xml failed"
                     << "status=" << status_name(get_file_status::invalid_xml_file);
            return make_result(
                get_file_status::invalid_xml_file,
                std::nullopt,
                validation.reason
            );
        }

        if (validation.exit == validation_exit::invalid_tag_closure) {
            qDebug() << "iiXml::writer::GetFile::parse_xml failed"
                     << "status=" << status_name(get_file_status::invalid_tag_closure);
            return make_result(
                get_file_status::invalid_tag_closure,
                std::nullopt,
                validation.reason
            );
        }

        if (validation.exit == validation_exit::exception_thrown) {
            qDebug() << "iiXml::writer::GetFile::parse_xml failed"
                     << "status=" << status_name(get_file_status::exception_thrown);
            return make_result(
                get_file_status::exception_thrown,
                std::nullopt,
                validation.reason
            );
        }

        const std::string_view parser_input = parser_body_after_declarations(input);
        const iiXml::parser::tag_parser parser;
        const std::optional<iiXml::parser::tag_value> token = parser.parse(parser_input);
        if (!token.has_value()) {
            qDebug() << "iiXml::writer::GetFile::parse_xml failed"
                     << "status=" << status_name(get_file_status::parser_rejected);
            return make_result(
                get_file_status::parser_rejected,
                std::nullopt,
                "tag parser rejected validated XML body"
            );
        }

        qDebug() << "iiXml::writer::GetFile::parse_xml parsed"
                 << "tag=" << QString::fromStdString(token->tag_name)
                 << "value_size=" << token->value.size();
        return make_result(get_file_status::parsed, token, "parsed");
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::writer::GetFile::parse_xml exception"
                 << "what=" << exception.what();
        return make_result(
            get_file_status::exception_thrown,
            std::nullopt,
            std::string("XML input exception: ") + exception.what()
        );
    } catch (...) {
        qDebug() << "iiXml::writer::GetFile::parse_xml exception"
                 << "what=unknown";
        return make_result(
            get_file_status::exception_thrown,
            std::nullopt,
            "XML input exception: unknown"
        );
    }
}

void GetFile::readFile(const QString& file_path) {
    qDebug() << "iiXml::writer::GetFile::readFile begin"
             << "path=" << file_path;
    try {
        const std::string path = to_utf8_string(file_path);
        const get_file_result result = parse_file(std::filesystem::path(path));
        if (result.status == get_file_status::parsed && result.token.has_value()) {
            qDebug() << "iiXml::writer::GetFile::readFile parsed"
                     << "tag=" << QString::fromStdString(result.token->tag_name)
                     << "value_size=" << result.token->value.size();
            emit parsed(
                QString::fromStdString(result.token->tag_name),
                QString::fromStdString(result.token->value)
            );
            return;
        }

        const Status status = to_qt_status(result.status);
        const QString reason = result.reason.empty()
            ? reason_for_status(result.status)
            : QString::fromStdString(result.reason);
        qDebug() << "iiXml::writer::GetFile::readFile failed"
                 << "status=" << status_name(result.status)
                 << "reason=" << reason;
        emit failed(status, reason);

        switch (status) {
            case Status::FileReadFailed:
                emit fileReadFailed();
                return;
            case Status::InvalidXmlFile:
                emit invalidXmlFile();
                return;
            case Status::InvalidTagClosure:
                emit invalidTagClosure();
                return;
            case Status::ParserRejected:
                emit parserRejected();
                return;
            case Status::ExceptionThrown:
                emit exceptionThrown();
                return;
            case Status::Parsed:
                return;
        }
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::writer::GetFile::readFile exception"
                 << "what=" << exception.what();
        emit failed(Status::ExceptionThrown, QString::fromStdString(
            std::string("file input slot exception: ") + exception.what()
        ));
        emit exceptionThrown();
    } catch (...) {
        qDebug() << "iiXml::writer::GetFile::readFile exception"
                 << "what=unknown";
        emit failed(Status::ExceptionThrown, "file input slot exception: unknown");
        emit exceptionThrown();
    }
}

void GetFile::readXml(const QString& input) {
    qDebug() << "iiXml::writer::GetFile::readXml begin"
             << "input_size=" << input.size();
    try {
        const std::string bytes = to_utf8_string(input);
        const get_file_result result = parse_xml(std::string_view(bytes.data(), bytes.size()));
        if (result.status == get_file_status::parsed && result.token.has_value()) {
            qDebug() << "iiXml::writer::GetFile::readXml parsed"
                     << "tag=" << QString::fromStdString(result.token->tag_name)
                     << "value_size=" << result.token->value.size();
            emit parsed(
                QString::fromStdString(result.token->tag_name),
                QString::fromStdString(result.token->value)
            );
            return;
        }

        const Status status = to_qt_status(result.status);
        const QString reason = result.reason.empty()
            ? reason_for_status(result.status)
            : QString::fromStdString(result.reason);
        qDebug() << "iiXml::writer::GetFile::readXml failed"
                 << "status=" << status_name(result.status)
                 << "reason=" << reason;
        emit failed(status, reason);

        switch (status) {
            case Status::FileReadFailed:
                emit fileReadFailed();
                return;
            case Status::InvalidXmlFile:
                emit invalidXmlFile();
                return;
            case Status::InvalidTagClosure:
                emit invalidTagClosure();
                return;
            case Status::ParserRejected:
                emit parserRejected();
                return;
            case Status::ExceptionThrown:
                emit exceptionThrown();
                return;
            case Status::Parsed:
                return;
        }
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::writer::GetFile::readXml exception"
                 << "what=" << exception.what();
        emit failed(Status::ExceptionThrown, QString::fromStdString(
            std::string("XML input slot exception: ") + exception.what()
        ));
        emit exceptionThrown();
    } catch (...) {
        qDebug() << "iiXml::writer::GetFile::readXml exception"
                 << "what=unknown";
        emit failed(Status::ExceptionThrown, "XML input slot exception: unknown");
        emit exceptionThrown();
    }
}

} // namespace iiXml::writer
