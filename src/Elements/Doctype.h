#pragma once

#include <QObject>
#include <QString>

#include <optional>
#include <string>
#include <string_view>

namespace iiXml::Elements {

enum class DoctypeKind {
    XmlDeclaration,
    DoctypeDeclaration
};

enum class DoctypeStatus {
    Matched,
    EmptyInput,
    NoTopDeclaration,
    MalformedXmlDeclaration,
    MalformedDoctypeDeclaration,
    ExceptionThrown
};

struct DoctypeMatch {
    DoctypeKind Kind;
    std::string Raw;
};

struct DoctypeResult {
    DoctypeStatus Status;
    std::optional<DoctypeMatch> Match;
    std::string Reason;
};

class Doctype : public QObject {
    Q_OBJECT

public:
    enum class Kind {
        XmlDeclaration,
        DoctypeDeclaration
    };
    Q_ENUM(Kind)

    explicit Doctype(QObject* parent = nullptr);

    [[nodiscard]] DoctypeResult MatchTop(std::string_view Input) const;
    [[nodiscard]] DoctypeResult MatchTopResult(std::string_view Input) const;
    [[nodiscard]] std::optional<DoctypeMatch> MatchTopMatch(std::string_view Input) const;
    [[nodiscard]] bool IsTopDoctype(std::string_view Input) const;
    [[nodiscard]] bool IsTopXmlDeclaration(std::string_view Input) const;

public slots:
    void MatchTopInput(const QString& Input);

signals:
    void DoctypeMatched(iiXml::Elements::Doctype::Kind Kind, const QString& Raw);
    void DoctypeRejected(const QString& Reason);
    void XmlDeclarationMatched(const QString& Raw);
    void DoctypeDeclarationMatched(const QString& Raw);
};

} // namespace iiXml::Elements

Q_DECLARE_METATYPE(iiXml::Elements::Doctype::Kind)
