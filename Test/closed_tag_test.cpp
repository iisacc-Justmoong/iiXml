#include <iiXml>

#include <QObject>
#include <QString>

#include <iostream>
#include <optional>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

void detects_immediate_closed_tag() {
    const iiXml::Elements::ClosedTag closed_tag;

    const std::optional<iiXml::Elements::ClosedTagMatch> matched =
        closed_tag.MatchImmediate("</XML>");

    expect(matched.has_value(), "immediate closed tag should match");
    if (!matched.has_value()) {
        return;
    }

    expect(matched->Flag, "immediate closed tag should set the flag");
    expect(matched->TagName == "XML", "closed tag name should be XML");
    expect(matched->Raw == "</XML>", "closed tag raw text should be preserved");
    expect(matched->RawBegin == 0, "closed tag raw begin should be zero");
    expect(matched->RawEnd == 6, "closed tag raw end should be exclusive");
    expect(closed_tag.IsImmediateClosedTag("</XML>"),
        "IsImmediateClosedTag should return true for a closed tag");
}

void preserves_source_offset_after_leading_space() {
    const iiXml::Elements::ClosedTag closed_tag;

    const std::optional<iiXml::Elements::ClosedTagMatch> matched =
        closed_tag.MatchImmediate(" \n</node>\t", 10);

    expect(matched.has_value(), "closed tag with leading whitespace should match");
    if (!matched.has_value()) {
        return;
    }

    expect(matched->TagName == "node", "closed tag name should be node");
    expect(matched->Raw == "</node>", "closed tag raw text should exclude outer whitespace");
    expect(matched->RawBegin == 12, "closed tag raw begin should include source offset");
    expect(matched->RawEnd == 19, "closed tag raw end should include source offset");
}

void rejects_non_closed_tag_input() {
    const iiXml::Elements::ClosedTag closed_tag;

    expect(!closed_tag.IsImmediateClosedTag("<XML></XML>"),
        "opening tag input should not be flagged as a closed tag");
    expect(!closed_tag.MatchImmediate("</>").has_value(),
        "closed tag without a name should be rejected");
    expect(!closed_tag.MatchImmediate("</XML attr>").has_value(),
        "closed tag with unexpected content should be rejected");
}

void emits_flag_for_closed_tag_slot() {
    iiXml::Elements::ClosedTag closed_tag;
    bool flag_signal_emitted = false;
    bool parsed_signal_emitted = false;
    bool flag = false;
    QString tag_name;
    QString raw;

    QObject::connect(&closed_tag, &iiXml::Elements::ClosedTag::ClosedTagFlagged,
        [&](bool emitted_flag) {
            flag_signal_emitted = true;
            flag = emitted_flag;
        });
    QObject::connect(&closed_tag, &iiXml::Elements::ClosedTag::ClosedTagParsed,
        [&](const QString& emitted_tag_name, const QString& emitted_raw) {
            parsed_signal_emitted = true;
            tag_name = emitted_tag_name;
            raw = emitted_raw;
        });

    closed_tag.ParseClosedTag("</XML>");

    expect(flag_signal_emitted, "ClosedTag should emit ClosedTagFlagged");
    expect(flag, "ClosedTag should emit a true flag for immediate closed tags");
    expect(parsed_signal_emitted, "ClosedTag should emit ClosedTagParsed");
    expect(tag_name == "XML", "ClosedTag should emit the parsed tag name");
    expect(raw == "</XML>", "ClosedTag should emit the raw closed tag");
}

void emits_rejection_for_non_closed_tag_slot() {
    iiXml::Elements::ClosedTag closed_tag;
    bool flag_signal_emitted = false;
    bool rejected_signal_emitted = false;
    bool flag = true;
    QString reason;

    QObject::connect(&closed_tag, &iiXml::Elements::ClosedTag::ClosedTagFlagged,
        [&](bool emitted_flag) {
            flag_signal_emitted = true;
            flag = emitted_flag;
        });
    QObject::connect(&closed_tag, &iiXml::Elements::ClosedTag::ClosedTagRejected,
        [&](const QString& emitted_reason) {
            rejected_signal_emitted = true;
            reason = emitted_reason;
        });

    closed_tag.ParseClosedTag("<XML></XML>");

    expect(flag_signal_emitted, "ClosedTag should emit a flag for rejected input");
    expect(!flag, "ClosedTag should emit a false flag for non closed tag input");
    expect(rejected_signal_emitted, "ClosedTag should emit ClosedTagRejected");
    expect(!reason.isEmpty(), "ClosedTag rejection should include a reason");
}

} // namespace

int main() {
    detects_immediate_closed_tag();
    preserves_source_offset_after_leading_space();
    rejects_non_closed_tag_input();
    emits_flag_for_closed_tag_slot();
    emits_rejection_for_non_closed_tag_slot();

    return failures == 0 ? 0 : 1;
}
