# 과거 iiHtmlBlock 설치

> 이 문서는 병합 전 독립 패키지의 기록이다. 현재 설치와 구성요소는 [통합 안내](../HtmlBlockIntegration.md)를 따른다.

Windows의 설치 레이아웃 검증은 POSIX 실행 비트 대신 Bash 스크립트 헤더를
검사한다. 네이티브 패키지는 CMake configure, build, CTest, install 순서로 검증한다.

`install.sh`는 기본적으로 iiHtmlBlock을 macOS, iOS, Android, WASM 플랫폼 패키지로 설치한다.
macOS 호환 설치 위치는 `~/.local/SDK/iiHtmlBlock`이고, 플랫폼별 설치물은
`~/.local/SDK/iiHtmlBlock/platforms/<platform>` 아래에 둔다. 설치 위치는 고정이며
다른 prefix를 받지 않는다.

## 전제 조건

- `cmake --fresh`를 지원하는 CMake 3.24 이상이 필요하다.
- Qt 6.8.3은 `/Volumes/Storage/Qt/6.8.3/macos` 아래에 설치되어 있어야 한다.
- iOS 패키지를 만들려면 Qt 6.8.3 iOS kit가 `/Volumes/Storage/Qt/6.8.3/ios` 아래에 설치되어 있어야 한다.
- Android 패키지를 만들려면 Qt Android kit, Android SDK/NDK가 설치되어 있어야 한다.
- WASM 패키지를 만들려면 Qt WASM kit와 Emscripten SDK가 설치되어 있어야 한다.
- iiXml은 `~/.local/SDK/iiXml` 아래에 설치되어 있어야 한다.
- iOS 패키지를 만들려면 iiXml iOS 패키지가 `~/.local/SDK/iiXml/platforms/ios` 아래에
  설치되어 있어야 한다.
- Android와 WASM 패키지는 각각 `~/.local/SDK/iiXml/platforms/android`,
  `~/.local/SDK/iiXml/platforms/wasm`의 iiXml 패키지를 사용한다.

## 실행

```sh
./install.sh
```

스크립트는 항상 루트의 `build/` 디렉터리 아래만 사용한다. macOS host 빌드는
`build/`를 그대로 쓰고, 교차 빌드는 `build/platforms/<platform>`을 사용한다.

1. macOS 패키지를 `cmake -S . -B build`로 구성한다.
2. macOS 라이브러리를 빌드하고 `ctest --test-dir build --output-on-failure`로 테스트를 실행한다.
3. macOS 패키지를 `~/.local/SDK/iiHtmlBlock`과 `~/.local/SDK/iiHtmlBlock/platforms/macos`에 설치한다.
4. iOS, Android, WASM 패키지를 각 Qt toolchain으로 구성하고 Release 라이브러리를 빌드한다.
5. 각 결과를 `~/.local/SDK/iiHtmlBlock/platforms/<platform>`에 설치하고 라이브러리와 CMake config 존재를 검증한다.

기본 플랫폼 목록은 `macos,ios,android,wasm`이다. 임시로 일부 플랫폼만 다시 설치해야 한다면 다음처럼
환경 변수로 제한할 수 있다.

```sh
IIHTMLBLOCK_INSTALL_PLATFORMS=ios ./install.sh
```

`build/CMakeCache.txt`가 다른 소스 디렉터리에서 생성된 캐시라면, 스크립트는 잘못된
빌드 산출물이 섞이지 않도록 `build/`를 삭제한 뒤 현재 저장소 루트로 다시 구성한다.

## 과거 iiHtmlBlock 설치

> 이 문서는 병합 전 독립 패키지의 기록이다. 현재 설치와 구성요소는 [통합 안내](../HtmlBlockIntegration.md)를 따른다. 결과

- `~/.local/SDK/iiHtmlBlock/lib`: iiHtmlBlock 공유 라이브러리
- `~/.local/SDK/iiHtmlBlock/include/iiHtmlBlock`: `#include <iiHtmlBlock>`용 umbrella 헤더
- `~/.local/SDK/iiHtmlBlock/include/iiHtmlBlock.h`: 기존 파일명 기반 공개 헤더
- `~/.local/SDK/iiHtmlBlock/include/Src`: 모듈별 공개 헤더
- `~/.local/SDK/iiHtmlBlock/lib/cmake/iiHtmlBlock`: `iiHtmlBlockConfig.cmake`와 export target 파일
- `~/.local/SDK/iiHtmlBlock/platforms/macos`: macOS 전용 패키지 mirror
- `~/.local/SDK/iiHtmlBlock/platforms/ios`: iOS 전용 패키지
- `~/.local/SDK/iiHtmlBlock/platforms/android`: Android arm64-v8a 전용 패키지
- `~/.local/SDK/iiHtmlBlock/platforms/wasm`: Qt WASM multithread 전용 static archive 패키지

## CMake 사용 예

설치 후 다른 Qt 프로젝트에서는 다음처럼 가져온다.

```cmake
list(PREPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local/SDK/iiHtmlBlock")

find_package(iiHtmlBlock CONFIG REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE iiHtmlBlock::iiHtmlBlock)
```

루트 `iiHtmlBlockConfig.cmake`는 소비 프로젝트의 `CMAKE_SYSTEM_NAME`을 기준으로
플랫폼 패키지를 먼저 찾는다. iOS, Android, Emscripten 구성에서는 각각 대응하는
`platforms/<platform>` config로 위임하므로 macOS dylib를 교차 빌드에 잘못 링크하는
경로를 막는다. 루트 버전 파일은 64비트 네이티브와 32-bit WASM을 함께 선택하도록
아키텍처 독립형으로 게시하고, 플랫폼별 버전 파일은 바이너리 포인터 크기 검사를 유지한다.
Qt WASM은 정적 Qt SDK를 사용하므로 WASM 패키지는 `libiiHtmlBlock.a` static archive로
게시한다. 이 방식은 여러 설치 라이브러리를 한 최종 WASM 실행 파일에 링크할 때
각 pseudo-shared library에 포함된 Qt 심볼이 충돌하는 문제를 막는다.

플랫폼별 config는 자기 설치 prefix의 sibling `../iiXml`, 플랫폼 sibling
`~/.local/SDK/iiXml/platforms/<platform>`, 기본 `~/.local/SDK/iiXml`, 그리고 대응 Qt prefix를
탐색 경로에 추가한다. 따라서 표준 설치 레이아웃에서는 소비 프로젝트가
iiXml prefix를 별도로 추가하지 않아도 `find_package(iiHtmlBlock)`만으로
`iiXml::iiXml` 의존성이 함께 해결된다.

config 파일을 직접 포함하는 방식도 지원한다.

```cmake
include("$ENV{HOME}/.local/SDK/iiHtmlBlock/lib/cmake/iiHtmlBlock/iiHtmlBlockConfig.cmake")

add_executable(app main.cpp)
target_link_libraries(app PRIVATE iiHtmlBlock::iiHtmlBlock)
```

Qt 또는 iiXml을 기본 위치가 아닌 곳에 설치했다면 해당 prefix는 소비 프로젝트에서
명시적으로 `CMAKE_PREFIX_PATH`에 추가해야 한다.

코드에서는 다음처럼 단일 umbrella 헤더를 사용할 수 있다.

```cpp
#include <iiHtmlBlock>

iiHtmlBlock::GetHTML html;
```

저장소의 `umbrella_header_test`도 이 angle-bracket include 형태를 직접 컴파일해
공개 헤더 진입점과 `iiHtmlBlock` 네임스페이스 사용을 검증한다.

`iiHtmlBlock::iiHtmlBlock` imported target은 include path, Qt `Core`, iiXml 링크
의존성을 함께 제공한다.

소스 저장소는 `Workspace/SDK/iiHtmlBlock`이다. 직접 CMake를 구성해도 기본 설치
prefix는 `~/.local/SDK/iiHtmlBlock`이며, 명시한 `CMAKE_INSTALL_PREFIX`는 유지한다.

macOS에서는 셸의 `LIBRARY_PATH`에 설치 라이브러리 경로가 포함되어 있어도
imported target이 필요한 runtime RPATH를 전달하므로 소비자 실행에 별도의
`DYLD_LIBRARY_PATH`가 필요하지 않다.

테스트와 `ParsedXmlHtmlExample`은 Release에서도 assertion을 실행한다. 이 실행 파일들은
assertion 안에서 파서 초기화와 결과 검사를 수행하므로 `NDEBUG`로 이를 생략하면 검사가
실행되지 않거나 비어 있는 결과를 참조하게 된다. 라이브러리의 Release 최적화 설정은 유지하며,
실행 파일에만 `NDEBUG` 해제를 적용한다. `BlockHTMLSerializer_test`는 assertion이 꺼진
구성을 컴파일 단계에서도 거부한다.

기본 SDK prefix는 이 저장소를 단독 구성할 때만 적용한다. `add_subdirectory()`로
포함한 상위 프로젝트의 설치 prefix는 유지하고, `HOME`이 비어 있으면
`USERPROFILE`을 사용자 홈으로 사용한다.
# Static linkage on Windows

Use `-DIIHTMLBLOCK_BUILD_SHARED=OFF` when linking against a static iiXml package.
This avoids exporting iiXml symbols again from an intermediate MinGW DLL.
Shared libraries remain the default; WebAssembly always uses static linkage.
