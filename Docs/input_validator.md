# InputValidator

`Src/Writer/InputValidator.h`와 `Src/Writer/InputValidator.cpp`는 XML 입력의 종료 상태를 판정한다.

## 반환 상태

`iiXml::writer::InputValidator::validate()`는 다음 순서로 입력을 검증한다.

1. `iiXml::elements::DOCTYPE`으로 문서 상단 선언을 판정한다.
2. 상단 선언을 통과한 경우 태그 열림/닫힘 쌍을 판정한다.

반환값은 `iiXml::writer::validation_exit`이다.

- `valid`: 유효한 입력이다.
- `invalid_xml_file`: `DOCTYPE` 객체를 통과하지 못한 입력이다.
- `invalid_tag_closure`: `InputValidator`의 태그 종료 검증을 통과하지 못한 입력이다.

## 사용 예

```cpp
#include "iiXml.h"

iiXml::writer::InputValidator validator;
auto result = validator.validate("<!DOCTYPE XML>\n<XML><number>1</number></XML>");

if (result == iiXml::writer::validation_exit::invalid_xml_file) {
    // 유효하지 않은 XML 파일
}

if (result == iiXml::writer::validation_exit::invalid_tag_closure) {
    // 올바르지 않은 태그 종료
}
```

## 태그 종료 규칙

- 시작 태그와 종료 태그는 스택 순서로 일치해야 한다.
- `<tag />` 형식의 자체 종료 태그는 별도 종료 태그를 요구하지 않는다.
- 주석, CDATA, 처리 명령, DOCTYPE 선언은 태그 종료 스택에서 제외한다.

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
