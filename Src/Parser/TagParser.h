#ifndef IIXML_PARSER_TAG_PARSER_H
#define IIXML_PARSER_TAG_PARSER_H

#include <QObject>
#include <QString>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iiXml::parser {

struct tag_value {
    std::string tag_name;
    std::string value;
    std::string raw;
};

class tag_parser : public QObject {
    Q_OBJECT

public:
    explicit tag_parser(QObject* parent = nullptr);

    [[nodiscard]] std::optional<tag_value> parse(std::string_view input) const;
    [[nodiscard]] std::optional<std::vector<tag_value>> parse_all(std::string_view input) const;

public slots:
    void parseTag(const QString& input);

signals:
    void tagParsed(const QString& tag_name, const QString& value);
    void parseFailed(const QString& reason);
};

} // namespace iiXml::parser

#endif // IIXML_PARSER_TAG_PARSER_H
