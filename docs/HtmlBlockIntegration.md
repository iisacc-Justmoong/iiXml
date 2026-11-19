# XML과 HTML 블록 통합

iiXml은 XML 파싱과 HTML 블록의 파싱·변환·분할·범위 추적·직렬화·편집을
하나의 SDK에서 제공한다. 기존 HTML 객체는 `iiHtmlBlock` 네임스페이스와
`<iiHtmlBlock>` 헤더를 유지하며, 구현과 테스트는 iiXml 안에 있다.

새 소비자는 find_package(iiXml 0.1.2 CONFIG REQUIRED COMPONENTS HtmlBlock)을
호출하고 iiXml::HtmlBlock을 링크한다. 구현 타깃은 XML 파서를 사용하는 방향으로
iiXml::iiXml에 링크된다. 기존 find_package(iiHtmlBlock CONFIG REQUIRED)와
iiHtmlBlock::iiHtmlBlock은 같은 iiXml 설치본의 컴포넌트를 가리킨다. 호환 이름도
0.1.2이며, 그 Config는 설치 위치에 함께 있는 iiXml Config를 직접 읽는다.
CMake가 고르는 별도 iiXml 설치본이나 iiXml_DIR을 통해 대상을 바꾸지 않는다.
구형 독립 iiHtmlBlock 패키지가 먼저 검색되는 환경에서는 iiHtmlBlock_DIR을
iiXml 설치본 안의 lib/cmake/iiHtmlBlock으로 바꾸거나 0.1.2 이상을 요청한다.
기존 <Src/...> 공개 헤더 경로도 빌드 및 설치 소비자에서 유지한다.

`IIXML_BUILD_HTML_BLOCK=OFF`는 HTML 구성요소를 제외한다. `IIXML_BUILD_QT_API=OFF`는
Qt를 찾지 않고 기존 `iiXmlNative`/`iiXml::Native`만 빌드한다. HTML 객체는 현재
Qt QObject API이며 Qt 없는 HTML 엔진을 구현했다고 주장하지 않는다.

기존 HTML 객체의 단위 테스트와 XML·HTML 통합 테스트, 두 패키지 이름을 사용하는
별도 설치 소비자 테스트를 `build/`에서 실행한다. 각 소비자는 상대 패키지의
CMake 검색을 금지한 상태로 구성·빌드하고 파싱 결과를 확인한다. 프로젝트 고정 자원은
`resources/`에 두며 사용자 파일 생성은 iiFileProvider에 위임한다.

2026-10-27 macOS arm64에서 기존 XML·HTML 기능 검사 28개와 수정된 설치 소비자
검사 1개를 통과했다. `IIXML_BUILD_QT_API=OFF` 구성과 Qt 탐색을 비활성화한
네이티브 설치 소비자도 각각 통과했다. Windows·Linux 실행은 확인하지 않았다.
