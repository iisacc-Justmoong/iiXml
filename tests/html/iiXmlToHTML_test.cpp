#include "Src/Modifier/iiXmlToHTML.h"

#include <cassert>
#include <string>

using namespace iiHtmlBlock;

namespace {

void ConvertsIiXmlThroughGetHTML() {
    iiXmlToHTML converter;
    assert(converter.Convert("<XML><unknownName data-key=\"x\">value</unknownName></XML>"));
    assert(converter.GetError().empty());
    assert(converter.GetHTMLText() == "<XML><unknownName data-key=\"x\">value</unknownName></XML>");
}

void ClearsStateOnFailure() {
    iiXmlToHTML converter;
    assert(converter.Convert("<XML>value</XML>"));
    assert(!converter.GetHTMLText().empty());

    assert(!converter.Convert("<XML>value"));
    assert(converter.GetHTMLText().empty());
    assert(!converter.GetError().empty());
}

} // namespace

int main() {
    ConvertsIiXmlThroughGetHTML();
    ClearsStateOnFailure();
    return 0;
}
