#pragma once

#include <QObject>
#include <QString>

#include <string>
#include <string_view>

namespace iiXml::writer {

enum class validation_exit {
    valid,
    invalid_xml_file,
    invalid_tag_closure,
    exception_thrown
};

struct validation_result {
    validation_exit exit;
    std::string reason;
};

class InputValidator : public QObject {
    Q_OBJECT

public:
    enum class ValidationExit {
        Valid,
        InvalidXmlFile,
        InvalidTagClosure,
        ExceptionThrown
    };
    Q_ENUM(ValidationExit)

    explicit InputValidator(QObject* parent = nullptr);

    [[nodiscard]] validation_exit validate(std::string_view input) const;
    [[nodiscard]] validation_result validate_result(std::string_view input) const;
    [[nodiscard]] bool has_valid_tag_closure(std::string_view input) const;

public slots:
    void validateInput(const QString& input);

signals:
    void validationFinished(iiXml::writer::InputValidator::ValidationExit result);
    void validationFailed(iiXml::writer::InputValidator::ValidationExit result, const QString& reason);
    void validXml();
    void invalidXmlFile();
    void invalidTagClosure();
    void exceptionThrown();
};

} // namespace iiXml::writer

Q_DECLARE_METATYPE(iiXml::writer::InputValidator::ValidationExit)
