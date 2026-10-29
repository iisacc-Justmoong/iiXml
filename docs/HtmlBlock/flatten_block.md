# FlattenBlock

`Src/Modifier/FlattenBlock.h`와 `Src/Modifier/FlattenBlock.cpp`는 문서 안의 모든 블록을
하나의 상위 그룹 블록 레이어로 감싸는 Modifier 모듈이다.

## 책임

- `Parse()`는 HTML/iiXml 문서를 받아 `DivideBlock`으로 블록 요소를 판정한 뒤 전체
  블록을 하나의 `LayerBlock`으로 묶는다.
- 이미 나뉜 블록 목록이 있으면 `SetBlocks()`로 전달하고 `LayerAll()`로 같은 레이어를
  만들 수 있다.
- `LayerBlock`은 단일 블록처럼 `is_block == true`로 취급한다.
- 개별 블록은 삭제하거나 합치지 않는다. `LayerBlock::elements`에 각 블록 스냅샷을
  그대로 보존하고, `LayerBlock::source_indexes`로 원본 블록 위치를 보관한다.
- `LayerBlock::raw`와 `LayerBlock::value`는 각 블록의 원문과 값을 문서 순서대로 이어
  붙인 레이어 표현이다. 이는 HTML 재작성 결과가 아니라 그룹 레이어의 내부 표현이다.
- `raw_begin`, `raw_end`, `value_begin`, `value_end`는 레이어에 포함된 블록들이 차지하는
  전체 offset 범위를 나타낸다.
- 블록이 하나도 없거나 파싱에 실패하면 레이어를 만들지 않고 `GetError()`에 실패
  사유를 저장한다.

## 사용 예시

```cpp
#include <iiHtmlBlock>

iiHtmlBlock::FlattenBlock flattener;

if (flattener.Parse("<XML><section><p>One</p><p>Two</p></section></XML>")) {
    const iiHtmlBlock::FlattenBlock::LayerBlock* layer = flattener.GetLayerBlock();
    // layer->is_block == true
    // layer->elements는 section, p, p 블록을 개별 블록으로 보존한다.
}
```

## 빌드와 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
