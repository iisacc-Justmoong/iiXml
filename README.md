# iiXml

iiXml is a C++ library for defining custom XML rules and producing input/output that follows those rules.

Core parser element modules live under `Src/Elements/`. `OpenTag` tracks relaxed tag closure, while `ClosedTag` flags input that starts immediately with a `</tag>` closing marker before a matching opener appears in the same view.
