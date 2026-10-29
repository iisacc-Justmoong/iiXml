# DeleteBlock

`Src/Modifier/DeleteBlock.h`와 `Src/Modifier/DeleteBlock.cpp`는 `DivideBlock`이 판정한
블록 목록 중 지정한 블록을 원본 HTML/iiXml 문자열에서 제거하는 Modifier 모듈이다.

## 책임

- `Parse()`는 HTML/iiXml 문서를 받아 현재 HTML 문자열과 블록 목록을 내부 상태로 저장한다.
- 블록 판정은 `DivideBlock::GetBlockElements()` 결과를 사용한다.
- `Delete(index)`는 현재 블록 목록의 인덱스를 받아 해당 블록의 `raw_begin`부터
  `raw_end`까지를 문서 본문에서 제거한다.
- 삭제 후 남은 문서를 다시 파싱해 `GetBlocks()`가 현재 문서의 블록 목록을 반환하게 한다.
- 삭제된 블록의 스냅샷은 `GetDeletedBlocks()`에 누적한다.
- XML 선언과 DOCTYPE처럼 파서가 본문 range로 취급하지 않는 앞부분은 보존한다.
- 유효하지 않은 인덱스나 삭제 후 재파싱 실패는 상태를 변경하지 않고 `GetError()`에
  실패 사유를 저장한다.

## 사용 예시

```cpp
#include <iiHtmlBlock>

iiHtmlBlock::DeleteBlock deleter;

if (deleter.Parse("<XML><section><p>One</p><p>Two</p></section></XML>")) {
    if (deleter.Delete(1)) {
        // deleter.GetHTMLText() == "<XML><section><p>Two</p></section></XML>"
        // deleter.GetDeletedBlocks()[0].raw == "<p>One</p>"
    }
}
```

`ParseHtml(const QString&)` 슬롯은 성공 시 `Parsed(block_count)`, 실패 시
`DeleteFailed(reason)` 시그널을 내보낸다. `DeleteBlockAt(int)` 슬롯은 성공 시
`Deleted(deleted_count, html)`, 실패 시 `DeleteFailed(reason)` 시그널을 내보낸다.

## 빌드와 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
