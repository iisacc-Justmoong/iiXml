#include "DOCTYPE.h"

#include "Src/Logging/XmlLog.h"

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

constexpr std::string_view xml_declaration_start = "<?xml";
constexpr std::string_view xml_declaration_end = "?>";
constexpr std::string_view doctype_start = "<!DOCTYPE";
constexpr std::string_view utf8_bom = "\xEF\xBB\xBF";

bool is_space(char value) {
    return value == ' ' || value == '\n' || value == '\r' || value == '\t';
}

bool is_alpha(char value) {
    return ('a' <= value && value <= 'z') || ('A' <= value && value <= 'Z');
}

bool is_digit(char value) {
    return '0' <= value && value <= '9';
}

bool is_name_start(char value) {
    return is_alpha(value) || value == '_' || value == ':';
}

bool is_name_char(char value) {
    return is_name_start(value) || is_digit(value) || value == '-' || value == '.';
}

std::string_view trim_top(std::string_view input) {
    if (input.starts_with(utf8_bom)) {
        input.remove_prefix(utf8_bom.size());
    }

    while (!input.empty() && is_space(input.front())) {
        input.remove_prefix(1);
    }

    return input;
}

bool starts_with_token(std::string_view input, std::string_view token) {
    return input.size() >= token.size() && input.substr(0, token.size()) == token;
}

std::optional<std::size_t> find_xml_declaration_end(std::string_view input) {
    const std::size_t end = input.find(xml_declaration_end);
    if (end == std::string_view::npos) {
        return std::nullopt;
    }

    return end + xml_declaration_end.size();
}

std::optional<std::size_t> find_doctype_end(std::string_view input) {
    char quote = '\0';
    int subset_depth = 0;

    for (std::size_t index = 0; index < input.size(); ++index) {
        const char value = input[index];

        if (quote != '\0') {
            if (value == quote) {
                quote = '\0';
            }
            continue;
        }

        if (value == '"' || value == '\'') {
            quote = value;
            continue;
        }

        if (value == '[') {
            ++subset_depth;
            continue;
        }

        if (value == ']' && subset_depth > 0) {
            --subset_depth;
            continue;
        }

        if (value == '>' && subset_depth == 0) {
            return index + 1;
        }
    }

    return std::nullopt;
}

bool has_xml_declaration_body(std::string_view declaration) {
    if (declaration.size() <= xml_declaration_start.size() + xml_declaration_end.size()) {
        return false;
    }

    const char after_start = declaration[xml_declaration_start.size()];
    return is_space(after_start);
}

bool has_doctype_name(std::string_view declaration) {
    if (declaration.size() <= doctype_start.size() + 1) {
        return false;
    }

    std::size_t index = doctype_start.size();
    if (!is_space(declaration[index])) {
        return false;
    }

    while (index < declaration.size() && is_space(declaration[index])) {
        ++index;
    }

    if (index >= declaration.size() || !is_name_start(declaration[index])) {
        return false;
    }

    ++index;
    while (index < declaration.size() && is_name_char(declaration[index])) {
        ++index;
    }

    return index < declaration.size() && (is_space(declaration[index]) || declaration[index] == '>');
}

} // namespace

namespace iiXml::elements {

namespace {

DOCTYPE::Kind to_qt_kind(doctype_kind kind) {
    switch (kind) {
        case doctype_kind::xml_declaration:
            return DOCTYPE::Kind::XmlDeclaration;
        case doctype_kind::doctype_declaration:
            return DOCTYPE::Kind::DoctypeDeclaration;
    }

    return DOCTYPE::Kind::DoctypeDeclaration;
}

const char* status_name(doctype_status status) {
    switch (status) {
        case doctype_status::matched:
            return "matched";
        case doctype_status::empty_input:
            return "empty_input";
        case doctype_status::no_top_declaration:
            return "no_top_declaration";
        case doctype_status::malformed_xml_declaration:
            return "malformed_xml_declaration";
        case doctype_status::malformed_doctype_declaration:
            return "malformed_doctype_declaration";
        case doctype_status::exception_thrown:
            return "exception_thrown";
    }

    return "unknown";
}

doctype_result make_doctype_failure(doctype_status status, std::string reason) {
    return doctype_result{status, std::nullopt, std::move(reason)};
}

doctype_result make_doctype_success(doctype_match match) {
    return doctype_result{
        doctype_status::matched,
        std::move(match),
        "top declaration matched"
    };
}

} // namespace

DOCTYPE::DOCTYPE(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<DOCTYPE::Kind>("iiXml::elements::DOCTYPE::Kind");
    qDebug() << "iiXml::elements::DOCTYPE::DOCTYPE constructed";
}

doctype_result DOCTYPE::match_top_result(std::string_view input) const {
    qDebug() << "iiXml::elements::DOCTYPE::match_top_result begin"
             << "input_size=" << input.size();
    iiXml::logging::log_input_summary("iiXml::elements::DOCTYPE::match_top_result", input);
    try {
        input = trim_top(input);

        if (input.empty()) {
            iiXml::logging::log_parse_failure(
                "iiXml::elements::DOCTYPE::match_top_result",
                "input is empty; expected XML declaration or DOCTYPE declaration",
                input,
                0
            );
            return make_doctype_failure(
                doctype_status::empty_input,
                "input is empty; expected XML declaration or DOCTYPE declaration"
            );
        }

        if (starts_with_token(input, xml_declaration_start)) {
            iiXml::logging::log_parse_event(
                "iiXml::elements::DOCTYPE::match_top_result",
                "xml_declaration_begin",
                input,
                0
            );
            const std::optional<std::size_t> end = find_xml_declaration_end(input);
            if (!end.has_value()) {
                iiXml::logging::log_parse_failure(
                    "iiXml::elements::DOCTYPE::match_top_result",
                    "XML declaration is not closed with ?>",
                    input,
                    0
                );
                return make_doctype_failure(
                    doctype_status::malformed_xml_declaration,
                    "XML declaration is not closed with ?>"
                );
            }

            const std::string_view raw = input.substr(0, *end);
            if (!has_xml_declaration_body(raw)) {
                iiXml::logging::log_parse_failure(
                    "iiXml::elements::DOCTYPE::match_top_result",
                    "XML declaration must contain a body after <?xml",
                    input,
                    xml_declaration_start.size()
                );
                return make_doctype_failure(
                    doctype_status::malformed_xml_declaration,
                    "XML declaration must contain a body after <?xml"
                );
            }

            qDebug() << "iiXml::elements::DOCTYPE::match_top_result matched"
                     << "kind=xml_declaration"
                     << "raw_size=" << raw.size();
            iiXml::logging::log_output_summary(
                "iiXml::elements::DOCTYPE::match_top_result",
                "matched",
                std::string("kind=xml_declaration raw_size=") + std::to_string(raw.size())
            );
            return make_doctype_success(doctype_match{
                doctype_kind::xml_declaration,
                std::string(raw)
            });
        }

        if (starts_with_token(input, doctype_start)) {
            iiXml::logging::log_parse_event(
                "iiXml::elements::DOCTYPE::match_top_result",
                "doctype_declaration_begin",
                input,
                0
            );
            const std::optional<std::size_t> end = find_doctype_end(input);
            if (!end.has_value()) {
                iiXml::logging::log_parse_failure(
                    "iiXml::elements::DOCTYPE::match_top_result",
                    "DOCTYPE declaration is not closed with >",
                    input,
                    0
                );
                return make_doctype_failure(
                    doctype_status::malformed_doctype_declaration,
                    "DOCTYPE declaration is not closed with >"
                );
            }

            const std::string_view raw = input.substr(0, *end);
            if (!has_doctype_name(raw)) {
                iiXml::logging::log_parse_failure(
                    "iiXml::elements::DOCTYPE::match_top_result",
                    "DOCTYPE declaration is missing a valid root name",
                    input,
                    doctype_start.size()
                );
                return make_doctype_failure(
                    doctype_status::malformed_doctype_declaration,
                    "DOCTYPE declaration is missing a valid root name"
                );
            }

            qDebug() << "iiXml::elements::DOCTYPE::match_top_result matched"
                     << "kind=doctype_declaration"
                     << "raw_size=" << raw.size();
            iiXml::logging::log_output_summary(
                "iiXml::elements::DOCTYPE::match_top_result",
                "matched",
                std::string("kind=doctype_declaration raw_size=") + std::to_string(raw.size())
            );
            return make_doctype_success(doctype_match{
                doctype_kind::doctype_declaration,
                std::string(raw)
            });
        }

        iiXml::logging::log_parse_failure(
            "iiXml::elements::DOCTYPE::match_top_result",
            "top declaration is missing; expected XML declaration or DOCTYPE declaration",
            input,
            0
        );
        return make_doctype_failure(
            doctype_status::no_top_declaration,
            "top declaration is missing; expected XML declaration or DOCTYPE declaration"
        );
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::elements::DOCTYPE::match_top_result exception"
                 << "what=" << exception.what();
        return make_doctype_failure(
            doctype_status::exception_thrown,
            std::string("DOCTYPE match exception: ") + exception.what()
        );
    } catch (...) {
        qDebug() << "iiXml::elements::DOCTYPE::match_top_result exception"
                 << "what=unknown";
        return make_doctype_failure(
            doctype_status::exception_thrown,
            "DOCTYPE match exception: unknown"
        );
    }
}

doctype_result DOCTYPE::match_top(std::string_view input) const {
    qDebug() << "iiXml::elements::DOCTYPE::match_top begin"
             << "input_size=" << input.size();
    iiXml::logging::log_input_summary("iiXml::elements::DOCTYPE::match_top", input);
    const doctype_result result = match_top_result(input);
    if (result.match.has_value()) {
        qDebug() << "iiXml::elements::DOCTYPE::match_top matched"
                 << "status=" << status_name(result.status);
        iiXml::logging::log_output_summary(
            "iiXml::elements::DOCTYPE::match_top",
            status_name(result.status),
            result.match.has_value()
                ? std::string("raw_size=") + std::to_string(result.match->raw.size())
                : "raw_size=0"
        );
    } else {
        qDebug() << "iiXml::elements::DOCTYPE::match_top failed"
                 << "status=" << status_name(result.status)
                 << "reason=" << QString::fromStdString(result.reason);
    }
    return result;
}

std::optional<doctype_match> DOCTYPE::match_top_match(std::string_view input) const {
    qDebug() << "iiXml::elements::DOCTYPE::match_top_match begin"
             << "input_size=" << input.size();
    iiXml::logging::log_input_summary("iiXml::elements::DOCTYPE::match_top_match", input);
    const doctype_result result = match_top_result(input);
    if (result.match.has_value()) {
        qDebug() << "iiXml::elements::DOCTYPE::match_top_match matched";
        iiXml::logging::log_output_summary(
            "iiXml::elements::DOCTYPE::match_top_match",
            "matched",
            std::string("raw_size=") + std::to_string(result.match->raw.size())
        );
    } else {
        qDebug() << "iiXml::elements::DOCTYPE::match_top_match failed"
                 << "status=" << status_name(result.status);
    }
    return result.match;
}

bool DOCTYPE::is_top_doctype(std::string_view input) const {
    qDebug() << "iiXml::elements::DOCTYPE::is_top_doctype begin"
             << "input_size=" << input.size();
    iiXml::logging::log_input_summary("iiXml::elements::DOCTYPE::is_top_doctype", input);
    const doctype_result matched = match_top(input);
    const bool result =
        matched.match.has_value() && matched.match->kind == doctype_kind::doctype_declaration;
    qDebug() << "iiXml::elements::DOCTYPE::is_top_doctype"
             << (result ? "matched" : "failed")
             << "result=" << result;
    iiXml::logging::log_output_summary(
        "iiXml::elements::DOCTYPE::is_top_doctype",
        result ? "matched" : "failed",
        std::string("result=") + (result ? "true" : "false")
    );
    return result;
}

bool DOCTYPE::is_top_xml_declaration(std::string_view input) const {
    qDebug() << "iiXml::elements::DOCTYPE::is_top_xml_declaration begin"
             << "input_size=" << input.size();
    iiXml::logging::log_input_summary("iiXml::elements::DOCTYPE::is_top_xml_declaration", input);
    const doctype_result matched = match_top(input);
    const bool result =
        matched.match.has_value() && matched.match->kind == doctype_kind::xml_declaration;
    qDebug() << "iiXml::elements::DOCTYPE::is_top_xml_declaration"
             << (result ? "matched" : "failed")
             << "result=" << result;
    iiXml::logging::log_output_summary(
        "iiXml::elements::DOCTYPE::is_top_xml_declaration",
        result ? "matched" : "failed",
        std::string("result=") + (result ? "true" : "false")
    );
    return result;
}

void DOCTYPE::matchTop(const QString& input) {
    qDebug() << "iiXml::elements::DOCTYPE::matchTop begin"
             << "input_size=" << input.size();
    try {
        const QByteArray utf8 = input.toUtf8();
        const std::string bytes(utf8.constData(), static_cast<std::size_t>(utf8.size()));
        iiXml::logging::log_input_summary(
            "iiXml::elements::DOCTYPE::matchTop",
            std::string_view(bytes.data(), bytes.size())
        );
        const doctype_result result = match_top_result(std::string_view(bytes.data(), bytes.size()));

        if (!result.match.has_value()) {
            qDebug() << "iiXml::elements::DOCTYPE::matchTop rejected"
                     << "status=" << status_name(result.status);
            emit doctypeRejected(QString::fromStdString(result.reason));
            return;
        }

        const QString raw = QString::fromStdString(result.match->raw);
        const Kind kind = to_qt_kind(result.match->kind);
        qDebug() << "iiXml::elements::DOCTYPE::matchTop matched"
                 << "kind=" << static_cast<int>(kind)
                 << "raw_size=" << raw.size();
        iiXml::logging::log_output_summary(
            "iiXml::elements::DOCTYPE::matchTop",
            "matched",
            std::string("raw_size=") + std::to_string(result.match->raw.size())
        );
        emit doctypeMatched(kind, raw);

        if (kind == Kind::XmlDeclaration) {
            emit xmlDeclarationMatched(raw);
            return;
        }

        emit doctypeDeclarationMatched(raw);
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::elements::DOCTYPE::matchTop exception"
                 << "what=" << exception.what();
        emit doctypeRejected(QString::fromStdString(
            std::string("DOCTYPE slot exception: ") + exception.what()
        ));
    } catch (...) {
        qDebug() << "iiXml::elements::DOCTYPE::matchTop exception"
                 << "what=unknown";
        emit doctypeRejected("DOCTYPE slot exception: unknown");
    }
}

} // namespace iiXml::elements
