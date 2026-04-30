# Qt Object API

iiXml의 주요 객체는 Qt 프로젝트에서 직접 연결할 수 있도록 `QObject`를 상속한다.

## 공통 정책

- Qt 6.8.3 `Core` 모듈을 기준으로 빌드한다.
- CMake는 `~/Qt/6.8.3/macos`를 Qt 탐색 경로 앞에 추가한다.
- 동기 API와 Qt 슬롯/시그널 API를 함께 제공한다. 검증/입력 API의 실패는 상태와 비어 있지 않은 사유 문자열로 반환하거나 방출한다.
- 모든 라이브러리 객체 생성자와 public 메서드는 `QDebug`/`qDebug()`로 디버깅 로그를 출력한다.

## TagParser

`iiXml::parser::tag_parser`는 `QObject`를 상속한다.

슬롯:

- `parseTag(const QString& input)`

동기 API:

- `parse(std::string_view input)`
- `parse_all(std::string_view input)`

`parse_all()`은 `OpenTag`의 교차 종료 정책을 사용하여 `<a><b></a></b>`에서도 `a`와 `b`를 모두 반환한다. 반환 타입은 `std::vector<tag_range>`이며, `value/raw` 문자열을 복사하지 않고 원본 입력의 offset 범위를 제공한다.

시그널:

- `tagParsed(const QString& tag_name, const QString& value)`
- `parseFailed(const QString& reason)`

## FileParser

`iiXml::parser::FileParser`는 `QObject`를 상속한다.

슬롯:

- `parseFile(const QString& file_path)`

시그널:

- `tagParsed(const QString& tag_name, const QString& value)`
- `parseFailed(const QString& reason)`

## DOCTYPE

`iiXml::elements::DOCTYPE`는 `QObject`를 상속한다.

슬롯:

- `matchTop(const QString& input)`

시그널:

- `doctypeMatched(DOCTYPE::Kind kind, const QString& raw)`
- `doctypeRejected(const QString& reason)`
- `xmlDeclarationMatched(const QString& raw)`
- `doctypeDeclarationMatched(const QString& raw)`

## OpenTag

`iiXml::elements::OpenTag`는 `QObject`를 상속한다.

동기 API:

- `close_open_tag(std::vector<std::string>& open_tags, std::string_view closing_tag_name)`
- `parse_open_tags(std::string_view input)`

`close_open_tag()`는 열린 태그 스택에서 닫는 태그 이름과 같은 항목을 찾아 제거한다. 최상단 태그만 닫는 엄격한 XML 정책이 아니라, `<a><b></a></b>` 같은 iiXml 교차 종료 구조를 허용하는 정책이다. `parse_open_tags()`는 같은 정책으로 모든 태그를 열림 순서대로 반환하고, 각 항목의 `raw_begin/value_begin/value_end/raw_end`에 원본 offset 범위를 보존한다.

## InputValidator

`iiXml::writer::InputValidator`는 `QObject`를 상속한다.

슬롯:

- `validateInput(const QString& input)`

시그널:

- `validationFinished(InputValidator::ValidationExit result)`
- `validationFailed(InputValidator::ValidationExit result, const QString& reason)`
- `validXml()`
- `invalidXmlFile()`
- `invalidTagClosure()`
- `exceptionThrown()`

## GetFile

`iiXml::writer::GetFile`은 `QObject`를 상속한다.

슬롯:

- `readFile(const QString& file_path)`
- `readXml(const QString& input)`

시그널:

- `parsed(const QString& tag_name, const QString& value)`
- `failed(GetFile::Status status, const QString& reason)`
- `fileReadFailed()`
- `invalidXmlFile()`
- `invalidTagClosure()`
- `parserRejected()`
- `exceptionThrown()`

## GetStringToken

`iiXml::writer::GetStringToken`은 `QObject`를 상속한다.

`QString` 문자열을 입력으로 받지만, 태그 파서에 바로 넘기지는 않는다. `DOCTYPE` 판정과 `InputValidator` 전체 XML 검증을 통과한 뒤 루트 태그를 파서로 전달한다.

슬롯:

- `readString(const QString& input)`

시그널:

- `parsed(const QString& tag_name, const QString& value)`
- `failed(GetStringToken::Status status, const QString& reason)`
- `parseFailed(const QString& reason)`

## 사용 예

```cpp
#include "iiXml.h"

iiXml::parser::tag_parser parser;

QObject::connect(&parser, &iiXml::parser::tag_parser::tagParsed,
    [](const QString& tag_name, const QString& value) {
        // tag_name, value 사용
    });

parser.parseTag("<number>숫자</number>");
```

```cpp
iiXml::writer::InputValidator validator;

QObject::connect(&validator, &iiXml::writer::InputValidator::invalidXmlFile,
    []() {
        // 유효하지 않은 XML 파일
    });

QObject::connect(&validator, &iiXml::writer::InputValidator::invalidTagClosure,
    []() {
        // 올바르지 않은 태그 종료
    });

validator.validateInput("<!DOCTYPE XML>\n<XML><number>1</number></XML>");
```

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
