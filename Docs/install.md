# 설치

`install.sh`는 기본적으로 iiXml을 현재 macOS 호스트에서 사용하는 전 플랫폼 집합
`macos,ios,android,wasm` 기준으로 설치한다. macOS 호환 설치 위치는 기존과 같은
`~/.local/iiXml`이고, 플랫폼별 설치물은 `~/.local/iiXml/platforms/<platform>` 아래에 둔다.
설치 위치는 고정이며 다른 prefix를 받지 않는다.

## 전제 조건

- `cmake --fresh`를 지원하는 CMake 3.24 이상이 필요하다.
- Qt 6.8.3은 `~/Qt/6.8.3/macos` 아래에 설치되어 있어야 한다.
- iOS 패키지를 만들려면 Qt 6.8.3 iOS kit가 `~/Qt/6.8.3/ios` 아래에 설치되어 있어야 한다.
- Android 패키지를 만들려면 Qt Android kit, Android SDK, Android NDK가 필요하다.
  스크립트는 Apple Silicon Homebrew의 `/opt/homebrew/share/android-commandlinetools`와
  `/opt/homebrew/share/android-ndk`도 자동 탐지한다.
- WASM 패키지를 만들려면 Qt WASM kit와 emsdk의 `Emscripten.cmake`가 필요하다.
- 기본 자동 플랫폼 목록에서 보조 툴체인을 찾지 못한 플랫폼은 건너뛴다. `IIXML_INSTALL_PLATFORMS`로
  명시한 플랫폼은 전제 조건이 없으면 실패한다.

## 실행

```sh
./install.sh
```

스크립트는 항상 루트의 `build/` 디렉터리를 사용한다. 기존 `CMakeCache.txt`가 다른 소스 또는 빌드 경로를 가리키면 오래된 빌드 트리로 보고 `build/`를 제거한 뒤 새로 구성한다.

1. macOS 패키지를 `cmake --fresh -S . -B build`로 구성한다.
2. macOS 라이브러리를 빌드하고 `ctest --test-dir build --output-on-failure`로 테스트를 실행한다.
3. macOS 패키지를 `~/.local/iiXml`과 `~/.local/iiXml/platforms/macos`에 설치한다.
4. iOS 패키지를 Qt iOS toolchain으로 `build/platforms/ios`에 구성하고 Release target을 빌드한다.
5. Android 패키지를 Qt Android toolchain으로 `build/platforms/android`에 구성하고 target을 빌드한다.
6. WASM 패키지를 Qt WASM toolchain으로 `build/platforms/wasm`에 구성하며 `IIXML_BUILD_SHARED=OFF`로 정적 라이브러리를 만든다.
7. 각 cross package를 `~/.local/iiXml/platforms/<platform>`에 설치한다.

임시로 일부 플랫폼만 다시 설치해야 한다면 다음처럼 환경 변수로 제한할 수 있다.

```sh
IIXML_INSTALL_PLATFORMS=ios ./install.sh
```

WASM 또는 Android 툴체인이 기본 위치에 없다면 다음 변수로 지정할 수 있다.

```sh
IIXML_EMSDK_ROOT=~/emsdk ./install.sh
IIXML_ANDROID_QT_PREFIX=~/Qt/6.8.3/android_arm64_v8a ANDROID_SDK_ROOT=~/Library/Android/sdk ./install.sh
```

## 설치 결과

- `~/.local/iiXml/lib`: iiXml 공유 라이브러리
- `~/.local/iiXml/include/iiXml`: `#include <iiXml>`용 umbrella 헤더
- `~/.local/iiXml/include/iiXml.h`: 기존 파일명 기반 umbrella 헤더
- `~/.local/iiXml/include/Src`: 모듈별 공개 헤더
- `~/.local/iiXml/lib/cmake/iiXml`: `iiXmlConfig.cmake`와 export target 파일
- `~/.local/iiXml/platforms/macos`: macOS 전용 패키지 mirror
- `~/.local/iiXml/platforms/ios`: iOS 전용 패키지
- `~/.local/iiXml/platforms/android`: Android 전용 패키지
- `~/.local/iiXml/platforms/wasm`: WASM 전용 정적 패키지

## CMake 사용 예

설치 후 다른 Qt 프로젝트에서는 다음처럼 가져온다.

```cmake
list(PREPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local/iiXml")

find_package(iiXml CONFIG REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE iiXml::iiXml)
```

코드에서는 다음처럼 단일 umbrella 헤더를 사용한다.

```cpp
#include <iiXml>
```

`iiXml::iiXml` imported target은 include path와 Qt `Core` 링크 의존성을 함께 제공한다.
macOS 패키지는 CMake가 일반 package 경로에 자동으로 추가하는 RPATH를 사용한다. `LIBRARY_PATH`로
package 경로가 implicit link directory가 되어 자동 RPATH가 생략되는 환경에서는 imported target이
현재 package prefix의 `lib` 경로만 보완한다. 따라서 중복 RPATH나 별도의 `DYLD_LIBRARY_PATH` 없이 실행된다.
루트 `iiXmlConfig.cmake`는 소비 프로젝트의 `CMAKE_SYSTEM_NAME`을 기준으로 플랫폼 패키지를
먼저 찾는다. iOS, Android, WASM 구성에서는 대응하는
`~/.local/iiXml/platforms/<platform>/lib/cmake/iiXml/iiXmlConfig.cmake`로 위임하므로,
cross build가 macOS 라이브러리를 잘못 링크하는 경로를 막는다.
루트 패키지의 버전 파일은 64비트 네이티브와 32-bit WASM을 함께 선택할 수 있도록
아키텍처 독립형으로 게시하며, 플랫폼별 버전 파일은 각 바이너리의 포인터 크기 검사를 유지한다.
