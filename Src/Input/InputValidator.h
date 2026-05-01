#pragma once

#include <QObject>
#include <QString>

#include <string>
#include <string_view>

namespace iiXml::Writer {

enum class ValidationExit {
    Valid,
    InvalidXmlFile,
    InvalidTagClosure,
    ExceptionThrown
};

struct ValidationResult {
    ValidationExit Exit;
    std::string Reason;
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

    [[nodiscard]] iiXml::Writer::ValidationExit Validate(std::string_view Input) const;
    [[nodiscard]] ValidationResult ValidateResult(std::string_view Input) const;
    [[nodiscard]] bool HasValidTagClosure(std::string_view Input) const;

public slots:
    void ValidateInput(const QString& Input);

signals:
    void ValidationFinished(iiXml::Writer::InputValidator::ValidationExit Result);
    void ValidationFailed(iiXml::Writer::InputValidator::ValidationExit Result, const QString& Reason);
    void ValidXml();
    void InvalidXmlFile();
    void InvalidTagClosure();
    void ExceptionThrown();
};

} // namespace iiXml::Writer

Q_DECLARE_METATYPE(iiXml::Writer::InputValidator::ValidationExit)
