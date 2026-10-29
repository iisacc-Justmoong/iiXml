# iiXml

Native XML sources are in `Native/src/`, with source-preserving markup in `Native/src/Markup/`; see [source layout](docs/SourceLayout.md). Installed include names are unchanged.

The independent `iiXmlMarkup` package exports `iiXml::Markup`: a C++23,
Qt-free source-preserving tag parser and HTML block range tracker used by
iiGeneralDocument. It does not depend on iiGeneralDocument or libxml2. See
[the markup contract](docs/Markup.md).

HTML block parsing, conversion, range tracking, serialization and editing are
owned by iiXml as the optional Qt component `iiXml::HtmlBlock`. Existing
`<iiHtmlBlock>` headers, the `iiHtmlBlock` namespace and the
`iiHtmlBlock::iiHtmlBlock` CMake package remain compatibility interfaces.
See [HTML integration](docs/HtmlBlockIntegration.md).

The C++23 Qt-free package `iiXmlNative` exports `iiXml::Native` and
`<iiXml/Xml.h>` for bounded UTF-8 pull reading and XML writing. Configure with
`-DIIXML_BUILD_QT_API=OFF` to avoid finding Qt. The existing QObject API remains
available independently. See [native contracts](docs/native.md).

[Windows build and execution](docs/WindowsBuildAndRun.md)

Package release: **0.1.1** (2026-10-19). This patch packages the current
source snapshot; build, test and installation evidence is recorded in the SDK
patch-release report. Earlier versioned feature notes remain historical.


iiXml is a C++ library for defining custom XML rules and producing input/output that follows those rules.

Core parser element modules live under `Src/Elements/`. `OpenTag` tracks relaxed tag closure, while `ClosedTag` flags input that starts immediately with a `</tag>` closing marker before a matching opener appears in the same view.

## License

SPDX-License-Identifier: MIT

The self-written code and documents of iiXml are distributed under the MIT License ( `MIT` ). The full terms follow [LICENSE](LICENSE).

Third-party code, libraries, tools, and models retain their respective licenses and copyright notices, and the repository's license does not replace them.

<a id="파일-저장-소유권"></a>

## File storage ownership

`FileParser` and `GetFile` accept an application-owned `iiXml::IO::FileReader`.
The XML library does not discover or link another SDK. The separate
`example/FileProvider` consumer adapts iiFileProvider to that contract, retaining
provider-owned file reads and the existing file parsing checks. The default
constructor has no reader: path-based calls fail until a reader is supplied,
while in-memory parsing is unchanged. Existing users must pass a reader and
rebuild both the library and their application; object layout and behavior
changed. See [FileParser](docs/file_parser.md) and [GetFile](docs/get_file.md).

<!-- workspace-docs:readme:start -->
## Documentation entry points

- Human overview: this English `README.md`.
- Agent instructions: [AGENT.md](AGENT.md).
- Human documentation: [documentation index](docs/README.md).
- Structured agent documentation: [agent index](docs/agent-index.json).
<!-- workspace-docs:readme:end -->
