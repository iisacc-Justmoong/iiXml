#include "Src/Parser/GetTagInfo.h"

#include <cassert>
#include <string>

using namespace iiHtmlBlock;

namespace {

const GetTagInfo::FieldInfo* FindField(
    const std::vector<GetTagInfo::FieldInfo>& fields,
    const std::string& name
) {
    for (const GetTagInfo::FieldInfo& field : fields) {
        if (field.name == name) {
            return &field;
        }
    }

    return nullptr;
}

const GetTagInfo::RangeInfo* FindRange(
    const std::vector<GetTagInfo::RangeInfo>& ranges,
    const std::string& name
) {
    for (const GetTagInfo::RangeInfo& range : ranges) {
        if (range.tag_name == name) {
            return &range;
        }
    }

    return nullptr;
}

void ParsesRootValueAndFields() {
    const std::string xml =
        "<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE XML>\n"
        "<XML id=\"main\" order=7 visible=true ratio=1.5><body>Hello</body></XML>";

    GetTagInfo info;
    assert(info.Parse(xml));
    assert(info.GetError().empty());
    assert(info.GetTagName() == "XML");
    assert(info.GetValue() == "<body>Hello</body>");
    assert(info.GetRaw() == "<XML id=\"main\" order=7 visible=true ratio=1.5><body>Hello</body></XML>");

    const GetTagInfo::TagInfo* root = info.GetRoot();
    assert(root != nullptr);
    assert(root->children.size() == 1);
    assert(root->children.front().tag_name == "body");
    assert(root->children.front().value == "Hello");

    const std::vector<GetTagInfo::FieldInfo>& fields = info.GetFields();
    assert(fields.size() == 4);

    const GetTagInfo::FieldInfo* id = FindField(fields, "id");
    assert(id != nullptr);
    assert(id->has_value);
    assert(id->value == "main");
    assert(id->name_begin < id->name_end);
    assert(id->value_begin < id->value_end);
    assert(id->type == GetTagInfo::FieldType::String);

    const GetTagInfo::FieldInfo* order = FindField(fields, "order");
    assert(order != nullptr);
    assert(order->has_value);
    assert(order->value == "7");
    assert(order->type == GetTagInfo::FieldType::Int);

    const GetTagInfo::FieldInfo* visible = FindField(fields, "visible");
    assert(visible != nullptr);
    assert(visible->has_value);
    assert(visible->value == "true");
    assert(visible->type == GetTagInfo::FieldType::Bool);

    const GetTagInfo::FieldInfo* ratio = FindField(fields, "ratio");
    assert(ratio != nullptr);
    assert(ratio->has_value);
    assert(ratio->value == "1.5");
    assert(ratio->type == GetTagInfo::FieldType::Float);
}

void PreservesIiXmlCrossClosedRanges() {
    const std::string xml =
        "<XML>\n"
        "  <body>\n"
        "    <paragraph order=\"1\">\n"
        "      Hello <bold>world\n"
        "    </paragraph>\n"
        "</bold>\n"
        "  </body>\n"
        "</XML>";

    GetTagInfo info;
    assert(info.Parse(xml));

    const GetTagInfo::TagInfo* root = info.GetRoot();
    assert(root != nullptr);
    assert(root->tag_name == "XML");
    assert(root->children.size() == 1);
    assert(root->children.front().tag_name == "body");

    const GetTagInfo::TagInfo& body = root->children.front();
    assert(body.children.size() == 2);
    assert(body.children[0].tag_name == "paragraph");
    assert(body.children[1].tag_name == "bold");

    const std::vector<GetTagInfo::RangeInfo>& ranges = info.GetRanges();
    assert(ranges.size() == 4);

    const GetTagInfo::RangeInfo* paragraph = FindRange(ranges, "paragraph");
    const GetTagInfo::RangeInfo* bold = FindRange(ranges, "bold");
    assert(paragraph != nullptr);
    assert(bold != nullptr);
    assert(paragraph->raw == "<paragraph order=\"1\">\n      Hello <bold>world\n    </paragraph>");
    assert(bold->raw == "<bold>world\n    </paragraph>\n</bold>");
    assert(paragraph->raw_begin < bold->raw_begin);
    assert(paragraph->raw_end < bold->raw_end);
    assert(bold->raw_begin < paragraph->raw_end);
}

void ClearsStoredStateOnFailure() {
    GetTagInfo info;
    assert(info.Parse("<XML id=\"main\">value</XML>"));
    assert(!info.GetTags().empty());
    assert(!info.GetRanges().empty());

    assert(!info.Parse("<XML id=\"main\">value"));
    assert(info.GetTags().empty());
    assert(info.GetRanges().empty());
    assert(info.GetRoot() == nullptr);
    assert(info.GetTagName().empty());
    assert(info.GetValue().empty());
    assert(!info.GetError().empty());
}

} // namespace

int main() {
    ParsesRootValueAndFields();
    PreservesIiXmlCrossClosedRanges();
    ClearsStoredStateOnFailure();
    return 0;
}
