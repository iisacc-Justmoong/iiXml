# 설치

`install.sh`는 iiXml을 `~/.local/iiXml` 아래에 설치한다. 설치 위치는 고정이며 다른 prefix를 받지 않는다.

## 실행

```sh
./install.sh
```

스크립트는 항상 루트의 `build/` 디렉터리를 사용한다.

1. `cmake -S . -B build`로 구성한다.
2. `cmake --build build`로 빌드한다.
3. `ctest --test-dir build --output-on-failure`로 테스트를 실행한다.
4. 이전 설치의 `~/.local/iiXml/include/iiXml/` 디렉터리가 있으면 제거한다.
5. `cmake --install build --prefix ~/.local/iiXml`로 설치한다.

## 설치 결과

- `~/.local/iiXml/lib`: iiXml 공유 라이브러리
- `~/.local/iiXml/include/iiXml`: `#include <iiXml>`용 umbrella 헤더
- `~/.local/iiXml/include/iiXml.h`: 기존 파일명 기반 umbrella 헤더
- `~/.local/iiXml/include/Src`: 모듈별 공개 헤더
- `~/.local/iiXml/lib/cmake/iiXml`: `iiXmlConfig.cmake`와 export target 파일

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
