# iiXml 에이전트 지침

`iiXml::Markup`은 문서 모듈과 독립적인 C++23 태그 파서와 블록 범위 추적기다. 계약은 `docs/Markup.md`를 따른다. Qt API와는 별도로 사용할 수 있다.

파일 경로 입력은 호출자가 `iiXml::IO::FileReader`를 생성자에 주입한다.
iiFileProvider 연결과 실제 파일 입력 테스트는 독립 소비자 `example/FileProvider`가
소유한다. 기본 생성자는 읽기 기능을 자동 등록하지 않는다.

HTML 블록 변환·편집은 iiXml::HtmlBlock 구성요소가 소유한다. 기존 iiHtmlBlock
공개 헤더와 네임스페이스는 호환 계약으로 유지한다. Qt 없는 iiXml::Native는
이 구성요소와 독립적으로 빌드되어야 한다. 새 소스는 src/에 둔다.

## 보존한 AGENTS.md 프로젝트 지침

<a id="repository-guidelines"></a>

# 저장소 지침

<a id="project-structure--module-organization"></a>

## 프로젝트 구조 및 모듈 구성

이 저장소는 C++23 기반의 `iiXml` 라이브러리입니다. 공개 헤더와 구현은 `src/`에 함께 두고 빌드 설정은 CMake에서 관리합니다. 새 기능은 책임별로 배치합니다. 기존 테스트는 `Test/`, HTML 구성요소 테스트는 `tests/html/`에 두고 문서는 `docs/`에 둡니다.

<a id="build-test-and-development-commands"></a>

## 빌드, 테스트 및 개발 명령

- `cmake -S . -B build`: 표준 빌드 디렉터리 `build/`를 생성하고 구성합니다.
- `cmake --build build`: `iiXml` 공유 라이브러리를 빌드합니다.
- `ctest --test-dir build --output-on-failure`: 등록된 테스트를 실행합니다.
- `rm -rf build && cmake -S . -B build && cmake --build build`: 깨끗한 상태에서 재구성 및 빌드합니다.

빌드 디렉터리는 항상 `build/`만 사용하십시오. `cmake-build-debug/` 같은 대체 디렉터리를 새로 만들거나 확장하지 마십시오.

<a id="platform--framework-policy"></a>

## 플랫폼 및 프레임워크 정책

프로젝트에서 Qt를 사용하는 모든 빌드 설정, C++ 코드, QML UI는 Qt 6.8.3을 기준으로 작성하고 검증하십시오. 다른 Qt 버전으로 변경해야 하는 경우 이 정책 문서와 관련 빌드 및 테스트 문서를 먼저 갱신하십시오.
Qt 설치 경로는 외장 스토리지의 `/Volumes/Storage/Qt` 하위 디렉터리를 기준으로 인식하십시오. Qt 경로를 문서화하거나 CMake 설정 예시를 작성할 때도 `/Volumes/Storage/Qt` 아래의 Qt 6.8.3 설치를 기준으로 설명하십시오.

<a id="coding-style--naming-conventions"></a>

## 코딩 스타일 및 명명 규칙

2026-10-22 사용자의 비GUI Qt 독립 요구에 따라 `Native/`와 `Test/native_test.cpp`는 C++23 표준 타입 기반의 독립 API로 구현한다. 이 영역에는 QObject 상속, Qt 메타 타입, Qt 로깅을 적용하지 않는다. 기존 Qt API의 규칙은 유지하며 `IIXML_BUILD_QT_API=OFF`일 때 Qt를 검색·링크하지 않아야 한다. 네이티브 계약은 `docs/native.md`를 따른다.

C++ 코드는 4칸 들여쓰기를 사용하고, 헤더에는 공개 API만 노출하십시오. 모든 객체는 QObject를 상속하고, Qt 메타 타입 시스템을 사용하여 C++와 QML 간의 상호 작용을 지원합니다. 모든 객체 및 메서드는 디버깅 로그를 출력해야 한다.

<a id="testing-guidelines"></a>

## 테스트 지침

새 동작에는 빌드 가능한 실행 테스트를 함께 추가하십시오. 테스트 파일은 `Test/<module>_test.cpp`처럼 대상 모듈명을 포함해 명명합니다. 테스트 프레임워크를 도입할 때는 CMake에 `enable_testing()`과 `add_test()`를 함께 등록해 `ctest --test-dir build`로 실행되게 하십시오. 에러는 건너뛰거나 숨기지 말고 재현 테스트와 함께 해결합니다.

<a id="documentation--change-requirements"></a>

## 문서 및 변경 요구 사항

소스 변경만 단독으로 제출하지 마십시오. 모든 변경에는 관련 문서 갱신과 테스트 갱신 또는 테스트 불필요 사유가 포함되어야 합니다. 새 API는 헤더 주석, 사용 예시, 또는 문서에 최소한의 계약을 남기십시오.

<a id="commit--pull-request-guidelines"></a>

## 커밋 및 풀 요청 지침

현재 경로는 Git 저장소로 감지되지 않아 기존 커밋 규칙을 확인할 수 없습니다. Git이 초기화되면 `feat:`, `fix:`, `docs:`, `test:`, `build:` 같은 명확한 접두사를 사용하십시오. PR에는 변경 요약, 빌드 결과, 테스트 결과, 관련 이슈, UI 변경 시 스크린샷을 포함하십시오.

<!-- workspace-docs:agent:start -->
## 문서 구조와 에이전트 탐색

- 프로젝트 루트의 `README.md`는 사람용 영문 진입 문서이며 `AGENT.md`는 에이전트 진입점이다.
- 하위 인간용 Markdown과 구조화된 agent JSON을 함께 유지한다. Markdown이 원본이며 JSON은 원문 해시와 줄 범위를 가진 파생 문서이다.
- 먼저 [docs/agent-index.json](docs/agent-index.json)에서 관련 문서와 JSON 투영을 찾는다. 원문 SHA-256이 다르면 현재 Markdown을 읽고 투영을 갱신한다.
- JSON의 규칙 후보는 검색용 어휘 일치 결과이다. 과거 기록·예제·실패 상태를 현재 지시나 검증 완료로 자동 해석하지 않는다.
- 기존 프로젝트별 규칙, 동결된 증거, 라이선스, 외부 원본, 사용자 데이터와 다른 작업의 변경을 보존한다.
- 이 문서 재편 지침은 과거 작업의 기능 구현·배포를 자동 재개하라는 요청이 아니다. 현재 사용자의 작업 범위를 우선한다.
- 문서를 수정하면 JSON 쌍·색인을 함께 갱신하고 검증한다. 기존 소스 변경에 대한 빌드·테스트 규칙은 계속 따른다.

Workspace에서 이 프로젝트의 문서를 갱신하는 명령은 다음과 같다.

```sh
python3 ../../Engineering/WorkspaceDocumentation/App/workspace_docs.py --workspace ../.. sync --project SDK/iiXml
```
<!-- workspace-docs:agent:end -->
