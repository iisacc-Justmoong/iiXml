#include "InlineTagMutation.h"

#include "Src/Parser/TagParser.h"

#include <QByteArray>
#include <QDebug>
#include <QStringView>
#include <QVector>

#include <algorithm>

namespace {

int bounded_offset(const int offset, const int sourceLength)
{
    return std::clamp(offset, 0, std::max(0, sourceLength));
}

QString normalized_tag_name(const QString& rawTagName)
{
    return rawTagName.trimmed().toCaseFolded();
}

QString source_tag_name(QStringView tagToken)
{
    if (tagToken.size() < 3 || tagToken.front() != QLatin1Char('<'))
        return {};

    int cursor = 1;
    while (cursor < tagToken.size() && tagToken.at(cursor).isSpace())
        ++cursor;
    if (cursor < tagToken.size() && tagToken.at(cursor) == QLatin1Char('/'))
        ++cursor;
    while (cursor < tagToken.size() && tagToken.at(cursor).isSpace())
        ++cursor;

    const int nameStart = cursor;
    while (cursor < tagToken.size())
    {
        const QChar ch = tagToken.at(cursor);
        if (!(ch.isLetterOrNumber()
              || ch == QLatin1Char('_')
              || ch == QLatin1Char('.')
              || ch == QLatin1Char(':')
              || ch == QLatin1Char('-')))
        {
            break;
        }
        ++cursor;
    }

    if (cursor <= nameStart)
        return {};

    return tagToken.mid(nameStart, cursor - nameStart).toString().trimmed().toCaseFolded();
}

bool source_tag_is_closing(QStringView tagToken)
{
    if (tagToken.size() < 3 || tagToken.front() != QLatin1Char('<'))
        return false;

    int cursor = 1;
    while (cursor < tagToken.size() && tagToken.at(cursor).isSpace())
        ++cursor;
    return cursor < tagToken.size() && tagToken.at(cursor) == QLatin1Char('/');
}

bool source_tag_is_self_closing(QStringView tagToken)
{
    if (tagToken.size() < 3 || tagToken.back() != QLatin1Char('>'))
        return false;

    int cursor = tagToken.size() - 2;
    while (cursor >= 0 && tagToken.at(cursor).isSpace())
        --cursor;
    return cursor >= 0 && tagToken.at(cursor) == QLatin1Char('/');
}

int html_entity_length_at(const QString& text, const int sourceOffset)
{
    if (sourceOffset < 0
        || sourceOffset >= text.size()
        || text.at(sourceOffset) != QLatin1Char('&'))
    {
        return 0;
    }

    const int semicolonOffset = text.indexOf(QLatin1Char(';'), sourceOffset + 1);
    if (semicolonOffset <= sourceOffset)
        return 0;

    const QString entityToken = text.mid(sourceOffset, semicolonOffset - sourceOffset + 1).toCaseFolded();
    if (entityToken == QStringLiteral("&amp;")
        || entityToken == QStringLiteral("&lt;")
        || entityToken == QStringLiteral("&gt;")
        || entityToken == QStringLiteral("&quot;")
        || entityToken == QStringLiteral("&apos;")
        || entityToken == QStringLiteral("&#39;")
        || entityToken == QStringLiteral("&nbsp;"))
    {
        return entityToken.size();
    }

    if (entityToken.startsWith(QStringLiteral("&#x"))
        || entityToken.startsWith(QStringLiteral("&#")))
    {
        return entityToken.size();
    }

    return 0;
}

int inline_style_index_for_tag(const QString& rawTagName, const iiXml::Mutation::InlineTagMutationOptions& options)
{
    const QString normalizedTag = normalized_tag_name(rawTagName);
    for (int index = 0; index < options.OrderedInlineTagNames.size(); ++index)
    {
        if (normalized_tag_name(options.OrderedInlineTagNames.at(index)) == normalizedTag)
            return index;
    }
    return -1;
}

QString open_tag_for_index(const int index, const iiXml::Mutation::InlineTagMutationOptions& options)
{
    if (index < 0 || index >= options.OrderedInlineTagNames.size())
        return {};
    return QStringLiteral("<") + normalized_tag_name(options.OrderedInlineTagNames.at(index)) + QStringLiteral(">");
}

QString close_tag_for_index(const int index, const iiXml::Mutation::InlineTagMutationOptions& options)
{
    if (index < 0 || index >= options.OrderedInlineTagNames.size())
        return {};
    return QStringLiteral("</") + normalized_tag_name(options.OrderedInlineTagNames.at(index)) + QStringLiteral(">");
}

bool is_logical_break_tag_name(const QString& rawTagName, const iiXml::Mutation::InlineTagMutationOptions& options)
{
    const QString normalizedTag = normalized_tag_name(rawTagName);
    for (const QString& breakTag : options.LogicalBreakTagNames)
    {
        if (normalized_tag_name(breakTag) == normalizedTag)
            return true;
    }
    return false;
}

struct inline_style_state final
{
    int LogicalLength = 0;
    QVector<QVector<bool>> Coverage;
};

inline_style_state build_inline_style_state(
    const QString& sourceText,
    const iiXml::Mutation::InlineTagMutationOptions& options)
{
    inline_style_state state;
    state.Coverage.resize(options.OrderedInlineTagNames.size());
    QVector<int> styleDepths(options.OrderedInlineTagNames.size(), 0);

    auto appendCoverageEntry = [&state, &styleDepths]() {
        for (int index = 0; index < state.Coverage.size(); ++index)
            state.Coverage[index].push_back(styleDepths.at(index) > 0);
        state.LogicalLength += 1;
    };

    int sourceOffset = 0;
    while (sourceOffset < sourceText.size())
    {
        if (sourceText.at(sourceOffset) == QLatin1Char('<'))
        {
            const int tagEnd = sourceText.indexOf(QLatin1Char('>'), sourceOffset + 1);
            if (tagEnd > sourceOffset)
            {
                const QStringView tagToken(sourceText.constData() + sourceOffset, tagEnd - sourceOffset + 1);
                const QString normalizedTagName = source_tag_name(tagToken);
                const bool closingTag = source_tag_is_closing(tagToken);
                const bool selfClosingTag = source_tag_is_self_closing(tagToken);
                const int styleIndex = inline_style_index_for_tag(normalizedTagName, options);
                if (styleIndex >= 0 && !selfClosingTag)
                {
                    int& styleDepth = styleDepths[styleIndex];
                    if (closingTag)
                        styleDepth = std::max(0, styleDepth - 1);
                    else
                        styleDepth += 1;
                    sourceOffset = tagEnd + 1;
                    continue;
                }
                if (is_logical_break_tag_name(normalizedTagName, options))
                {
                    appendCoverageEntry();
                    sourceOffset = tagEnd + 1;
                    continue;
                }
                sourceOffset = tagEnd + 1;
                continue;
            }
        }

        const int entityLength = html_entity_length_at(sourceText, sourceOffset);
        if (entityLength > 0)
        {
            appendCoverageEntry();
            sourceOffset += entityLength;
            continue;
        }

        appendCoverageEntry();
        sourceOffset += 1;
    }

    return state;
}

void apply_coverage_range(
    QVector<bool>* styleCoverage,
    const int selectionStart,
    const int selectionEnd,
    const bool nextActive)
{
    if (styleCoverage == nullptr || selectionEnd <= selectionStart)
        return;

    const int boundedStart = std::max(0, selectionStart);
    const int boundedEnd = std::min(selectionEnd, static_cast<int>(styleCoverage->size()));
    for (int logicalOffset = boundedStart; logicalOffset < boundedEnd; ++logicalOffset)
        (*styleCoverage)[logicalOffset] = nextActive;
}

void synchronize_output_inline_style_state(
    QString* output,
    QVector<int>* emittedStyleOrder,
    const QVector<QVector<bool>>& desiredCoverage,
    const iiXml::Mutation::InlineTagMutationOptions& options,
    const int logicalOffset)
{
    if (output == nullptr || emittedStyleOrder == nullptr)
        return;

    QVector<int> targetStyleOrder;
    targetStyleOrder.reserve(desiredCoverage.size());
    for (int index = 0; index < desiredCoverage.size(); ++index)
    {
        const QVector<bool>& coverage = desiredCoverage.at(index);
        const bool shouldBeActive = logicalOffset >= 0
            && logicalOffset < coverage.size()
            && coverage.at(logicalOffset);
        if (shouldBeActive)
            targetStyleOrder.push_back(index);
    }

    int commonPrefixLength = 0;
    while (commonPrefixLength < emittedStyleOrder->size()
           && commonPrefixLength < targetStyleOrder.size()
           && emittedStyleOrder->at(commonPrefixLength) == targetStyleOrder.at(commonPrefixLength))
    {
        ++commonPrefixLength;
    }

    for (int index = emittedStyleOrder->size() - 1; index >= commonPrefixLength; --index)
    {
        *output += close_tag_for_index(emittedStyleOrder->at(index), options);
        emittedStyleOrder->removeAt(index);
    }

    for (int index = commonPrefixLength; index < targetStyleOrder.size(); ++index)
    {
        const int styleIndex = targetStyleOrder.at(index);
        *output += open_tag_for_index(styleIndex, options);
        emittedStyleOrder->push_back(styleIndex);
    }
}

QString rebuild_source_text_with_inline_style_coverage(
    const QString& sourceText,
    const QVector<QVector<bool>>& desiredCoverage,
    const iiXml::Mutation::InlineTagMutationOptions& options,
    const int logicalLength)
{
    QString output;
    output.reserve(sourceText.size() + 32);

    QVector<int> emittedStyleOrder;
    emittedStyleOrder.reserve(options.OrderedInlineTagNames.size());

    int logicalOffset = 0;
    int sourceOffset = 0;
    while (sourceOffset < sourceText.size())
    {
        if (sourceText.at(sourceOffset) == QLatin1Char('<'))
        {
            const int tagEnd = sourceText.indexOf(QLatin1Char('>'), sourceOffset + 1);
            if (tagEnd > sourceOffset)
            {
                const QStringView tagToken(sourceText.constData() + sourceOffset, tagEnd - sourceOffset + 1);
                const QString normalizedTagName = source_tag_name(tagToken);
                const bool selfClosingTag = source_tag_is_self_closing(tagToken);
                const int styleIndex = inline_style_index_for_tag(normalizedTagName, options);
                if (styleIndex >= 0 && !selfClosingTag)
                {
                    sourceOffset = tagEnd + 1;
                    continue;
                }

                synchronize_output_inline_style_state(
                    &output,
                    &emittedStyleOrder,
                    desiredCoverage,
                    options,
                    logicalOffset);
                output += sourceText.mid(sourceOffset, tagEnd - sourceOffset + 1);
                sourceOffset = tagEnd + 1;
                if (is_logical_break_tag_name(normalizedTagName, options))
                    logicalOffset += 1;
                continue;
            }
        }

        synchronize_output_inline_style_state(
            &output,
            &emittedStyleOrder,
            desiredCoverage,
            options,
            logicalOffset);
        const int entityLength = html_entity_length_at(sourceText, sourceOffset);
        if (entityLength > 0)
        {
            output += sourceText.mid(sourceOffset, entityLength);
            sourceOffset += entityLength;
            logicalOffset += 1;
            continue;
        }

        output += sourceText.at(sourceOffset);
        sourceOffset += 1;
        logicalOffset += 1;
    }

    synchronize_output_inline_style_state(
        &output,
        &emittedStyleOrder,
        desiredCoverage,
        options,
        logicalLength);
    return output;
}

QString validation_fragment_for_source(const QString& sourceText)
{
    return QStringLiteral("<root>") + sourceText + QStringLiteral("</root>");
}

} // namespace

namespace iiXml::Mutation {

InlineTagMutation::InlineTagMutation(QObject* parent)
    : QObject(parent)
{
    qDebug().noquote() << "iiXml::Mutation::InlineTagMutation::InlineTagMutation constructed";
}

InlineTagMutationResult InlineTagMutation::ApplyLogicalStyleRange(
    const QString& sourceText,
    const QString& inlineTagName,
    int logicalStart,
    int logicalEnd,
    const InlineTagMutationOptions& options,
    const bool active) const
{
    qDebug().noquote() << "iiXml::Mutation::InlineTagMutation::ApplyLogicalStyleRange begin"
                       << "source_size=" << sourceText.size()
                       << "tag_name=" << inlineTagName
                       << "logical_start=" << logicalStart
                       << "logical_end=" << logicalEnd;

    const int styleIndex = inline_style_index_for_tag(inlineTagName, options);
    if (styleIndex < 0)
    {
        qDebug().noquote() << "iiXml::Mutation::InlineTagMutation::ApplyLogicalStyleRange failed"
                           << "reason=unsupported inline tag";
        return {false, false, 0, sourceText, QStringLiteral("unsupported-inline-tag")};
    }

    const inline_style_state state = build_inline_style_state(sourceText, options);
    logicalStart = std::clamp(logicalStart, 0, state.LogicalLength);
    logicalEnd = std::clamp(logicalEnd, logicalStart, state.LogicalLength);

    QVector<QVector<bool>> desiredCoverage = state.Coverage;
    apply_coverage_range(&desiredCoverage[styleIndex], logicalStart, logicalEnd, active);
    const QString rewritten = rebuild_source_text_with_inline_style_coverage(
        sourceText,
        desiredCoverage,
        options,
        state.LogicalLength);
    const bool valid = CanParseFragment(rewritten);

    qDebug().noquote() << "iiXml::Mutation::InlineTagMutation::ApplyLogicalStyleRange output"
                       << "valid=" << valid
                       << "rewritten_size=" << rewritten.size();
    return {
        rewritten != sourceText,
        valid,
        state.LogicalLength,
        rewritten,
        valid ? QString{} : QStringLiteral("fragment-parse-failed")
    };
}

int InlineTagMutation::LogicalOffsetForSourceBoundary(
    const QString& sourceText,
    const int requestedSourceOffset,
    const InlineTagMutationOptions& options) const
{
    qDebug().noquote() << "iiXml::Mutation::InlineTagMutation::LogicalOffsetForSourceBoundary begin"
                       << "source_size=" << sourceText.size()
                       << "requested_offset=" << requestedSourceOffset;

    const int boundedSourceOffset = bounded_offset(requestedSourceOffset, sourceText.size());
    int logicalOffset = 0;
    int sourceOffset = 0;
    while (sourceOffset < boundedSourceOffset)
    {
        if (sourceText.at(sourceOffset) == QLatin1Char('<'))
        {
            const int tagEnd = sourceText.indexOf(QLatin1Char('>'), sourceOffset + 1);
            if (tagEnd > sourceOffset && tagEnd < boundedSourceOffset)
            {
                const QStringView tagToken(sourceText.constData() + sourceOffset, tagEnd - sourceOffset + 1);
                if (is_logical_break_tag_name(source_tag_name(tagToken), options))
                    logicalOffset += 1;
                sourceOffset = tagEnd + 1;
                continue;
            }
        }

        const int entityLength = html_entity_length_at(sourceText, sourceOffset);
        if (entityLength > 0 && sourceOffset + entityLength <= boundedSourceOffset)
        {
            logicalOffset += 1;
            sourceOffset += entityLength;
            continue;
        }

        logicalOffset += 1;
        sourceOffset += 1;
    }

    qDebug().noquote() << "iiXml::Mutation::InlineTagMutation::LogicalOffsetForSourceBoundary output"
                       << "logical_offset=" << logicalOffset;
    return logicalOffset;
}

int InlineTagMutation::SourceBoundaryForLogicalOffset(
    const QString& sourceText,
    const int targetLogicalOffset,
    const InlineTagMutationOptions& options) const
{
    qDebug().noquote() << "iiXml::Mutation::InlineTagMutation::SourceBoundaryForLogicalOffset begin"
                       << "source_size=" << sourceText.size()
                       << "logical_offset=" << targetLogicalOffset;

    const int boundedLogicalOffset = std::max(0, targetLogicalOffset);
    int logicalOffset = 0;
    int sourceOffset = 0;
    while (sourceOffset < sourceText.size())
    {
        if (sourceText.at(sourceOffset) == QLatin1Char('<'))
        {
            const int tagEnd = sourceText.indexOf(QLatin1Char('>'), sourceOffset + 1);
            if (tagEnd > sourceOffset)
            {
                const QStringView tagToken(sourceText.constData() + sourceOffset, tagEnd - sourceOffset + 1);
                const QString normalizedTagName = source_tag_name(tagToken);
                if (inline_style_index_for_tag(normalizedTagName, options) >= 0 && !source_tag_is_self_closing(tagToken))
                {
                    sourceOffset = tagEnd + 1;
                    continue;
                }
                if (logicalOffset == boundedLogicalOffset)
                    return sourceOffset;
                sourceOffset = tagEnd + 1;
                if (is_logical_break_tag_name(normalizedTagName, options))
                    logicalOffset += 1;
                continue;
            }
        }

        if (logicalOffset == boundedLogicalOffset)
            return sourceOffset;

        const int entityLength = html_entity_length_at(sourceText, sourceOffset);
        if (entityLength > 0)
        {
            logicalOffset += 1;
            sourceOffset += entityLength;
            continue;
        }

        logicalOffset += 1;
        sourceOffset += 1;
    }

    qDebug().noquote() << "iiXml::Mutation::InlineTagMutation::SourceBoundaryForLogicalOffset output"
                       << "source_offset=" << sourceText.size();
    return sourceText.size();
}

bool InlineTagMutation::CanParseFragment(const QString& sourceText) const
{
    qDebug().noquote() << "iiXml::Mutation::InlineTagMutation::CanParseFragment begin"
                       << "source_size=" << sourceText.size();
    const QByteArray utf8 = validation_fragment_for_source(sourceText).toUtf8();
    const iiXml::Parser::TagParser parser;
    const iiXml::Parser::TagDocumentResult parsed = parser.ParseAllDocumentResult(
        std::string_view(utf8.constData(), static_cast<std::size_t>(utf8.size())));
    const bool valid = parsed.Status == iiXml::Parser::TagTreeParseStatus::Parsed
        && parsed.Document.has_value();
    qDebug().noquote() << "iiXml::Mutation::InlineTagMutation::CanParseFragment output"
                       << "valid=" << valid;
    return valid;
}

} // namespace iiXml::Mutation
