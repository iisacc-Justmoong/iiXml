#pragma once

#include <QObject>
#include <QString>

#include <string>
#include <string_view>

namespace iiHtmlBlock {

class GetHTML : public QObject {
    Q_OBJECT

public:
    explicit GetHTML(QObject* parent = nullptr);

    [[nodiscard]] bool Parse(const char* xml);
    [[nodiscard]] bool Parse(std::string_view xml);
    [[nodiscard]] bool Parse(const QString& xml);

    [[nodiscard]] const std::string& GetHTMLText() const;
    [[nodiscard]] const std::string& GetTagName() const;
    [[nodiscard]] const std::string& GetSourceTagName() const;
    [[nodiscard]] const std::string& GetError() const;

public slots:
    void ParseXml(const QString& xml);

signals:
    void Parsed(const QString& html);
    void ParseFailed(const QString& reason);

private:
    void Clear();

    std::string html_;
    std::string tag_name_;
    std::string source_tag_name_;
    std::string error_;
};

} // namespace iiHtmlBlock
