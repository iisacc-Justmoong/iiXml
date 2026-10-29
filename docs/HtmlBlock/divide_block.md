# DivideBlock

`Src/Modifier/DivideBlock.h`와 `Src/Modifier/DivideBlock.cpp`는 파싱된 HTML/iiXml 태그
range를 순회하면서 각 요소가 블록 단위인지 판정하는 Modifier 모듈이다.

## 책임

- 내부 파싱은 `Src/Parser/GetTagInfo.*`의 range 목록을 사용한다.
- `section`, `div`, `p`, `article`, `nav`, `ul`, `li` 같은 기본 HTML 블록 태그는
  블록 요소로 판정한다.
- 태그명 판정은 ASCII 대소문자를 구분하지 않는다.
- `style="display: block"` 또는 `display=block`처럼 명시적 display 값이 있으면
  기본 태그 판정보다 우선한다.
- 명시적 display 값이 `inline`이면 `div` 같은 기본 블록 태그도 블록으로 보지 않는다.
- 알 수 없는 커스텀 태그는 기본적으로 인라인으로 보고, 명시적 `display: block`
  또는 `display=block`이 있을 때만 블록으로 판정한다.
- 파싱 실패 시 이전 요소 목록을 제거하고 `GetError()`에 실패 사유를 저장한다.

## 사용 예시

```cpp
#include <iiHtmlBlock>

iiHtmlBlock::DivideBlock divider;

if (divider.Parse("<XML><section><span>text</span></section></XML>")) {
    // divider.GetElements()[1].tag_name == "section"
    // divider.GetElements()[1].is_block == true
    // divider.GetBlockElements().size() == 1
}
```

이미 `GetTagInfo::RangeInfo`를 가지고 있다면 전체 입력을 다시 저장하지 않고
`iiHtmlBlock::DivideBlock::IsBlockElement(range)`로 같은 규칙을 적용할 수 있다.

## 빌드와 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
