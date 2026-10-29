#include <iiXml/Markup/TagParser.h>
#include "src/Parser/TagParser.h"
#include <type_traits>

static_assert(std::is_class_v<ii::xml::markup::Parser::TagParser>);
static_assert(std::is_class_v<iiXml::Parser::TagParser>);
int main() { return 0; }
