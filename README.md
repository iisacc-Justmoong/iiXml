# iiXml

iiXml is a C++ library for defining custom XML rules and producing input/output that follows those rules.

Core parser element modules live under `Src/Elements/`. `OpenTag` tracks relaxed tag closure, while `ClosedTag` flags input that starts immediately with a `</tag>` closing marker before a matching opener appears in the same view.

## License

SPDX-License-Identifier: AGPL-3.0-only

iiXml의 자체 작성 코드와 문서는 GNU Affero General Public License version 3 only
(`AGPL-3.0-only`)로 배포된다. 전체 조건은 [LICENSE](LICENSE)를 따른다.

서드파티 코드·라이브러리·도구·모델은 각각의 라이선스와 저작권 고지를 유지하며,
이 저장소의 라이선스가 이를 대체하지 않는다.

## 파일 저장 소유권

FileParser 및 GetFile은 iiFileProvider 0.5가 읽은 바이트를 XML 파서로 전달한다. 입력 경로의 I/O는 provider가, 토큰화·문법 검증·QObject 신호는 iiXml이 소유한다. 파일 생성·갱신·삭제도 provider API로 조합하며 역참조를 금지한다.

provider의 읽기 실패는 기존 `ParseFile failed` 로그와 실패 반환값을 유지한다. macOS 검증은 `cmake -E env --unset=DYLD_LIBRARY_PATH --unset=DYLD_FRAMEWORK_PATH --unset=DYLD_FALLBACK_LIBRARY_PATH ctest --test-dir build --output-on-failure`로 실행하여 이전 설치 라이브러리가 새 빌드를 가리는 것을 방지한다.
설치된 공유 라이브러리는 private iiFileProvider 의존성의 런타임 경로를 보존하므로, iiHtmlBlock처럼 XML만 링크하는 소비자도 provider를 로드할 수 있다.
