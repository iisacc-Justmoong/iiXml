# DOCTYPE 요소

`Src/Elements/DOCTYPE.h`와 `Src/Elements/DOCTYPE.cpp`는 XML 문서 상단 선언을 판정한다.

## 지원 범위

- `<?xml ...?>` 형식의 XML 선언을 문서 상단 선언으로 인식한다.
- `<!DOCTYPE ...>` 형식의 DOCTYPE 선언을 문서 상단 선언으로 인식한다.
- UTF-8 BOM과 선언 앞의 공백은 건너뛴다.
- 선언이 루트 태그 뒤에 나오거나 닫히지 않은 경우에는 실패한다.

## 사용 예

```cpp
#include "iiXml.h"

iiXml::elements::DOCTYPE doctype;
auto matched = doctype.match_top("<!DOCTYPE XML SYSTEM \"iixml.dtd\">\n<XML></XML>");

if (matched.has_value()) {
    // matched->kind == iiXml::elements::doctype_kind::doctype_declaration
    // matched->raw == "<!DOCTYPE XML SYSTEM \"iixml.dtd\">"
}
```

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
