# iiXmlToHTML

`Src/Modifier/iiXmlToHTML.h`와 `Src/Modifier/iiXmlToHTML.cpp`는 iiXml/XML 입력을
HTML 문자열로 변환하는 Modifier 모듈이다.

## 책임

- 변환 구현은 `Src/Parser/GetHTML.*`의 HTML 판정 규칙을 사용한다.
- 태그명은 변환하지 않고 입력 이름 그대로 커스텀 HTML 태그로 유지한다.
- 변환 실패 시 이전 결과를 제거하고 `GetError()`에 실패 사유를 저장한다.
- `ConvertXml(const QString&)` 슬롯은 성공 시 `Converted(html)`, 실패 시
  `ConvertFailed(reason)` 시그널을 내보낸다.

## 사용 예시

```cpp
#include <iiHtmlBlock>

iiHtmlBlock::iiXmlToHTML converter;

if (converter.Convert("<XML><unknownName>value</unknownName></XML>")) {
    // converter.GetHTMLText() == "<XML><unknownName>value</unknownName></XML>"
}
```

## 빌드와 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
