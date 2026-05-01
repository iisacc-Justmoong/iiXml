#include "InputValidator.h"

#include "Src/Logging/XmlLog.h"
#include "Src/Elements/Doctype.h"
#include "Src/Elements/OpenTag.h"

#include <QByteArray>
#include <QDebug>
#include <QMetaType>
#include <QString>

#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

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

std::string_view body_after_doctype(std::string_view input, const iiXml::Elements::DoctypeMatch& matched) {
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

bool starts_with_at(std::string_view input, std::size_t index, std::string_view token) {
    return index + token.size() <= input.size() && input.substr(index, token.size()) == token;
}

std::optional<std::size_t> find_token_end(std::string_view input, std::size_t begin, std::string_view token) {
    const std::size_t end = input.find(token, begin);
    if (end == std::string_view::npos) {
        return std::nullopt;
    }

    return end + token.size();
}

std::optional<std::size_t> find_markup_end(std::string_view input, std::size_t begin) {
    char quote = '\0';

    for (std::size_t index = begin; index < input.size(); ++index) {
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

        if (value == '>') {
            return index;
        }
    }

    return std::nullopt;
}

std::optional<std::string> read_tag_name(std::string_view markup, std::size_t begin) {
    if (begin >= markup.size() || !is_name_start(markup[begin])) {
        return std::nullopt;
    }

    std::size_t end = begin + 1;
    while (end < markup.size() && is_name_char(markup[end])) {
        ++end;
    }

    return std::string(markup.substr(begin, end - begin));
}

bool is_self_closing_markup(std::string_view markup) {
    if (markup.size() < 2 || markup.back() != '>') {
        return false;
    }

    std::size_t index = markup.size() - 1;
    while (index > 0 && is_space(markup[index - 1])) {
        --index;
    }

    return index > 0 && markup[index - 1] == '/';
}

} // namespace

namespace iiXml::Writer {

namespace {

InputValidator::ValidationExit to_qt_exit(iiXml::Writer::ValidationExit result) {
    switch (result) {
        case iiXml::Writer::ValidationExit::Valid:
            return InputValidator::ValidationExit::Valid;
        case iiXml::Writer::ValidationExit::InvalidXmlFile:
            return InputValidator::ValidationExit::InvalidXmlFile;
        case iiXml::Writer::ValidationExit::InvalidTagClosure:
            return InputValidator::ValidationExit::InvalidTagClosure;
        case iiXml::Writer::ValidationExit::ExceptionThrown:
            return InputValidator::ValidationExit::ExceptionThrown;
    }

    return InputValidator::ValidationExit::ExceptionThrown;
}

const char* validation_name(iiXml::Writer::ValidationExit result) {
    switch (result) {
        case iiXml::Writer::ValidationExit::Valid:
            return "valid";
        case iiXml::Writer::ValidationExit::InvalidXmlFile:
            return "invalid_xml_file";
        case iiXml::Writer::ValidationExit::InvalidTagClosure:
            return "invalid_tag_closure";
        case iiXml::Writer::ValidationExit::ExceptionThrown:
            return "exception_thrown";
    }

    return "unknown";
}

} // namespace

InputValidator::InputValidator(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<InputValidator::ValidationExit>(
        "iiXml::Writer::InputValidator::ValidationExit");
    qDebug() << "iiXml::Writer::InputValidator::InputValidator constructed";
}

iiXml::Writer::ValidationExit InputValidator::Validate(std::string_view input) const {
    qDebug() << "iiXml::Writer::InputValidator::Validate begin"
             << "input_size=" << input.size();
    iiXml::Logging::LogInputSummary("iiXml::Writer::InputValidator::Validate", input);
    const ValidationResult result = ValidateResult(input);
    if (result.Exit == iiXml::Writer::ValidationExit::Valid) {
        qDebug() << "iiXml::Writer::InputValidator::Validate valid";
    } else {
        qDebug() << "iiXml::Writer::InputValidator::Validate failed"
                 << "exit=" << validation_name(result.Exit)
                 << "reason=" << QString::fromStdString(result.Reason);
    }
    iiXml::Logging::LogOutputSummary(
        "iiXml::Writer::InputValidator::Validate",
        validation_name(result.Exit),
        result.Reason
    );
    return result.Exit;
}

ValidationResult InputValidator::ValidateResult(std::string_view input) const {
    qDebug() << "iiXml::Writer::InputValidator::ValidateResult begin"
             << "input_size=" << input.size();
    iiXml::Logging::LogInputSummary("iiXml::Writer::InputValidator::ValidateResult", input);
    try {
        const iiXml::Elements::Doctype doctype;
        const iiXml::Elements::DoctypeResult matched = doctype.MatchTopResult(input);
        if (!matched.Match.has_value()) {
            iiXml::Logging::LogParseFailure(
                "iiXml::Writer::InputValidator::ValidateResult",
                matched.Reason,
                input,
                0
            );
            return ValidationResult{iiXml::Writer::ValidationExit::InvalidXmlFile, matched.Reason};
        }

        const std::string_view body = body_after_doctype(input, *matched.Match);
        if (!HasValidTagClosure(body)) {
            iiXml::Logging::LogParseFailure(
                "iiXml::Writer::InputValidator::ValidateResult",
                "tag closure validation failed",
                body,
                0
            );
            return ValidationResult{
                iiXml::Writer::ValidationExit::InvalidTagClosure,
                "tag closure validation failed"
            };
        }

        qDebug() << "iiXml::Writer::InputValidator::ValidateResult valid";
        iiXml::Logging::LogOutputSummary(
            "iiXml::Writer::InputValidator::ValidateResult",
            validation_name(iiXml::Writer::ValidationExit::Valid),
            "valid XML input"
        );
        return ValidationResult{iiXml::Writer::ValidationExit::Valid, "valid XML input"};
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Writer::InputValidator::ValidateResult exception"
                 << "what=" << exception.what();
        return ValidationResult{
            iiXml::Writer::ValidationExit::ExceptionThrown,
            std::string("input validation exception: ") + exception.what()
        };
    } catch (...) {
        qDebug() << "iiXml::Writer::InputValidator::ValidateResult exception"
                 << "what=unknown";
        return ValidationResult{
            iiXml::Writer::ValidationExit::ExceptionThrown,
            "input validation exception: unknown"
        };
    }
}

bool InputValidator::HasValidTagClosure(std::string_view input) const {
    qDebug() << "iiXml::Writer::InputValidator::HasValidTagClosure begin"
             << "input_size=" << input.size();
    iiXml::Logging::LogInputSummary("iiXml::Writer::InputValidator::HasValidTagClosure", input);
    try {
        input = trim_outer(input);
        const iiXml::Elements::OpenTag open_tag;
        std::vector<std::string> open_tags;
        bool saw_element = false;

        for (std::size_t index = 0; index < input.size();) {
            const std::size_t tag_start = input.find('<', index);
            if (tag_start == std::string_view::npos) {
                break;
            }

            if (starts_with_at(input, tag_start, "<!--")) {
                const std::optional<std::size_t> end = find_token_end(input, tag_start + 4, "-->");
                if (!end.has_value()) {
                    iiXml::Logging::LogParseFailure(
                        "iiXml::Writer::InputValidator::HasValidTagClosure",
                        "unclosed comment",
                        input,
                        tag_start
                    );
                    return false;
                }
                index = *end;
                continue;
            }

            if (starts_with_at(input, tag_start, "<![CDATA[")) {
                const std::optional<std::size_t> end = find_token_end(input, tag_start + 9, "]]>");
                if (!end.has_value()) {
                    iiXml::Logging::LogParseFailure(
                        "iiXml::Writer::InputValidator::HasValidTagClosure",
                        "unclosed cdata",
                        input,
                        tag_start
                    );
                    return false;
                }
                index = *end;
                continue;
            }

            if (starts_with_at(input, tag_start, "<?")) {
                const std::optional<std::size_t> end = find_token_end(input, tag_start + 2, "?>");
                if (!end.has_value()) {
                    iiXml::Logging::LogParseFailure(
                        "iiXml::Writer::InputValidator::HasValidTagClosure",
                        "unclosed processing instruction",
                        input,
                        tag_start
                    );
                    return false;
                }
                index = *end;
                continue;
            }

            if (starts_with_at(input, tag_start, "<!Doctype")) {
                const iiXml::Elements::Doctype doctype;
                const iiXml::Elements::DoctypeResult matched = doctype.MatchTop(input.substr(tag_start));
                if (!matched.Match.has_value()) {
                    iiXml::Logging::LogParseFailure(
                        "iiXml::Writer::InputValidator::HasValidTagClosure",
                        "malformed nested doctype",
                        input,
                        tag_start
                    );
                    return false;
                }
                index = tag_start + matched.Match->Raw.size();
                continue;
            }

            const std::optional<std::size_t> tag_end = find_markup_end(input, tag_start + 1);
            if (!tag_end.has_value()) {
                iiXml::Logging::LogParseFailure(
                    "iiXml::Writer::InputValidator::HasValidTagClosure",
                    "unclosed tag markup",
                    input,
                    tag_start
                );
                return false;
            }

            const std::string_view markup = input.substr(tag_start, *tag_end - tag_start + 1);
            if (markup.size() < 3) {
                iiXml::Logging::LogParseFailure(
                    "iiXml::Writer::InputValidator::HasValidTagClosure",
                    "markup too short",
                    input,
                    tag_start
                );
                return false;
            }

            if (markup[1] == '/') {
                const std::optional<std::string> TagName = read_tag_name(markup, 2);
                if (!TagName.has_value()) {
                    iiXml::Logging::LogParseFailure(
                        "iiXml::Writer::InputValidator::HasValidTagClosure",
                        "invalid closing tag name",
                        input,
                        tag_start + 2
                    );
                    return false;
                }

                if (!open_tag.CloseOpenTag(open_tags, *TagName)) {
                    iiXml::Logging::LogParseFailure(
                        "iiXml::Writer::InputValidator::HasValidTagClosure",
                        "closing tag mismatch",
                        input,
                        tag_start
                    );
                    return false;
                }
                iiXml::Logging::LogParseEvent(
                    "iiXml::Writer::InputValidator::HasValidTagClosure",
                    std::string("close_tag name=") + *TagName,
                    input,
                    tag_start
                );
            } else {
                const std::optional<std::string> TagName = read_tag_name(markup, 1);
                if (!TagName.has_value()) {
                    iiXml::Logging::LogParseFailure(
                        "iiXml::Writer::InputValidator::HasValidTagClosure",
                        "invalid opening tag name",
                        input,
                        tag_start + 1
                    );
                    return false;
                }

                saw_element = true;
                if (!is_self_closing_markup(markup)) {
                    iiXml::Logging::LogParseEvent(
                        "iiXml::Writer::InputValidator::HasValidTagClosure",
                        std::string("open_tag name=") + *TagName,
                        input,
                        tag_start
                    );
                    open_tags.push_back(*TagName);
                } else {
                    iiXml::Logging::LogParseEvent(
                        "iiXml::Writer::InputValidator::HasValidTagClosure",
                        std::string("self_closing_tag name=") + *TagName,
                        input,
                        tag_start
                    );
                }
            }

            index = *tag_end + 1;
        }

        const bool result = saw_element && open_tags.empty();
        if (!result) {
            iiXml::Logging::LogParseFailure(
                "iiXml::Writer::InputValidator::HasValidTagClosure",
                saw_element ? "unclosed open tags" : "no element found",
                input,
                saw_element ? input.size() : 0
            );
            qDebug() << "iiXml::Writer::InputValidator::HasValidTagClosure failed"
                     << "result=" << result
                     << "remaining_open_tags=" << open_tags.size();
            iiXml::Logging::LogOutputSummary(
                "iiXml::Writer::InputValidator::HasValidTagClosure",
                "failed",
                std::string("remaining_open_tags=") + std::to_string(open_tags.size())
            );
            return false;
        }

        qDebug() << "iiXml::Writer::InputValidator::HasValidTagClosure valid"
                 << "result=" << result
                 << "remaining_open_tags=" << open_tags.size();
        iiXml::Logging::LogOutputSummary(
            "iiXml::Writer::InputValidator::HasValidTagClosure",
            "valid",
            std::string("remaining_open_tags=") + std::to_string(open_tags.size())
        );
        return result;
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Writer::InputValidator::HasValidTagClosure exception"
                 << "what=" << exception.what();
        return false;
    } catch (...) {
        qDebug() << "iiXml::Writer::InputValidator::HasValidTagClosure exception"
                 << "what=unknown";
        return false;
    }
}

void InputValidator::ValidateInput(const QString& input) {
    qDebug() << "iiXml::Writer::InputValidator::ValidateInput begin"
             << "input_size=" << input.size();
    try {
        const QByteArray utf8 = input.toUtf8();
        const std::string bytes(utf8.constData(), static_cast<std::size_t>(utf8.size()));
        iiXml::Logging::LogInputSummary(
            "iiXml::Writer::InputValidator::ValidateInput",
            std::string_view(bytes.data(), bytes.size())
        );
        const ValidationResult detailed = ValidateResult(std::string_view(bytes.data(), bytes.size()));
        const InputValidator::ValidationExit result = to_qt_exit(detailed.Exit);

        qDebug() << "iiXml::Writer::InputValidator::ValidateInput"
                 << (detailed.Exit == iiXml::Writer::ValidationExit::Valid ? "valid" : "failed")
                 << "exit=" << validation_name(detailed.Exit);
        iiXml::Logging::LogOutputSummary(
            "iiXml::Writer::InputValidator::ValidateInput",
            validation_name(detailed.Exit),
            detailed.Reason
        );
        emit ValidationFinished(result);

        switch (result) {
            case InputValidator::ValidationExit::Valid:
                emit ValidXml();
                return;
            case InputValidator::ValidationExit::InvalidXmlFile:
                emit ValidationFailed(result, QString::fromStdString(detailed.Reason));
                emit InvalidXmlFile();
                return;
            case InputValidator::ValidationExit::InvalidTagClosure:
                emit ValidationFailed(result, QString::fromStdString(detailed.Reason));
                emit InvalidTagClosure();
                return;
            case InputValidator::ValidationExit::ExceptionThrown:
                emit ValidationFailed(result, QString::fromStdString(detailed.Reason));
                emit ExceptionThrown();
                return;
        }
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Writer::InputValidator::ValidateInput exception"
                 << "what=" << exception.what();
        const InputValidator::ValidationExit result = InputValidator::ValidationExit::ExceptionThrown;
        emit ValidationFinished(result);
        emit ValidationFailed(result, QString::fromStdString(
            std::string("input validation slot exception: ") + exception.what()
        ));
        emit ExceptionThrown();
    } catch (...) {
        qDebug() << "iiXml::Writer::InputValidator::ValidateInput exception"
                 << "what=unknown";
        const InputValidator::ValidationExit result = InputValidator::ValidationExit::ExceptionThrown;
        emit ValidationFinished(result);
        emit ValidationFailed(result, "input validation slot exception: unknown");
        emit ExceptionThrown();
    }
}

} // namespace iiXml::Writer
