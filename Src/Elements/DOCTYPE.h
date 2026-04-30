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

enum class doctype_status {
    matched,
    empty_input,
    no_top_declaration,
    malformed_xml_declaration,
    malformed_doctype_declaration,
    exception_thrown
};

struct doctype_match {
    doctype_kind kind;
    std::string raw;
};

struct doctype_result {
    doctype_status status;
    std::optional<doctype_match> match;
    std::string reason;
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

    [[nodiscard]] doctype_result match_top(std::string_view input) const;
    [[nodiscard]] doctype_result match_top_result(std::string_view input) const;
    [[nodiscard]] std::optional<doctype_match> match_top_match(std::string_view input) const;
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
