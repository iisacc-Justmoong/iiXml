# Qt Object API

iiXml의 주요 객체는 Qt 프로젝트에서 직접 연결할 수 있도록 `QObject`를 상속한다.

## 공통 정책

- Qt 6.8.3 `Core` 모듈을 기준으로 빌드한다.
- CMake는 `/Volumes/Storage/Qt/6.8.3/macos`를 Qt 탐색 경로 앞에 추가한다.
- 동기 API와 Qt 슬롯/시그널 API를 함께 제공한다. 검증/입력 API의 실패는 상태와 비어 있지 않은 사유 문자열로 반환하거나 방출한다.
- 모든 라이브러리 객체 생성자와 public 메서드는 `QDebug`/`qDebug()`로 디버깅 로그를 출력한다.

## TagParser

`iiXml::Parser::TagParser`는 `QObject`를 상속한다.

슬롯:

- `ParseTag(const QString& input)`

동기 API:

- `Parse(std::string_view Input)`
- `ParseResult(std::string_view Input)`
- `ParseAll(std::string_view Input)`
- `ParseAllResult(std::string_view Input)`
- `ParseAllDocument(std::string_view Input)`
- `ParseAllDocumentResult(std::string_view Input)`

`ParseAll()`은 `OpenTag`의 교차 종료 정책을 사용하여 `<a><b></a></b>`에서도 `a`와 `b`를 모두 반환한다. 반환 타입은 `std::vector<TagNode>`이며, 각 노드는 `Range`, `Fields`, `Children`을 가진다. `Value/Raw` 문자열과 field 값은 복사하지 않고 원본 입력의 offset 범위로 제공한다.

Result API는 `TagParseResult`와 `TagTreeParseResult`를 반환한다. 각 Result는 `Status`, 성공 payload(`Token` 또는 `Nodes`), 실패 위치를 담은 `Diagnostic`을 가진다.

`ParseAllDocument()`와 `ParseAllDocumentResult()`는 range 기반 결과를 `TagDocument`로 감싼다. `TagDocument`는 입력 원문을 `Source`에 한 번만 소유하고 `Nodes`에 range 트리를 담기 때문에, 다른 프로젝트의 도메인 객체에 통째로 멤버로 보관할 수 있다. 값 접근은 `RawView()`, `ValueView()`, `FieldNameView()`, `FieldValueView()`로 수행한다.

시그널:

- `TagParsed(const QString& TagName, const QString& Value)`
- `ParseFailed(const QString& Reason)`

## FileParser

`iiXml::Parser::FileParser`는 `QObject`를 상속한다.

슬롯:

- `ParseFileInput(const QString& FilePath)`

시그널:

- `TagParsed(const QString& TagName, const QString& Value)`
- `ParseFailed(const QString& Reason)`

## Doctype

`iiXml::Elements::Doctype`는 `QObject`를 상속한다.

슬롯:

- `MatchTopInput(const QString& input)`

시그널:

- `DoctypeMatched(Doctype::Kind Kind, const QString& Raw)`
- `DoctypeRejected(const QString& Reason)`
- `XmlDeclarationMatched(const QString& Raw)`
- `DoctypeDeclarationMatched(const QString& Raw)`

## OpenTag

`iiXml::Elements::OpenTag`는 `QObject`를 상속한다.

동기 API:

- `CloseOpenTag(std::vector<std::string>& OpenTags, std::string_view ClosingTagName)`
- `ParseOpenTags(std::string_view Input)`

`CloseOpenTag()`는 열린 태그 스택에서 닫는 태그 이름과 같은 항목을 찾아 제거한다. 최상단 태그만 닫는 엄격한 XML 정책이 아니라, `<a><b></a></b>` 같은 iiXml 교차 종료 구조를 허용하는 정책이다. `ParseOpenTags()`는 같은 정책으로 모든 태그를 열림 순서대로 반환하고, 각 항목의 `RawBegin/ValueBegin/ValueEnd/RawEnd`에 원본 offset 범위를 보존한다.

## ClosedTag

`iiXml::Elements::ClosedTag`는 `QObject`를 상속한다.

슬롯:

- `ParseClosedTag(const QString& Input)`

동기 API:

- `IsImmediateClosedTag(std::string_view Input)`
- `MatchImmediate(std::string_view Input, std::size_t SourceOffset = 0)`

시그널:

- `ClosedTagFlagged(bool Flag)`
- `ClosedTagParsed(const QString& TagName, const QString& Raw)`
- `ClosedTagRejected(const QString& Reason)`

`ClosedTag`는 입력의 선행 공백 이후 첫 마크업이 `</tag>`인지를 판정한다. 여는 태그 없이 즉시 닫힌 태그가 들어오면 `ClosedTagFlagged(true)`를 방출하고, 아니라면 `ClosedTagFlagged(false)`와 실패 사유를 방출한다.

## InlineProperties

`iiXml::Elements::InlineProperties`는 `QObject`를 상속한다.

동기 API:

- `Parse(std::string_view opening_tag, std::size_t source_offset = 0)`

`Parse()`는 여는 태그 안의 다중 attribute를 읽고, 각 속성의 이름 범위, 값 범위, `StringType`/`IntType`/`FloatType`/`BoolType` 타입 정보를 반환한다.

## InputValidator

`iiXml::Writer::InputValidator`는 `QObject`를 상속한다.

슬롯:

- `ValidateInput(const QString& input)`

시그널:

- `ValidationFinished(InputValidator::ValidationExit Result)`
- `ValidationFailed(InputValidator::ValidationExit Result, const QString& Reason)`
- `ValidXml()`
- `InvalidXmlFile()`
- `InvalidTagClosure()`
- `ExceptionThrown()`

## GetFile

`iiXml::Writer::GetFile`은 `QObject`를 상속한다.

슬롯:

- `ReadFile(const QString& FilePath)`
- `ReadXml(const QString& Input)`

시그널:

- `Parsed(const QString& TagName, const QString& Value)`
- `Failed(GetFile::Status Status, const QString& Reason)`
- `FileReadFailed()`
- `InvalidXmlFile()`
- `InvalidTagClosure()`
- `ParserRejected()`
- `ExceptionThrown()`

## GetStringToken

`iiXml::Writer::GetStringToken`은 `QObject`를 상속한다.

`QString` 문자열을 입력으로 받지만, 태그 파서에 바로 넘기지는 않는다. `Doctype` 판정과 `InputValidator` 전체 XML 검증을 통과한 뒤 루트 태그를 파서로 전달한다.

슬롯:

- `ReadString(const QString& input)`

시그널:

- `Parsed(const QString& TagName, const QString& Value)`
- `Failed(GetStringToken::Status Status, const QString& Reason)`
- `ParseFailed(const QString& Reason)`

## 사용 예

```cpp
#include <iiXml>

iiXml::Parser::TagParser parser;

QObject::connect(&parser, &iiXml::Parser::TagParser::TagParsed,
    [](const QString& TagName, const QString& Value) {
        // TagName, Value 사용
    });

parser.ParseTag("<number>숫자</number>");
```

```cpp
iiXml::Writer::InputValidator validator;

QObject::connect(&validator, &iiXml::Writer::InputValidator::InvalidXmlFile,
    []() {
        // 유효하지 않은 XML 파일
    });

QObject::connect(&validator, &iiXml::Writer::InputValidator::InvalidTagClosure,
    []() {
        // 올바르지 않은 태그 종료
    });

validator.ValidateInput("<!Doctype XML>\n<XML><number>1</number></XML>");
```

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
