# iiXml

iiXml is a C++ library for defining custom XML rules and producing input/output that follows those rules.

Core parser element modules live under `Src/Elements/`. `OpenTag` tracks relaxed tag closure, while `ClosedTag` flags input that starts immediately with a `</tag>` closing marker before a matching opener appears in the same view.

## License

SPDX-License-Identifier: AGPL-3.0-only

iiXml의 자체 작성 코드와 문서는 GNU Affero General Public License version 3 only
(`AGPL-3.0-only`)로 배포된다. 전체 조건은 [LICENSE](LICENSE)를 따른다.

서드파티 코드·라이브러리·도구·모델은 각각의 라이선스와 저작권 고지를 유지하며,
이 저장소의 라이선스가 이를 대체하지 않는다.
