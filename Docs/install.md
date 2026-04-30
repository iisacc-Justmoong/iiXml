# 설치

`install.sh`는 iiXml을 `~/.local` 아래에 설치한다. 설치 위치는 고정이며 다른 prefix를 받지 않는다.

## 실행

```sh
./install.sh
```

스크립트는 항상 루트의 `build/` 디렉터리를 사용한다.

1. `cmake -S . -B build`로 구성한다.
2. `cmake --build build`로 빌드한다.
3. `ctest --test-dir build --output-on-failure`로 테스트를 실행한다.
4. `cmake --install build --prefix ~/.local`로 설치한다.

## 설치 결과

- `~/.local/lib`: iiXml 공유 라이브러리
- `~/.local/include/iiXml`: 공개 헤더와 모듈 헤더
- `~/.local/lib/cmake/iiXml`: `iiXmlConfig.cmake`와 export target 파일

## CMake 사용 예

설치 후 다른 Qt 프로젝트에서는 다음처럼 가져온다.

```cmake
list(PREPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local")

find_package(iiXml CONFIG REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE iiXml::iiXml)
```

코드에서는 둘 중 편한 방식을 사용할 수 있다.

```cpp
#include "iiXml.h"
```

```cpp
#include <iiXml/iiXml.h>
```

`iiXml::iiXml` imported target은 include path와 Qt `Core` 링크 의존성을 함께 제공한다.
