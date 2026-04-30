# DOCTYPE 요소

`Src/Elements/DOCTYPE.h`와 `Src/Elements/DOCTYPE.cpp`는 XML 문서 상단 선언을 판정한다.

## 지원 범위

- `<?xml ...?>` 형식의 XML 선언을 문서 상단 선언으로 인식한다.
- `<!DOCTYPE ...>` 형식의 DOCTYPE 선언을 문서 상단 선언으로 인식한다.
- DOCTYPE 이름은 `XML`로 고정하지 않는다. `<!DOCTYPE ABCD>`처럼 유효한 이름이면 통과한다.
- UTF-8 BOM과 선언 앞의 공백은 건너뛴다.
- 선언이 루트 태그 뒤에 나오거나 닫히지 않은 경우에는 실패한다.

## 사용 예

```cpp
#include "iiXml.h"

iiXml::elements::DOCTYPE doctype;
auto result = doctype.match_top("<!DOCTYPE XML SYSTEM \"iixml.dtd\">\n<XML></XML>");

if (result.status == iiXml::elements::doctype_status::matched &&
    result.match.has_value()) {
    // result.match->kind == iiXml::elements::doctype_kind::doctype_declaration
    // result.match->raw == "<!DOCTYPE XML SYSTEM \"iixml.dtd\">"
}
```

`match_top()`은 실패 시에도 빈 `std::optional`만 반환하지 않고 `status`와 비어 있지 않은 `reason`을 포함한 `doctype_result`를 반환한다. 기존처럼 매치 여부만 필요한 내부 코드에는 `match_top_match()`를 사용할 수 있다.

생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패, 예외 로그를 출력한다.

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
