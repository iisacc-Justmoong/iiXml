# Parser 모듈

`Parser/` 모듈은 iiXml 커스텀 XML 규약의 입력 문자열에서 단일 여는 태그와 닫는 태그를 인식한다.

## 태그 파서

`iixml::parser::tag_parser`는 하나의 태그 쌍을 읽어 `tag_name`과 `value`를 반환한다.

```cpp
#include "iiXml.h"

iixml::parser::tag_parser parser;
auto parsed = parser.parse("<number>숫자</number>");

if (parsed.has_value()) {
    // parsed->tag_name == "number"
    // parsed->value == "숫자"
}
```

## 현재 계약

- 입력은 `<tag>value</tag>` 형태의 단일 태그 쌍이어야 한다.
- 태그 이름은 영문자 또는 `_`로 시작하고, 이후에는 영문자, 숫자, `_`, `-`, `.`, `:`를 사용할 수 있다.
- 입력 문자열의 바깥쪽 공백은 무시하지만, 태그 내부 값의 공백은 보존한다.
- 닫는 태그가 없거나 여는 태그와 닫는 태그 이름이 다르면 `std::nullopt`를 반환한다.
- 속성, 중첩 구조의 의미 분석, XML 이스케이프 해석은 아직 수행하지 않는다.

## 검증

다음 명령으로 빌드와 테스트를 실행한다.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
