# ClosedTag

`Src/Elements/ClosedTag.h`와 `Src/Elements/ClosedTag.cpp`는 입력의 첫 실제 마크업이 `</`로 시작하는 즉시 닫힌 태그를 감지하는 요소 모듈이다.

## 감지 규칙

`iiXml::Elements::ClosedTag`는 선행 공백을 제외한 첫 토큰이 `</tag>` 형태인지 확인한다. 같은 입력 범위 안에서 `<tag`로 시작하는 여는 태그가 먼저 나오지 않고 곧바로 닫는 태그가 나오면 `ClosedTagMatch::Flag`를 `true`로 둔다.

```cpp
#include <iiXml>

iiXml::Elements::ClosedTag closed_tag;
auto matched = closed_tag.MatchImmediate("</XML>");

if (matched.has_value() && matched->Flag) {
    // matched->TagName == "XML"
    // matched->Raw == "</XML>"
}
```

`MatchImmediate()`는 원본 입력을 복사해 다시 만들지 않도록 `RawBegin`과 `RawEnd`에 source offset 기준의 범위를 함께 제공한다. `SourceOffset` 인자를 넘기면 큰 입력의 부분 문자열을 검사할 때도 원본 기준 위치를 유지할 수 있다.

## Qt 시그널

`ParseClosedTag(const QString& Input)` 슬롯은 판정 결과를 시그널로 방출한다.

- 즉시 닫힌 태그를 감지하면 `ClosedTagFlagged(true)`와 `ClosedTagParsed(TagName, Raw)`를 방출한다.
- 즉시 닫힌 태그가 아니거나 닫힌 태그 이름이 올바르지 않으면 `ClosedTagFlagged(false)`와 `ClosedTagRejected(Reason)`을 방출한다.

```cpp
iiXml::Elements::ClosedTag closed_tag;

QObject::connect(&closed_tag, &iiXml::Elements::ClosedTag::ClosedTagFlagged,
    [](bool flag) {
        // flag == true 이면 입력이 즉시 닫힌 태그다.
    });

closed_tag.ParseClosedTag("</XML>");
```

## 실패 조건

- 입력이 비어 있으면 실패한다.
- 선행 공백 이후 첫 토큰이 `</`로 시작하지 않으면 실패한다.
- 닫힌 태그가 `>`로 끝나지 않으면 실패한다.
- 닫힌 태그 이름이 비어 있거나 이름 규칙을 통과하지 못하면 실패한다.
- 닫힌 태그 이름 뒤와 `>` 사이에는 공백만 허용한다.

생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패, 예외 로그를 출력한다.

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
