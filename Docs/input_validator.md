# InputValidator

`Src/Input/InputValidator.h`와 `Src/Input/InputValidator.cpp`는 XML 입력의 종료 상태를 판정한다.

## 반환 상태

`iiXml::writer::InputValidator::validate()`는 다음 순서로 입력을 검증한다.

1. `iiXml::elements::DOCTYPE`으로 문서 상단 선언을 판정한다.
2. 상단 선언을 통과한 경우 태그 열림/닫힘 쌍을 판정한다.

반환값은 `iiXml::writer::validation_exit`이다.

- `valid`: 유효한 입력이다.
- `invalid_xml_file`: `DOCTYPE` 객체를 통과하지 못한 입력이다.
- `invalid_tag_closure`: `InputValidator`의 태그 종료 검증을 통과하지 못한 입력이다.
- `exception_thrown`: 검증 중 예외가 발생했다.

실패 사유가 필요한 경우 `validate_result()`를 사용한다. 반환값은 `iiXml::writer::validation_result`이며, `exit`와 비어 있지 않은 `reason`을 포함한다.

생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패, 예외 로그를 출력한다.

## 사용 예

```cpp
#include "iiXml.h"

iiXml::writer::InputValidator validator;
auto result = validator.validate("<!DOCTYPE XML>\n<XML><number>1</number></XML>");
auto detailed = validator.validate_result("<XML><number>1</number></XML>");

if (result == iiXml::writer::validation_exit::invalid_xml_file) {
    // 유효하지 않은 XML 파일
}

if (detailed.exit == iiXml::writer::validation_exit::invalid_xml_file) {
    // detailed.reason에 실패 사유가 들어 있다.
}

if (result == iiXml::writer::validation_exit::invalid_tag_closure) {
    // 올바르지 않은 태그 종료
}
```

## 태그 종료 규칙

- 시작 태그와 종료 태그는 `iiXml::elements::OpenTag` 정책으로 판정한다.
- 닫는 태그는 태그 이름으로 매칭하며, 같은 이름이 여러 번 열려 있으면 가장 최근에 열린 같은 이름 태그를 닫는다.
- 따라서 `<a><b></a></b>`처럼 태그가 교차 종료되는 구조는 iiXml 입력으로 허용한다. 이는 표준 XML의 엄격한 중첩 규칙과 다르다.
- `<tag />` 형식의 자체 종료 태그는 별도 종료 태그를 요구하지 않는다.
- 주석, CDATA, 처리 명령, DOCTYPE 선언은 태그 종료 스택에서 제외한다.

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
