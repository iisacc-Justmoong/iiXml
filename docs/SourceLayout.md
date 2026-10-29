# 네이티브 소스 책임 분류

Qt 없는 XML API의 `Xml.h`와 `Xml.cpp`는 `Native/src/`에 함께 둔다. 원문 태그 파서, 진단과 블록 범위 추적 구현은 `Native/src/Markup/`에 헤더와 함께 둔다. 기존 Qt API와 HTML 블록 구성요소는 별도의 `src/` 책임 디렉터리를 유지한다.

빌드에서 생성되는 forwarding 헤더와 설치 패키지는 `<iiXml/Xml.h>` 및 `<iiXml/Markup/...>` 공개 경로를 유지한다. `Test/source_layout.cmake`와 SDK 공통 구조 검사가 중복 디렉터리의 재도입을 검사한다.
`Test/installed-native`는 설치된 XML과 마크업 CMake 타깃을 독립적으로 소비한다.
macOS의 로컬 테스트는 셸의 DYLD 검색 경로를 제거해 이전 설치본 대신 방금 빌드한 라이브러리를 검사한다.
