# CombineBlock

`Src/Modifier/CombineBlock.h`와 `Src/Modifier/CombineBlock.cpp`는 `DivideBlock`으로
나뉜 블록 목록에서 여러 블록을 선택하고, 선택 묶음을 하나의 논리 블록으로 병합
취급하는 Modifier 모듈이다.

## 책임

- 입력 블록은 `SetBlocks()`로 받으며 보통 `DivideBlock::GetBlockElements()` 결과를
  전달한다.
- `Select()`, `SelectRange()`, `Deselect()`, `ClearSelection()`으로 병합할 블록
  인덱스를 관리한다.
- `CombineSelected()`는 선택된 두 개 이상의 블록을 `CombinedBlock` 하나로 저장한다.
- 병합은 원본 HTML 문자열을 재작성하지 않는다. 원본 블록 목록은 유지하고, 병합
  결과에 선택 인덱스와 선택된 블록 스냅샷을 함께 저장한다.
- `CombinedBlock::raw`와 `CombinedBlock::value`는 선택된 블록의 원문과 값을 선택
  순서가 아니라 원본 인덱스 순서로 이어 붙인다.
- `raw_begin`, `raw_end`, `value_begin`, `value_end`는 선택된 블록들이 차지하는 전체
  offset 범위를 나타낸다.
- 선택 블록이 두 개 미만이거나 인덱스가 범위를 벗어나면 실패하고 `GetError()`에
  사유를 저장한다.

## 사용 예시

```cpp
#include <iiHtmlBlock>

iiHtmlBlock::DivideBlock divider;
iiHtmlBlock::CombineBlock combiner;

if (divider.Parse("<XML><section><p>One</p><p>Two</p></section></XML>")) {
    combiner.SetBlocks(divider.GetBlockElements());
    combiner.SelectRange(1, 2);

    if (combiner.CombineSelected()) {
        // combiner.GetCombinedBlocks()[0].source_indexes == {1, 2}
        // combiner.GetCombinedBlocks()[0].raw == "<p>One</p><p>Two</p>"
    }
}
```

## 빌드와 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
