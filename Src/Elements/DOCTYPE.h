#pragma once

#include <QObject>
#include <QString>

#include <optional>
#include <string>
#include <string_view>

namespace iiXml::elements {

enum class doctype_kind {
    xml_declaration,
    doctype_declaration
};

struct doctype_match {
    doctype_kind kind;
    std::string raw;
};

class DOCTYPE : public QObject {
    Q_OBJECT

public:
    enum class Kind {
        XmlDeclaration,
        DoctypeDeclaration
    };
    Q_ENUM(Kind)

    explicit DOCTYPE(QObject* parent = nullptr);

    [[nodiscard]] std::optional<doctype_match> match_top(std::string_view input) const;
    [[nodiscard]] bool is_top_doctype(std::string_view input) const;
    [[nodiscard]] bool is_top_xml_declaration(std::string_view input) const;

public slots:
    void matchTop(const QString& input);

signals:
    void doctypeMatched(iiXml::elements::DOCTYPE::Kind kind, const QString& raw);
    void doctypeRejected(const QString& reason);
    void xmlDeclarationMatched(const QString& raw);
    void doctypeDeclarationMatched(const QString& raw);
};

} // namespace iiXml::elements

Q_DECLARE_METATYPE(iiXml::elements::DOCTYPE::Kind)
