# Inline tag mutation 모듈

`Src/Mutation/InlineTagMutation.*`는 iiXml source fragment에서 인라인 스타일 태그의 logical 범위 mutation을 수행하는 보조 모듈이다.

이 모듈은 다음 책임을 가진다.

- source boundary를 visible logical offset으로 변환
- visible logical offset을 source boundary로 역변환
- 기존 인라인 스타일 태그를 보존한 채 선택 구간에 새 스타일 coverage 적용
- 재구성된 fragment가 다시 iiXml 문서로 파싱 가능한지 검증

## 예시

```cpp
#include <iiXml>

iiXml::Mutation::InlineTagMutation mutation;
iiXml::Mutation::InlineTagMutationOptions options;
options.OrderedInlineTagNames = {"bold", "italic", "underline"};
options.LogicalBreakTagNames = {"break", "hr"};

const QString source = "<bold>Alpha Beta</bold>";
const int start = mutation.LogicalOffsetForSourceBoundary(source, 8, options);
const int end = mutation.LogicalOffsetForSourceBoundary(source, 17, options);
const auto result = mutation.ApplyLogicalStyleRange(source, "italic", start, end, options);

// result.SourceText == "<bold>Al<italic>pha</italic></bold><italic> Beta</italic>"
// result.Valid == true
```

## 현재 계약

- 인라인 스타일 우선순서는 `OrderedInlineTagNames` 순서를 따른다.
- `LogicalBreakTagNames`에 포함된 태그는 visible logical 길이 1로 계산한다.
- 기존 인라인 스타일 태그는 재직렬화 과정에서 source에서 제거한 뒤 coverage 상태에 따라 다시 생성한다.
- 결과 fragment의 parse validity는 내부적으로 `TagParser::ParseAllDocumentResult()`로 검증한다.
- 태그 의미 정책 자체는 호출자가 제공한다. 이 모듈은 `bold`나 `italic`의 의미를 내장하지 않는다.

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
