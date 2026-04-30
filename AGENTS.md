# Repository Guidelines

## Project Structure & Module Organization

이 저장소는 C++20 기반의 `iixml` 공유 라이브러리입니다. 현재 모듈은 최상위의 `iiXml.h` 공개 헤더와 `iiXml.cpp` 구현 파일로 구성됩니다. 빌드 설정은 `CMakeLists.txt`에만 둡니다. 새 모듈을 추가할 때는 디렉터리 구조를 앱 아키텍처의 청사진으로 보고, 기능 단위로 파일과 폴더를 배치하십시오. 테스트가 추가되면 `tests/` 아래에 모듈별로 배치하고, 문서는 `docs/` 또는 최상위 Markdown 파일에 둡니다.

## Build, Test, and Development Commands

- `cmake -S . -B build`: 표준 빌드 디렉터리 `build/`를 생성하고 구성합니다.
- `cmake --build build`: `iixml` 공유 라이브러리를 빌드합니다.
- `ctest --test-dir build --output-on-failure`: 등록된 테스트를 실행합니다.
- `rm -rf build && cmake -S . -B build && cmake --build build`: 깨끗한 상태에서 재구성 및 빌드합니다.

빌드 디렉터리는 항상 `build/`만 사용하십시오. `cmake-build-debug/` 같은 대체 디렉터리를 새로 만들거나 확장하지 마십시오.

## Platform & Framework Policy

프로젝트에서 Qt를 사용하는 모든 빌드 설정, C++ 코드, QML UI는 Qt 6.8.3을 기준으로 작성하고 검증하십시오. 다른 Qt 버전으로 변경해야 하는 경우 이 정책 문서와 관련 빌드 및 테스트 문서를 먼저 갱신하십시오.
Qt 설치 경로는 사용자 홈의 `~/Qt` 하위 디렉터리를 기준으로 인식하십시오. Qt 경로를 문서화하거나 CMake 설정 예시를 작성할 때도 `~/Qt` 아래의 Qt 6.8.3 설치를 기준으로 설명하십시오.

## Coding Style & Naming Conventions

C++ 코드는 4칸 들여쓰기를 사용하고, 헤더에는 공개 API만 노출하십시오. 

## Testing Guidelines

새 동작에는 빌드 가능한 실행 테스트를 함께 추가하십시오. 테스트 파일은 `Test/<module>_test.cpp`처럼 대상 모듈명을 포함해 명명합니다. 테스트 프레임워크를 도입할 때는 CMake에 `enable_testing()`과 `add_test()`를 함께 등록해 `ctest --test-dir build`로 실행되게 하십시오. 에러는 건너뛰거나 숨기지 말고 재현 테스트와 함께 해결합니다.

## Documentation & Change Requirements

소스 변경만 단독으로 제출하지 마십시오. 모든 변경에는 관련 문서 갱신과 테스트 갱신 또는 테스트 불필요 사유가 포함되어야 합니다. 새 API는 헤더 주석, 사용 예시, 또는 문서에 최소한의 계약을 남기십시오.

## Commit & Pull Request Guidelines

현재 경로는 Git 저장소로 감지되지 않아 기존 커밋 규칙을 확인할 수 없습니다. Git이 초기화되면 `feat:`, `fix:`, `docs:`, `test:`, `build:` 같은 명확한 접두사를 사용하십시오. PR에는 변경 요약, 빌드 결과, 테스트 결과, 관련 이슈, UI 변경 시 스크린샷을 포함하십시오.
