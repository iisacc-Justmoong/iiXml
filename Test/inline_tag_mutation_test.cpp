#include <iiXml>

#include <iostream>

namespace {

int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        ++failures;
    }
}

iiXml::Mutation::InlineTagMutationOptions default_options()
{
    iiXml::Mutation::InlineTagMutationOptions options;
    options.OrderedInlineTagNames = {
        QStringLiteral("bold"),
        QStringLiteral("italic"),
        QStringLiteral("underline"),
        QStringLiteral("strikethrough"),
        QStringLiteral("highlight")
    };
    options.LogicalBreakTagNames = {
        QStringLiteral("break"),
        QStringLiteral("hr")
    };
    return options;
}

void applies_cross_inline_style_range()
{
    const iiXml::Mutation::InlineTagMutation mutation;
    const iiXml::Mutation::InlineTagMutationOptions options = default_options();
    const QString source = QStringLiteral("<bold>Alpha</bold> Beta");

    const int logicalStart = mutation.LogicalOffsetForSourceBoundary(source, 8, options);
    const int logicalEnd = mutation.LogicalOffsetForSourceBoundary(source, source.size(), options);
    const iiXml::Mutation::InlineTagMutationResult result =
        mutation.ApplyLogicalStyleRange(source, QStringLiteral("italic"), logicalStart, logicalEnd, options);

    expect(result.Applied, "cross inline style mutation should apply");
    expect(result.Valid, "cross inline style mutation should stay parseable");
    expect(result.SourceText.toStdString() == "<bold>Al<italic>pha</italic></bold><italic> Beta</italic>",
        "cross inline style mutation should preserve crossed structure");
}

void maps_source_and_logical_offsets_around_inline_tags()
{
    const iiXml::Mutation::InlineTagMutation mutation;
    const iiXml::Mutation::InlineTagMutationOptions options = default_options();
    const QString source = QStringLiteral("<bold>Al<italic>pha</italic></bold><italic> Beta</italic>");

    const int sourceOffsetAtVisibleTwo = mutation.SourceBoundaryForLogicalOffset(source, 2, options);
    expect(sourceOffsetAtVisibleTwo == 16, "visible offset 2 should map to the first styled character boundary");

    const int logicalOffset = mutation.LogicalOffsetForSourceBoundary(source, sourceOffsetAtVisibleTwo, options);
    expect(logicalOffset == 2, "mapped source boundary should round-trip back to visible offset 2");
}

void rejects_unknown_inline_tag()
{
    const iiXml::Mutation::InlineTagMutation mutation;
    const iiXml::Mutation::InlineTagMutationOptions options = default_options();
    const iiXml::Mutation::InlineTagMutationResult result =
        mutation.ApplyLogicalStyleRange(QStringLiteral("Alpha"), QStringLiteral("unknown"), 0, 5, options);

    expect(!result.Valid, "unknown style tag should be rejected");
    expect(result.ErrorReason == QStringLiteral("unsupported-inline-tag"),
        "unknown style tag should expose structured reason");
}

} // namespace

int main()
{
    applies_cross_inline_style_range();
    maps_source_and_logical_offsets_around_inline_tags();
    rejects_unknown_inline_tag();

    if (failures != 0)
        return 1;
    return 0;
}
