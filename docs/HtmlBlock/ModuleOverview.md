> Historical iiHtmlBlock documentation, preserved after integration into iiXml. Current build and package instructions are in [HTML block integration](../HtmlBlockIntegration.md).

# iiHtmlBlock

[Windows build and execution](WindowsBuildAndRun.md)

Package release: **0.1.1** (2026-10-19). This patch packages the current
source snapshot; build, test and installation evidence is recorded in the SDK
patch-release report. Earlier versioned feature notes remain historical.


`iiHtmlBlock` is a Qt6.8.3/C++20 shared library for converting `iiXml` input into HTML blocks and manipulating them. The public C++ API resides under the `iiHtmlBlock` namespace.

## GetTagInfo

`Src/Parser/GetTagInfo.*` accepts an XML string and stores the first root tag's value and fields as internal state in a way compatible with `iiXml::Parser::TagParser`. The full tag hierarchy is queried through `GetTags()`/`GetRoot()`, while the first root tag's value and fields are queried through `GetValue()` and `GetFields()`. iiXml structures that are difficult to represent with a tree alone, such as crossing closing tags, are preserved as the original offset ranges in `GetRanges()`. The XML declaration and DOCTYPE preceding the body tags are not tag ranges and are therefore removed before parsing.

See `get_tag_info.md` for details.

## GetHTML

`Src/Parser/GetHTML.*` accepts `iiXml` and other XML strings and determines them as HTML tag strings. Tag names are not transformed, and tags like `paragraph`, `XML`, `CustomTag` are also returned as custom HTML tags with their original names. Unknown tags are also kept with their input names. For example, `unknownName` is returned as `<unknownName>...</unknownName>`, and `OtherWidget` is returned as `<OtherWidget>...</OtherWidget>`.

See `get_html.md` for details.

## iiXmlToHTML

`Src/Modifier/iiXmlToHTML.*` uses the determination rules of `GetHTML` to transform iiXml / XML input into HTML string. Tag names are not transformed and are kept with their input names.

See `iixml_to_html.md` for details.

## DivideBlock

`Src/Modifier/DivideBlock.*` determines whether each element is a block unit based on the tag range parsed by `GetTagInfo`. It recognizes the default HTML block tag, and explicit display values like `style="display: block"` or `display=block` take precedence over the default tag determination. Unknown custom tags are considered blocks only when an explicit display value exists.

See `divide_block.md` for details.

## BlockRangeTracker

`Src/Modifier/BlockRangeTracker.*` tracks blocks determined by `DivideBlock` in units of the original text range. Each block is assigned a tracking ID, and a specific block can be looked up or blocks overlapping a given offset/range can be found based on the offset relative to the entire original text. `Update()` maintains existing block IDs when possible while reparsing the new document.

See `block_range_tracker.md` for details.

## BlockHTMLSerializer

`Src/Modifier/BlockHTMLSerializer.*` serializes the split results of `DivideBlock` and the merge results of `CombineBlock` into HTML string fragments. Nested blocks are not duplicated; they are output only once as included in the parent block, and the merge results are used as logical block fragments `CombinedBlock::raw`.

See `block_html_serializer.md` for details.

## DeleteBlock

`Src/Modifier/DeleteBlock.*` removes the specified block from the original HTML / iiXml string in the block list divided by `DivideBlock`. After deletion, the remaining document is reparsed to update the current block list, and the deleted block snapshot can be looked up at `GetDeletedBlocks()`.

See `delete_block.md` for details.

## CombineBlock

`Src/Modifier/CombineBlock.*` selects multiple blocks from the block list divided by `DivideBlock` and treats the selected group as a single logical block for merging. Merging saves the selection indices and the selected block snapshots to `CombinedBlock` without rewriting the original HTML.

See `combine_block.md` for details.

## FlattenBlock

`Src/Modifier/FlattenBlock.*` groups all blocks of the document into a single parent group block layer while maintaining each block. Each block remains as an individual `ElementInfo` while simultaneously being treated as a block included in a single `LayerBlock` group.

See `flatten_block.md` for details.

<a id="설치"></a>

## Install

`install.sh` builds, tests, and installs at once in the same manner as iiXml, and installs iiHtmlBlock as macOS, iOS, Android, WASM packages. The compatible root prefix for macOS is `~/.local/SDK/iiHtmlBlock`, and platform-specific packages are placed in `~/.local/SDK/iiHtmlBlock/platforms/<platform>`. After installation, the consuming project can separately add the `iiXml` prefix without using the standard install layout, or can directly import `find_package(iiHtmlBlock CONFIG REQUIRED)` or the install config file `include()`. The root config delegates to the platform package corresponding to the CMake target system to prevent the host dylib from being incorrectly linked in cross-compilation.

Targets specified by the consumer `iiXml_DIR` or already retrieved `iiXml::iiXml` are preserved. Automatic platform discovery is used only when there is no explicit selection, and XML packages using new iiFileProvider CRUD hierarchies are not reverted to the previous `.local` installation. `installed_dependency_selection_test` reads the actual installation package as a separate CMake consumer to verify that this selection is maintained.

See `install.md` for details.

<a id="사용-예시"></a>

## Usage Examples

```cpp
#include <iiHtmlBlock>

iiHtmlBlock::GetHTML html;
iiHtmlBlock::DivideBlock divider;
iiHtmlBlock::BlockRangeTracker tracker;
iiHtmlBlock::BlockHTMLSerializer serializer;
iiHtmlBlock::DeleteBlock deleter;
```

Runnable examples are in `Example/ParsedXmlHtmlExample.cpp`, and they show together the parsed XML range, HTML transformation results, block splitting, range tracking, block serialization, block deletion, selection merging, and the full layering flow. See `examples.md` for details.

<a id="검증"></a>

## Verification

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## License

SPDX-License-Identifier: MIT

The self-written code and documentation of iiHtmlBlock are distributed under the MIT License. The license text is in [LICENSE](../../LICENSE).

Third-party code, libraries, tools, and model weights retain their own individual licenses.

<!-- workspace-docs:readme:start -->
## Documentation entry points

- Human overview: this English `README.md`.
- Agent instructions: [AGENT.md](../../AGENT.md).
- Human documentation: [documentation index](README.md).
- Structured agent documentation: [agent index](../agent-index.json).
<!-- workspace-docs:readme:end -->
