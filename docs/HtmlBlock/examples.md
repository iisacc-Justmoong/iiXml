# 예제

`Example/ParsedXmlHtmlExample.cpp`는 `#include <iiHtmlBlock>` 하나로 파싱된 XML과
HTML 블록 API를 함께 사용하는 흐름을 보여준다.

## ParsedXmlHtmlExample

예제는 다음 순서로 동작한다.

- `iiHtmlBlock::GetTagInfo`로 XML/iiXml 태그 range와 루트 태그를 조회한다.
- `iiHtmlBlock::GetHTML`로 XML/iiXml 입력을 HTML 문자열로 변환한다.
- `iiHtmlBlock::DivideBlock`으로 변환된 HTML 요소 중 블록 요소를 판정한다.
- `iiHtmlBlock::BlockRangeTracker`로 블록을 원문 range와 추적 ID 기준으로 조회한다.
- `iiHtmlBlock::BlockHTMLSerializer`로 분할/병합 결과를 HTML 문자열로 직렬화한다.
- `iiHtmlBlock::DeleteBlock`으로 선택한 블록을 HTML 문자열에서 제거한다.
- `iiHtmlBlock::CombineBlock`으로 선택한 여러 블록을 하나의 논리 블록으로 병합 취급한다.
- `iiHtmlBlock::FlattenBlock`으로 전체 블록을 하나의 상위 레이어 블록으로 묶는다.

## 실행

```sh
cmake -S . -B build
cmake --build build --target ParsedXmlHtmlExample
./build/ParsedXmlHtmlExample
```

예제는 CTest에도 등록되어 있어 전체 검증에서 함께 실행된다.

```sh
ctest --test-dir build --output-on-failure
```
