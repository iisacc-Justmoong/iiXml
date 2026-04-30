#include "InputValidator.h"

#include "Src/Elements/DOCTYPE.h"
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

std::string_view body_after_doctype(std::string_view input, const iiXml::elements::doctype_match& matched) {
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

namespace iiXml::writer {

namespace {

InputValidator::ValidationExit to_qt_exit(validation_exit result) {
    switch (result) {
        case validation_exit::valid:
            return InputValidator::ValidationExit::Valid;
        case validation_exit::invalid_xml_file:
            return InputValidator::ValidationExit::InvalidXmlFile;
        case validation_exit::invalid_tag_closure:
            return InputValidator::ValidationExit::InvalidTagClosure;
        case validation_exit::exception_thrown:
            return InputValidator::ValidationExit::ExceptionThrown;
    }

    return InputValidator::ValidationExit::ExceptionThrown;
}

const char* validation_name(validation_exit result) {
    switch (result) {
        case validation_exit::valid:
            return "valid";
        case validation_exit::invalid_xml_file:
            return "invalid_xml_file";
        case validation_exit::invalid_tag_closure:
            return "invalid_tag_closure";
        case validation_exit::exception_thrown:
            return "exception_thrown";
    }

    return "unknown";
}

} // namespace

InputValidator::InputValidator(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<InputValidator::ValidationExit>(
        "iiXml::writer::InputValidator::ValidationExit");
    qDebug() << "iiXml::writer::InputValidator::InputValidator constructed";
}

validation_exit InputValidator::validate(std::string_view input) const {
    qDebug() << "iiXml::writer::InputValidator::validate begin"
             << "input_size=" << input.size();
    const validation_result result = validate_result(input);
    if (result.exit == validation_exit::valid) {
        qDebug() << "iiXml::writer::InputValidator::validate valid";
    } else {
        qDebug() << "iiXml::writer::InputValidator::validate failed"
                 << "exit=" << validation_name(result.exit)
                 << "reason=" << QString::fromStdString(result.reason);
    }
    return result.exit;
}

validation_result InputValidator::validate_result(std::string_view input) const {
    qDebug() << "iiXml::writer::InputValidator::validate_result begin"
             << "input_size=" << input.size();
    try {
        const iiXml::elements::DOCTYPE doctype;
        const iiXml::elements::doctype_result matched = doctype.match_top_result(input);
        if (!matched.match.has_value()) {
            qDebug() << "iiXml::writer::InputValidator::validate_result failed"
                     << "exit=" << validation_name(validation_exit::invalid_xml_file)
                     << "reason=" << QString::fromStdString(matched.reason);
            return validation_result{validation_exit::invalid_xml_file, matched.reason};
        }

        if (!has_valid_tag_closure(body_after_doctype(input, *matched.match))) {
            qDebug() << "iiXml::writer::InputValidator::validate_result failed"
                     << "exit=" << validation_name(validation_exit::invalid_tag_closure);
            return validation_result{
                validation_exit::invalid_tag_closure,
                "tag closure validation failed"
            };
        }

        qDebug() << "iiXml::writer::InputValidator::validate_result valid";
        return validation_result{validation_exit::valid, "valid XML input"};
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::writer::InputValidator::validate_result exception"
                 << "what=" << exception.what();
        return validation_result{
            validation_exit::exception_thrown,
            std::string("input validation exception: ") + exception.what()
        };
    } catch (...) {
        qDebug() << "iiXml::writer::InputValidator::validate_result exception"
                 << "what=unknown";
        return validation_result{
            validation_exit::exception_thrown,
            "input validation exception: unknown"
        };
    }
}

bool InputValidator::has_valid_tag_closure(std::string_view input) const {
    qDebug() << "iiXml::writer::InputValidator::has_valid_tag_closure begin"
             << "input_size=" << input.size();
    try {
        input = trim_outer(input);
        const iiXml::elements::OpenTag open_tag;
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
                    qDebug() << "iiXml::writer::InputValidator::has_valid_tag_closure failed"
                             << "reason=unclosed comment";
                    return false;
                }
                index = *end;
                continue;
            }

            if (starts_with_at(input, tag_start, "<![CDATA[")) {
                const std::optional<std::size_t> end = find_token_end(input, tag_start + 9, "]]>");
                if (!end.has_value()) {
                    qDebug() << "iiXml::writer::InputValidator::has_valid_tag_closure failed"
                             << "reason=unclosed cdata";
                    return false;
                }
                index = *end;
                continue;
            }

            if (starts_with_at(input, tag_start, "<?")) {
                const std::optional<std::size_t> end = find_token_end(input, tag_start + 2, "?>");
                if (!end.has_value()) {
                    qDebug() << "iiXml::writer::InputValidator::has_valid_tag_closure failed"
                             << "reason=unclosed processing instruction";
                    return false;
                }
                index = *end;
                continue;
            }

            if (starts_with_at(input, tag_start, "<!DOCTYPE")) {
                const iiXml::elements::DOCTYPE doctype;
                const iiXml::elements::doctype_result matched = doctype.match_top(input.substr(tag_start));
                if (!matched.match.has_value()) {
                    qDebug() << "iiXml::writer::InputValidator::has_valid_tag_closure failed"
                             << "reason=malformed nested doctype";
                    return false;
                }
                index = tag_start + matched.match->raw.size();
                continue;
            }

            const std::optional<std::size_t> tag_end = find_markup_end(input, tag_start + 1);
            if (!tag_end.has_value()) {
                qDebug() << "iiXml::writer::InputValidator::has_valid_tag_closure failed"
                         << "reason=unclosed tag markup";
                return false;
            }

            const std::string_view markup = input.substr(tag_start, *tag_end - tag_start + 1);
            if (markup.size() < 3) {
                qDebug() << "iiXml::writer::InputValidator::has_valid_tag_closure failed"
                         << "reason=markup too short";
                return false;
            }

            if (markup[1] == '/') {
                const std::optional<std::string> tag_name = read_tag_name(markup, 2);
                if (!tag_name.has_value() || !open_tag.close_open_tag(open_tags, *tag_name)) {
                    qDebug() << "iiXml::writer::InputValidator::has_valid_tag_closure failed"
                             << "reason=closing tag mismatch";
                    return false;
                }
            } else {
                const std::optional<std::string> tag_name = read_tag_name(markup, 1);
                if (!tag_name.has_value()) {
                    qDebug() << "iiXml::writer::InputValidator::has_valid_tag_closure failed"
                             << "reason=invalid opening tag name";
                    return false;
                }

                saw_element = true;
                if (!is_self_closing_markup(markup)) {
                    open_tags.push_back(*tag_name);
                }
            }

            index = *tag_end + 1;
        }

        const bool result = saw_element && open_tags.empty();
        qDebug() << "iiXml::writer::InputValidator::has_valid_tag_closure"
                 << (result ? "valid" : "failed")
                 << "result=" << result
                 << "remaining_open_tags=" << open_tags.size();
        return result;
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::writer::InputValidator::has_valid_tag_closure exception"
                 << "what=" << exception.what();
        return false;
    } catch (...) {
        qDebug() << "iiXml::writer::InputValidator::has_valid_tag_closure exception"
                 << "what=unknown";
        return false;
    }
}

void InputValidator::validateInput(const QString& input) {
    qDebug() << "iiXml::writer::InputValidator::validateInput begin"
             << "input_size=" << input.size();
    try {
        const QByteArray utf8 = input.toUtf8();
        const std::string bytes(utf8.constData(), static_cast<std::size_t>(utf8.size()));
        const validation_result detailed = validate_result(std::string_view(bytes.data(), bytes.size()));
        const ValidationExit result = to_qt_exit(detailed.exit);

        qDebug() << "iiXml::writer::InputValidator::validateInput"
                 << (detailed.exit == validation_exit::valid ? "valid" : "failed")
                 << "exit=" << validation_name(detailed.exit);
        emit validationFinished(result);

        switch (result) {
            case ValidationExit::Valid:
                emit validXml();
                return;
            case ValidationExit::InvalidXmlFile:
                emit validationFailed(result, QString::fromStdString(detailed.reason));
                emit invalidXmlFile();
                return;
            case ValidationExit::InvalidTagClosure:
                emit validationFailed(result, QString::fromStdString(detailed.reason));
                emit invalidTagClosure();
                return;
            case ValidationExit::ExceptionThrown:
                emit validationFailed(result, QString::fromStdString(detailed.reason));
                emit exceptionThrown();
                return;
        }
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::writer::InputValidator::validateInput exception"
                 << "what=" << exception.what();
        const ValidationExit result = ValidationExit::ExceptionThrown;
        emit validationFinished(result);
        emit validationFailed(result, QString::fromStdString(
            std::string("input validation slot exception: ") + exception.what()
        ));
        emit exceptionThrown();
    } catch (...) {
        qDebug() << "iiXml::writer::InputValidator::validateInput exception"
                 << "what=unknown";
        const ValidationExit result = ValidationExit::ExceptionThrown;
        emit validationFinished(result);
        emit validationFailed(result, "input validation slot exception: unknown");
        emit exceptionThrown();
    }
}

} // namespace iiXml::writer
