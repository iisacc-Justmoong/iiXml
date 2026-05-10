#ifndef IIXML_MUTATION_INLINE_TAG_MUTATION_H
#define IIXML_MUTATION_INLINE_TAG_MUTATION_H

#include <QObject>
#include <QString>
#include <QStringList>

namespace iiXml::Mutation {

struct InlineTagMutationOptions {
    QStringList OrderedInlineTagNames;
    QStringList LogicalBreakTagNames;
};

struct InlineTagMutationResult {
    bool Applied = false;
    bool Valid = false;
    int LogicalLength = 0;
    QString SourceText;
    QString ErrorReason;
};

class InlineTagMutation : public QObject {
    Q_OBJECT

public:
    explicit InlineTagMutation(QObject* parent = nullptr);

    [[nodiscard]] InlineTagMutationResult ApplyLogicalStyleRange(
        const QString& sourceText,
        const QString& inlineTagName,
        int logicalStart,
        int logicalEnd,
        const InlineTagMutationOptions& options,
        bool active = true) const;

    [[nodiscard]] int LogicalOffsetForSourceBoundary(
        const QString& sourceText,
        int sourceOffset,
        const InlineTagMutationOptions& options) const;

    [[nodiscard]] int SourceBoundaryForLogicalOffset(
        const QString& sourceText,
        int logicalOffset,
        const InlineTagMutationOptions& options) const;

    [[nodiscard]] bool CanParseFragment(const QString& sourceText) const;
};

} // namespace iiXml::Mutation

#endif // IIXML_MUTATION_INLINE_TAG_MUTATION_H
