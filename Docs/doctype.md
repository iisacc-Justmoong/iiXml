# Doctype 요소

`Src/Elements/Doctype.h`와 `Src/Elements/Doctype.cpp`는 XML 문서 상단 선언을 판정한다.

## 지원 범위

- `<?xml ...?>` 형식의 XML 선언을 문서 상단 선언으로 인식한다.
- `<!Doctype ...>` 형식의 Doctype 선언을 문서 상단 선언으로 인식한다.
- Doctype 이름은 `XML`로 고정하지 않는다. `<!Doctype ABCD>`처럼 유효한 이름이면 통과한다.
- UTF-8 BOM과 선언 앞의 공백은 건너뛴다.
- 선언이 루트 태그 뒤에 나오거나 닫히지 않은 경우에는 실패한다.

## 사용 예

```cpp
#include <iiXml>

iiXml::Elements::Doctype doctype;
auto result = doctype.MatchTop("<!Doctype XML SYSTEM \"iixml.dtd\">\n<XML></XML>");

if (result.Status == iiXml::Elements::DoctypeStatus::Matched &&
    result.Match.has_value()) {
    // result.Match->Kind == iiXml::Elements::DoctypeKind::DoctypeDeclaration
    // result.Match->Raw == "<!Doctype XML SYSTEM \"iixml.dtd\">"
}
```

`MatchTop()`은 실패 시에도 빈 `std::optional`만 반환하지 않고 `Status`와 비어 있지 않은 `Reason`을 포함한 `DoctypeResult`를 반환한다. Qt signal이 필요한 경우에는 문자열 slot인 `MatchTopInput()`을 사용한다.

생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패, 예외 로그를 출력한다.

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
