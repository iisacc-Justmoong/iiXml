# BlockRangeTracker

`Src/Modifier/BlockRangeTracker.h`와 `Src/Modifier/BlockRangeTracker.cpp`는
`DivideBlock`이 판정한 블록을 원문 range 단위로 추적하는 Modifier 모듈이다.

## 책임

- `Parse()`는 HTML/iiXml 문서를 받아 블록 목록을 만들고 각 블록에 추적 ID를 부여한다.
- 공개 `TrackedBlock::raw_begin`, `value_begin`, `value_end`, `raw_end`는 XML 선언과
  DOCTYPE을 포함한 원문 전체 기준 offset이다.
- `TrackedBlock::element` 안의 `DivideBlock::ElementInfo`는 기존 API와 같이 파서 본문
  기준 offset을 유지한다.
- `Update()`는 새 문서를 다시 파싱하고, 기존 블록과 새 블록을 range와 원문 조각 기준으로
  맞춰 가능한 경우 기존 추적 ID를 유지한다.
- `GetBlockById()`와 `GetBlockByRange()`로 특정 블록을 조회한다.
- `GetInnermostBlockAt()`은 주어진 offset을 포함하는 블록 중 가장 좁은 블록을 반환한다.
- `GetBlocksOverlapping()`은 지정한 range와 겹치는 모든 추적 블록을 문서 순서로 반환한다.
- `SetBlocks()`는 이미 나뉜 `DivideBlock::ElementInfo` 목록을 직접 추적 대상으로 등록한다.
- `Update()` 실패 시 이전 추적 상태는 유지하고 `GetError()`에 실패 사유를 저장한다.

## 사용 예시

```cpp
#include <iiHtmlBlock>

iiHtmlBlock::BlockRangeTracker tracker;

if (tracker.Parse("<XML><section><p>One</p></section></XML>")) {
    const std::size_t offset = tracker.GetHTMLText().find("One");
    const iiHtmlBlock::BlockRangeTracker::TrackedBlock* block =
        tracker.GetInnermostBlockAt(offset);

    // block->element.tag_name == "p"
    // block->raw_begin/raw_end는 원문 전체 기준 range이다.
}
```

문서가 변경된 뒤에는 `Update()`를 사용한다. 원문과 태그 값이 같은 블록은 range가
이동해도 기존 ID를 유지할 수 있다.

```cpp
tracker.Parse("<XML><p>One</p><p>Two</p></XML>");
const std::size_t one_id = tracker.GetTrackedBlocks()[0].id;

tracker.Update("<XML><p>Zero</p><p>One</p><p>Two</p></XML>");
// 값이 "One"인 블록은 가능한 경우 one_id를 유지한다.
```

## 빌드와 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
