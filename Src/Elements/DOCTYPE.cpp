#include "Doctype.h"

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
constexpr std::string_view doctype_start = "<!Doctype";
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

namespace iiXml::Elements {

namespace {

Doctype::Kind to_qt_kind(DoctypeKind kind) {
    switch (kind) {
        case DoctypeKind::XmlDeclaration:
            return Doctype::Kind::XmlDeclaration;
        case DoctypeKind::DoctypeDeclaration:
            return Doctype::Kind::DoctypeDeclaration;
    }

    return Doctype::Kind::DoctypeDeclaration;
}

const char* status_name(DoctypeStatus status) {
    switch (status) {
        case DoctypeStatus::Matched:
            return "matched";
        case DoctypeStatus::EmptyInput:
            return "empty_input";
        case DoctypeStatus::NoTopDeclaration:
            return "no_top_declaration";
        case DoctypeStatus::MalformedXmlDeclaration:
            return "malformed_xml_declaration";
        case DoctypeStatus::MalformedDoctypeDeclaration:
            return "malformed_doctype_declaration";
        case DoctypeStatus::ExceptionThrown:
            return "exception_thrown";
    }

    return "unknown";
}

DoctypeResult make_doctype_failure(DoctypeStatus status, std::string reason) {
    return DoctypeResult{status, std::nullopt, std::move(reason)};
}

DoctypeResult make_doctype_success(DoctypeMatch match) {
    return DoctypeResult{
        DoctypeStatus::Matched,
        std::move(match),
        "top declaration matched"
    };
}

} // namespace

Doctype::Doctype(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<Doctype::Kind>("iiXml::Elements::Doctype::Kind");
    qDebug() << "iiXml::Elements::Doctype::Doctype constructed";
}

DoctypeResult Doctype::MatchTopResult(std::string_view input) const {
    qDebug() << "iiXml::Elements::Doctype::MatchTopResult begin"
             << "input_size=" << input.size();
    iiXml::Logging::LogInputSummary("iiXml::Elements::Doctype::MatchTopResult", input);
    try {
        input = trim_top(input);

        if (input.empty()) {
            iiXml::Logging::LogParseFailure(
                "iiXml::Elements::Doctype::MatchTopResult",
                "input is empty; expected XML declaration or Doctype declaration",
                input,
                0
            );
            return make_doctype_failure(
                DoctypeStatus::EmptyInput,
                "input is empty; expected XML declaration or Doctype declaration"
            );
        }

        if (starts_with_token(input, xml_declaration_start)) {
            iiXml::Logging::LogParseEvent(
                "iiXml::Elements::Doctype::MatchTopResult",
                "xml_declaration_begin",
                input,
                0
            );
            const std::optional<std::size_t> end = find_xml_declaration_end(input);
            if (!end.has_value()) {
                iiXml::Logging::LogParseFailure(
                    "iiXml::Elements::Doctype::MatchTopResult",
                    "XML declaration is not closed with ?>",
                    input,
                    0
                );
                return make_doctype_failure(
                    DoctypeStatus::MalformedXmlDeclaration,
                    "XML declaration is not closed with ?>"
                );
            }

            const std::string_view raw = input.substr(0, *end);
            if (!has_xml_declaration_body(raw)) {
                iiXml::Logging::LogParseFailure(
                    "iiXml::Elements::Doctype::MatchTopResult",
                    "XML declaration must contain a body after <?xml",
                    input,
                    xml_declaration_start.size()
                );
                return make_doctype_failure(
                    DoctypeStatus::MalformedXmlDeclaration,
                    "XML declaration must contain a body after <?xml"
                );
            }

            qDebug() << "iiXml::Elements::Doctype::MatchTopResult matched"
                     << "kind=xml_declaration"
                     << "raw_size=" << raw.size();
            iiXml::Logging::LogOutputSummary(
                "iiXml::Elements::Doctype::MatchTopResult",
                "matched",
                std::string("kind=xml_declaration raw_size=") + std::to_string(raw.size())
            );
            return make_doctype_success(DoctypeMatch{
                DoctypeKind::XmlDeclaration,
                std::string(raw)
            });
        }

        if (starts_with_token(input, doctype_start)) {
            iiXml::Logging::LogParseEvent(
                "iiXml::Elements::Doctype::MatchTopResult",
                "doctype_declaration_begin",
                input,
                0
            );
            const std::optional<std::size_t> end = find_doctype_end(input);
            if (!end.has_value()) {
                iiXml::Logging::LogParseFailure(
                    "iiXml::Elements::Doctype::MatchTopResult",
                    "Doctype declaration is not closed with >",
                    input,
                    0
                );
                return make_doctype_failure(
                    DoctypeStatus::MalformedDoctypeDeclaration,
                    "Doctype declaration is not closed with >"
                );
            }

            const std::string_view raw = input.substr(0, *end);
            if (!has_doctype_name(raw)) {
                iiXml::Logging::LogParseFailure(
                    "iiXml::Elements::Doctype::MatchTopResult",
                    "Doctype declaration is missing a valid root name",
                    input,
                    doctype_start.size()
                );
                return make_doctype_failure(
                    DoctypeStatus::MalformedDoctypeDeclaration,
                    "Doctype declaration is missing a valid root name"
                );
            }

            qDebug() << "iiXml::Elements::Doctype::MatchTopResult matched"
                     << "kind=doctype_declaration"
                     << "raw_size=" << raw.size();
            iiXml::Logging::LogOutputSummary(
                "iiXml::Elements::Doctype::MatchTopResult",
                "matched",
                std::string("kind=doctype_declaration raw_size=") + std::to_string(raw.size())
            );
            return make_doctype_success(DoctypeMatch{
                DoctypeKind::DoctypeDeclaration,
                std::string(raw)
            });
        }

        iiXml::Logging::LogParseFailure(
            "iiXml::Elements::Doctype::MatchTopResult",
            "top declaration is missing; expected XML declaration or Doctype declaration",
            input,
            0
        );
        return make_doctype_failure(
            DoctypeStatus::NoTopDeclaration,
            "top declaration is missing; expected XML declaration or Doctype declaration"
        );
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Elements::Doctype::MatchTopResult exception"
                 << "what=" << exception.what();
        return make_doctype_failure(
            DoctypeStatus::ExceptionThrown,
            std::string("Doctype match exception: ") + exception.what()
        );
    } catch (...) {
        qDebug() << "iiXml::Elements::Doctype::MatchTopResult exception"
                 << "what=unknown";
        return make_doctype_failure(
            DoctypeStatus::ExceptionThrown,
            "Doctype match exception: unknown"
        );
    }
}

DoctypeResult Doctype::MatchTop(std::string_view input) const {
    qDebug() << "iiXml::Elements::Doctype::MatchTop begin"
             << "input_size=" << input.size();
    iiXml::Logging::LogInputSummary("iiXml::Elements::Doctype::MatchTop", input);
    const DoctypeResult result = MatchTopResult(input);
    if (result.Match.has_value()) {
        qDebug() << "iiXml::Elements::Doctype::MatchTop matched"
                 << "status=" << status_name(result.Status);
        iiXml::Logging::LogOutputSummary(
            "iiXml::Elements::Doctype::MatchTop",
            status_name(result.Status),
            result.Match.has_value()
                ? std::string("raw_size=") + std::to_string(result.Match->Raw.size())
                : "raw_size=0"
        );
    } else {
        qDebug() << "iiXml::Elements::Doctype::MatchTop failed"
                 << "status=" << status_name(result.Status)
                 << "reason=" << QString::fromStdString(result.Reason);
    }
    return result;
}

std::optional<DoctypeMatch> Doctype::MatchTopMatch(std::string_view input) const {
    qDebug() << "iiXml::Elements::Doctype::MatchTopMatch begin"
             << "input_size=" << input.size();
    iiXml::Logging::LogInputSummary("iiXml::Elements::Doctype::MatchTopMatch", input);
    const DoctypeResult result = MatchTopResult(input);
    if (result.Match.has_value()) {
        qDebug() << "iiXml::Elements::Doctype::MatchTopMatch matched";
        iiXml::Logging::LogOutputSummary(
            "iiXml::Elements::Doctype::MatchTopMatch",
            "matched",
            std::string("raw_size=") + std::to_string(result.Match->Raw.size())
        );
    } else {
        qDebug() << "iiXml::Elements::Doctype::MatchTopMatch failed"
                 << "status=" << status_name(result.Status);
    }
    return result.Match;
}

bool Doctype::IsTopDoctype(std::string_view input) const {
    qDebug() << "iiXml::Elements::Doctype::IsTopDoctype begin"
             << "input_size=" << input.size();
    iiXml::Logging::LogInputSummary("iiXml::Elements::Doctype::IsTopDoctype", input);
    const DoctypeResult matched = MatchTop(input);
    const bool result =
        matched.Match.has_value() && matched.Match->Kind == DoctypeKind::DoctypeDeclaration;
    qDebug() << "iiXml::Elements::Doctype::IsTopDoctype"
             << (result ? "matched" : "failed")
             << "result=" << result;
    iiXml::Logging::LogOutputSummary(
        "iiXml::Elements::Doctype::IsTopDoctype",
        result ? "matched" : "failed",
        std::string("result=") + (result ? "true" : "false")
    );
    return result;
}

bool Doctype::IsTopXmlDeclaration(std::string_view input) const {
    qDebug() << "iiXml::Elements::Doctype::IsTopXmlDeclaration begin"
             << "input_size=" << input.size();
    iiXml::Logging::LogInputSummary("iiXml::Elements::Doctype::IsTopXmlDeclaration", input);
    const DoctypeResult matched = MatchTop(input);
    const bool result =
        matched.Match.has_value() && matched.Match->Kind == DoctypeKind::XmlDeclaration;
    qDebug() << "iiXml::Elements::Doctype::IsTopXmlDeclaration"
             << (result ? "matched" : "failed")
             << "result=" << result;
    iiXml::Logging::LogOutputSummary(
        "iiXml::Elements::Doctype::IsTopXmlDeclaration",
        result ? "matched" : "failed",
        std::string("result=") + (result ? "true" : "false")
    );
    return result;
}

void Doctype::MatchTopInput(const QString& input) {
    qDebug() << "iiXml::Elements::Doctype::MatchTopInput begin"
             << "input_size=" << input.size();
    try {
        const QByteArray utf8 = input.toUtf8();
        const std::string bytes(utf8.constData(), static_cast<std::size_t>(utf8.size()));
        iiXml::Logging::LogInputSummary(
            "iiXml::Elements::Doctype::MatchTopInput",
            std::string_view(bytes.data(), bytes.size())
        );
        const DoctypeResult result = MatchTopResult(std::string_view(bytes.data(), bytes.size()));

        if (!result.Match.has_value()) {
            qDebug() << "iiXml::Elements::Doctype::MatchTopInput rejected"
                     << "status=" << status_name(result.Status);
            emit DoctypeRejected(QString::fromStdString(result.Reason));
            return;
        }

        const QString raw = QString::fromStdString(result.Match->Raw);
        const Kind kind = to_qt_kind(result.Match->Kind);
        qDebug() << "iiXml::Elements::Doctype::MatchTopInput matched"
                 << "kind=" << static_cast<int>(kind)
                 << "raw_size=" << raw.size();
        iiXml::Logging::LogOutputSummary(
            "iiXml::Elements::Doctype::MatchTopInput",
            "matched",
            std::string("raw_size=") + std::to_string(result.Match->Raw.size())
        );
        emit DoctypeMatched(kind, raw);

        if (kind == Kind::XmlDeclaration) {
            emit XmlDeclarationMatched(raw);
            return;
        }

        emit DoctypeDeclarationMatched(raw);
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Elements::Doctype::MatchTopInput exception"
                 << "what=" << exception.what();
        emit DoctypeRejected(QString::fromStdString(
            std::string("Doctype slot exception: ") + exception.what()
        ));
    } catch (...) {
        qDebug() << "iiXml::Elements::Doctype::MatchTopInput exception"
                 << "what=unknown";
        emit DoctypeRejected("Doctype slot exception: unknown");
    }
}

} // namespace iiXml::Elements
