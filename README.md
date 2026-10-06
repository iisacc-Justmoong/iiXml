# iiXml

iiXml is a C++ library for defining custom XML rules and producing input/output that follows those rules.

Core parser element modules live under `Src/Elements/`. `OpenTag` tracks relaxed tag closure, while `ClosedTag` flags input that starts immediately with a `</tag>` closing marker before a matching opener appears in the same view.

## License

SPDX-License-Identifier: AGPL-3.0-only

The self-written code and documents of iiXml are distributed under the GNU Affero General Public License version 3 only ( `AGPL-3.0-only` ). The full terms follow [LICENSE](LICENSE).

Third-party code, libraries, tools, and models retain their respective licenses and copyright notices, and the repository's license does not replace them.

<a id="파일-저장-소유권"></a>

## File storage ownership

FileParser and GetFile are iiFileProvider 0.5 passes the bytes read by XML to the parser. I/O of the input path is owned by the provider, while tokenization, syntax validation, and QObject signals are owned by iiXml. File creation, update, and deletion are also combined via the provider API, and back-references are forbidden.

A read failure by the provider retains the existing `ParseFile failed` logs and the failure return value. macOS validation is executed by `cmake -E env --unset=DYLD_LIBRARY_PATH --unset=DYLD_FRAMEWORK_PATH --unset=DYLD_FALLBACK_LIBRARY_PATH ctest --test-dir build --output-on-failure` to prevent the previous installed library from obscuring the new build. Since the installed shared library preserves the runtime path of private iiFileProvider dependencies, a consumer that links only XML, like iiHtmlBlock, can also load the provider.
