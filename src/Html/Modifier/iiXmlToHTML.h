#pragma once

#include <QObject>
#include <QString>

#include <string>
#include <string_view>

namespace iiHtmlBlock {

class iiXmlToHTML : public QObject {
    Q_OBJECT

public:
    explicit iiXmlToHTML(QObject* parent = nullptr);

    [[nodiscard]] bool Convert(const char* xml);
    [[nodiscard]] bool Convert(std::string_view xml);
    [[nodiscard]] bool Convert(const QString& xml);

    [[nodiscard]] const std::string& GetHTMLText() const;
    [[nodiscard]] const std::string& GetError() const;

public slots:
    void ConvertXml(const QString& xml);

signals:
    void Converted(const QString& html);
    void ConvertFailed(const QString& reason);

private:
    void Clear();

    std::string html_;
    std::string error_;
};

} // namespace iiHtmlBlock
