#ifndef IIXML_PARSER_TAG_PARSER_H
#define IIXML_PARSER_TAG_PARSER_H

#include "Src/Elements/InlineProperties.h"

#include <QObject>
#include <QString>

#include <cstddef>
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

struct tag_range {
    std::string tag_name;
    std::size_t raw_begin;
    std::size_t value_begin;
    std::size_t value_end;
    std::size_t raw_end;
};

struct tag_field {
    std::string name;
    std::size_t name_begin;
    std::size_t name_end;
    bool has_value;
    std::size_t value_begin;
    std::size_t value_end;
    iiXml::elements::inline_property_type value_type;
    bool type_declared;
};

struct tag_node {
    tag_range range;
    std::vector<tag_field> fields;
    std::vector<tag_node> children;
};

class tag_parser : public QObject {
    Q_OBJECT

public:
    explicit tag_parser(QObject* parent = nullptr);

    [[nodiscard]] std::optional<tag_value> parse(std::string_view input) const;
    [[nodiscard]] std::optional<std::vector<tag_node>> parse_all(std::string_view input) const;

public slots:
    void parseTag(const QString& input);

signals:
    void tagParsed(const QString& tag_name, const QString& value);
    void parseFailed(const QString& reason);
};

} // namespace iiXml::parser

#endif // IIXML_PARSER_TAG_PARSER_H
