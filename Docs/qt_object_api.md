# Qt Object API

iiXml의 주요 객체는 Qt 프로젝트에서 직접 연결할 수 있도록 `QObject`를 상속한다.

## 공통 정책

- Qt 6.8.3 `Core` 모듈을 기준으로 빌드한다.
- CMake는 `~/Qt/6.8.3/macos`를 Qt 탐색 경로 앞에 추가한다.
- 기존 동기 반환 API는 유지하고, Qt 슬롯/시그널 API를 추가 제공한다.

## TagParser

`iiXml::parser::tag_parser`는 `QObject`를 상속한다.

슬롯:

- `parseTag(const QString& input)`

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

## InputValidator

`iiXml::writer::InputValidator`는 `QObject`를 상속한다.

슬롯:

- `validateInput(const QString& input)`

시그널:

- `validationFinished(InputValidator::ValidationExit result)`
- `validXml()`
- `invalidXmlFile()`
- `invalidTagClosure()`

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
