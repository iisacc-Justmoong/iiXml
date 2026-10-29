# GetHTML

`Src/Parser/GetHTML.h`와 `Src/Parser/GetHTML.cpp`는 `iiXml`을 비롯한 XML 문자열을
받아 HTML 태그 문자열로 판정하고 반환하는 객체이다.

## 책임

- XML 원문은 설치된 `~/.local/SDK`의 `iiXml::iiXml` 타깃과 공개 헤더 `iiXml.h`로 파싱한다.
- 내부 파싱은 `iiXml::Parser::TagParser::ParseAll()`을 사용한다.
- 본문 태그 앞의 XML 선언과 DOCTYPE은 HTML 변환 대상이 아니므로 파싱 전에 제거한다.
- XML 태그명은 HTML 표준 태그나 의미 태그로 변환하지 않고 그대로 출력한다.
- `paragraph`, `bold`, `XML`, `CustomTag` 같은 태그도 커스텀 HTML 태그로 보고
  입력 태그명 그대로 열고 닫는다.
- 알 수 없는 XML 태그도 `div` 같은 대체 태그로 바꾸지 않는다. 예를 들어 입력
  태그명이 `unknownName`이면 `<unknownName>...</unknownName>`로, `OtherWidget`이면
  `<OtherWidget>...</OtherWidget>`로 반환한다.
- 원본 태그명을 별도 `data-xml-tag` 속성으로 복제하지 않는다.
- 파싱 실패 시 이전 HTML 결과를 제거하고 `GetError()`에 실패 사유를 저장한다.

## 사용 예시

```cpp
#include <iiHtmlBlock>

iiHtmlBlock::GetHTML html;

if (html.Parse("<paragraph>Hello <bold>world</bold></paragraph>")) {
    // html.GetTagName() == "paragraph"
    // html.GetHTMLText()
    //     == "<paragraph>Hello <bold>world</bold></paragraph>"
}
```

`ParseXml(const QString&)` 슬롯을 사용하면 성공 시 `Parsed(html)`, 실패 시
`ParseFailed(reason)` 시그널을 내보낸다.

## 빌드와 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
