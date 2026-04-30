# FileParser

`Src/Parser/FileParser.h`와 `Src/Parser/FileParser.cpp`는 파일 경로를 입력으로 받아 파일 내용을 기존 태그 파서에 전달한다.

## 반환 형식

`iiXml::parser::FileParser::parse_file()`은 파일 전체를 읽은 뒤 `iiXml::parser::tag_parser::parse()`에 넘긴다.

```cpp
std::optional<iiXml::parser::tag_value>
```

파일을 열 수 없거나 파일 내용이 태그 파서를 통과하지 못하면 `std::nullopt`를 반환한다.

생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패 로그를 출력한다.

## 사용 예

```cpp
#include "iiXml.h"

iiXml::parser::FileParser parser;
auto parsed = parser.parse_file("input.xml");

if (parsed.has_value()) {
    // parsed->tag_name
    // parsed->value
}
```

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
