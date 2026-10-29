# BlockHTMLSerializer

`Src/Modifier/BlockHTMLSerializer.h`와 `Src/Modifier/BlockHTMLSerializer.cpp`는
`DivideBlock`의 분할 결과와 `CombineBlock`의 병합 결과를 HTML 문자열 조각으로
직렬화하는 Modifier 모듈이다.

## 책임

- `SerializeBlocks()`는 `DivideBlock::ElementInfo` 목록을 문서 순서로 직렬화한다.
- 중첩 블록이 함께 들어온 경우 이미 출력된 상위 블록 range에 포함된 하위 블록은
  중복 출력하지 않는다.
- `SerializeCombinedBlocks()`는 `CombineBlock::CombinedBlock` 목록을 논리 병합 블록
  조각으로 직렬화한다.
- `SerializeMergedBlocks()`는 원본 분할 블록 목록에 병합 결과를 적용한 뒤 직렬화한다.
  병합된 source index는 원본 블록으로 다시 출력하지 않고 `CombinedBlock::raw`로 대체한다.
- 병합 source index가 원본 블록 범위를 벗어나거나 range가 뒤집힌 입력은 실패하고
  `GetError()`에 실패 사유를 저장한다.
- 이 객체는 루트 태그나 비블록 인라인 노드를 새로 생성하지 않는다. 입력된 블록/병합 결과가
  가진 `raw` 조각을 안정적으로 HTML 문자열로 재구성하는 책임만 가진다.

## 사용 예시

```cpp
#include <iiHtmlBlock>

iiHtmlBlock::DivideBlock divider;
iiHtmlBlock::CombineBlock combiner;
iiHtmlBlock::BlockHTMLSerializer serializer;

if (divider.Parse("<XML><p>One</p><p>Two</p></XML>")) {
    combiner.SetBlocks(divider.GetBlockElements());
    combiner.SelectRange(0, 1);

    if (combiner.CombineSelected()) {
        serializer.SerializeCombinedBlocks(combiner.GetCombinedBlocks());
        // serializer.GetHTMLText() == "<p>One</p><p>Two</p>"
    }
}
```

분할 결과만 직렬화할 수도 있다.

```cpp
serializer.SerializeBlocks(divider.GetBlockElements());
```

## 빌드와 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
