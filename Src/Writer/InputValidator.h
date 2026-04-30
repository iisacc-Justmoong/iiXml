#pragma once

#include <QObject>
#include <QString>

#include <string_view>

namespace iiXml::writer {

enum class validation_exit {
    valid,
    invalid_xml_file,
    invalid_tag_closure
};

class InputValidator : public QObject {
    Q_OBJECT

public:
    enum class ValidationExit {
        Valid,
        InvalidXmlFile,
        InvalidTagClosure
    };
    Q_ENUM(ValidationExit)

    explicit InputValidator(QObject* parent = nullptr);

    [[nodiscard]] validation_exit validate(std::string_view input) const;
    [[nodiscard]] bool has_valid_tag_closure(std::string_view input) const;

public slots:
    void validateInput(const QString& input);

signals:
    void validationFinished(iiXml::writer::InputValidator::ValidationExit result);
    void validXml();
    void invalidXmlFile();
    void invalidTagClosure();
};

} // namespace iiXml::writer

Q_DECLARE_METATYPE(iiXml::writer::InputValidator::ValidationExit)
