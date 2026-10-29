# Parser 모듈

`Src/Parser/` 모듈은 iiXml 커스텀 XML 규약의 입력 문자열에서 단일 여는 태그와 닫는 태그를 인식한다.

## 태그 파서

`iiXml::Parser::TagParser`는 하나의 태그 쌍을 읽어 `TagName`과 `Value`를 반환한다.

```cpp
#include <iiXml>

iiXml::Parser::TagParser parser;
auto parsed = parser.Parse("<number>숫자</number>");

if (parsed.has_value()) {
    // parsed->TagName == "number"
    // parsed->Value == "숫자"
}
```

실패 사유와 위치가 필요한 호출자는 `ParseResult()`를 사용한다. 이 API는 `std::optional`만 반환하지 않고 상태, payload, 진단 정보를 구조화해서 제공한다.

```cpp
auto result = parser.ParseResult("<number>숫자</text>");

if (result.Status == iiXml::Parser::TagParseStatus::ClosingTagMismatch) {
    // result.Token.has_value() == false
    // result.Diagnostic.Reason == "closing tag mismatch"
    // result.Diagnostic.Offset, Line, Column, Context 사용 가능
}
```

`ParseAll()`은 입력 안의 태그 쌍을 모두 읽어 `std::vector<TagNode>`로 반환한다. 각 노드는 `Range`, `Fields`, `Children`을 가진다. 이 API는 `iiXml::Elements::OpenTag`의 완화된 교차 종료 규칙을 사용하므로, `<a><b></a></b>`에서도 `a`와 `b`를 모두 보존한다. 태그 수는 두 개로 제한하지 않으며, 여러 스타일 태그와 문단 태그가 교차해도 모든 태그를 반환한다.

```cpp
iiXml::Parser::TagParser parser;
std::string_view input = "<a><b></a></b>";
auto parsed = parser.ParseAll(input);

if (parsed.has_value()) {
    // (*parsed)[0].Range.TagName == "a"
    // input.substr((*parsed)[0].Range.RawBegin,
    //     (*parsed)[0].Range.RawEnd - (*parsed)[0].Range.RawBegin) == "<a><b></a>"
    // (*parsed)[1].Range.TagName == "b"
    // input.substr((*parsed)[1].Range.RawBegin,
    //     (*parsed)[1].Range.RawEnd - (*parsed)[1].Range.RawBegin) == "<b></a></b>"
}
```

`ParseAllResult()`는 전체 태그 트리 파싱을 Result 형태로 반환한다.

```cpp
auto result = parser.ParseAllResult(input);

if (result.Status == iiXml::Parser::TagTreeParseStatus::Parsed &&
    result.Nodes.has_value()) {
    // (*result.Nodes)[0].Range.TagName == "contents"
}
```

`ParseAllDocument()`와 `ParseAllDocumentResult()`는 range 결과를 통째로 저장하거나 다른 객체에 멤버로 임베딩할 수 있는 `TagDocument`로 반환한다. `TagDocument`는 원본 입력을 `Source`에 한 번만 소유하고, `Nodes`에는 기존 `TagNode` range 트리를 그대로 담는다. 따라서 큰 문서에서도 태그마다 `Raw`/`Value` 문자열을 반복 복제하지 않으며, 호출자는 원본 입력 문자열의 수명을 별도로 관리하지 않아도 된다.

```cpp
auto result = parser.ParseAllDocument("<a><b></a></b>");

if (result.has_value()) {
    const auto& first = result->Nodes[0];
    // first.Range.TagName == "a"
    // result->ValueView(first) == "<b>"
    // result->RawView(first) == "<a><b></a>"
}
```

정상 중첩된 입력은 `Children`으로 계층화된다. 여는 태그의 attribute는 `Fields`로 제공하며, attribute 값도 원본 입력의 offset 범위로 보존한다.

```cpp
std::string_view input =
    "<contents id=\"abc\"><body><paragraph order=1 visible=true>one</paragraph></body></contents>";
auto parsed = parser.ParseAll(input);

if (parsed.has_value()) {
    const auto& contents = (*parsed)[0];
    // contents.Range.TagName == "contents"
    // contents.Fields[0].Name == "id"
    // input.substr(contents.Fields[0].ValueBegin,
    //     contents.Fields[0].ValueEnd - contents.Fields[0].ValueBegin) == "abc"
    // contents.Children[0].Range.TagName == "body"
    // contents.Children[0].Children[0].Range.TagName == "paragraph"
    // contents.Children[0].Children[0].Fields[0].ValueType
    //     == iiXml::Elements::InlinePropertyType::IntType
    // contents.Children[0].Children[0].Fields[1].ValueType
    //     == iiXml::Elements::InlinePropertyType::BoolType
}
```

## 현재 계약

- `Parse()` 입력은 `<tag>value</tag>` 형태의 단일 태그 쌍이어야 한다.
- `ParseResult()`는 `TagParseResult`를 반환한다. `Status`는 `Parsed`, `EmptyInput`, `MissingOpeningBracket`, `OpeningTagNotClosed`, `InvalidTagName`, `InputShorterThanClosingTag`, `ClosingTagMismatch`, `ExceptionThrown` 중 하나다.
- `TagParseResult::Token`은 성공 시에만 존재한다. 실패 시에는 `Diagnostic`의 `Reason`, `Offset`, `Line`, `Column`, `Context`로 구조화된 실패 정보를 제공한다.
- `ParseAll()`은 여러 태그를 계층 노드로 반환하고, 각 노드의 `Range.ValueBegin/ValueEnd`와 `Range.RawBegin/RawEnd`에는 자기 닫기 태그까지 전진한 원본 offset 범위를 보존한다.
- `ParseAllResult()`는 `TagTreeParseResult`를 반환한다. `Status`는 `Parsed`, `OpenTagParserRejected`, `HierarchyBuildFailed`, `ExceptionThrown` 중 하나이며, 성공 시 `Nodes`에 계층 노드를 담는다.
- `ParseAllDocument()`는 `TagDocument`를 반환한다. `TagDocument::Source`는 입력 전체를 한 번 소유하고, `TagDocument::Nodes`는 그 입력을 가리키는 range 트리를 담는다.
- `ParseAllDocumentResult()`는 `TagDocumentResult`를 반환한다. 성공 시 `Document`가 존재하고, 실패 시 `Status`와 `Diagnostic`은 range 파서 실패 정보를 그대로 전달한다.
- `TagDocument::RawView()`, `ValueView()`, `FieldNameView()`, `FieldValueView()`는 `Source`와 range offset으로 `std::string_view`를 만들어 반환한다. 이 view는 `TagDocument` 객체가 살아 있는 동안만 유효하다.
- 정상 포함 관계가 성립하는 태그는 `Children`에 배치한다. 교차 종료 때문에 포함 관계가 성립하지 않는 태그들은 같은 계층의 독립 노드로 남긴다.
- 여는 태그의 attribute는 `Fields`에 배치하며, 필드 이름과 값 범위, `StringType`/`IntType`/`FloatType`/`BoolType` 타입 정보를 제공한다.
- 열린 태그 프로세스는 짝 닫기 태그를 만나기 전까지 완료되지 않으며, 완료된 뒤에만 `TagNode`의 `Range`와 `Fields`가 확정된다.
- 태그 매칭은 태그 이름 기준이며, 같은 이름이 여러 번 열려 있으면 가장 최근에 열린 같은 이름 태그가 먼저 닫힌다.
- 태그 이름은 영문자 또는 `_`로 시작하고, 이후에는 영문자, 숫자, `_`, `-`, `.`, `:`를 사용할 수 있다.
- 입력 문자열의 바깥쪽 공백은 무시하지만, 태그 내부 값의 공백은 보존한다.
- 닫는 태그가 없거나 여는 태그와 닫는 태그 이름이 다르면 `std::nullopt`를 반환한다.
- `ParseAll()`에서 닫히지 않은 열린 태그가 남으면 `std::nullopt`를 반환한다.
- 속성의 의미 분석과 XML 이스케이프 해석은 아직 수행하지 않는다.
- 생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패 로그를 출력한다.

## 검증

다음 명령으로 빌드와 테스트를 실행한다.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
