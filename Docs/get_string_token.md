# GetStringToken

`Src/Input/GetStringToken.h`와 `Src/Input/GetStringToken.cpp`는 파일이 아닌 `QString` 문자열 입력을 검증한 뒤 태그 파서로 넘기는 입력 모듈이다.

## 역할

`GetStringToken`은 파일 경로를 받지 않는다. XML 형식 파일이 아니라 메모리 안의 `QString` 문자열을 받아 UTF-8로 변환한다.

검증 순서는 `GetFile`의 문자열 입력 경로와 같다.

1. `DOCTYPE`으로 XML 선언 또는 DOCTYPE 선언을 판정한다.
2. `InputValidator`로 전체 XML 태그 종료를 검증한다.
3. XML 선언과 DOCTYPE 선언을 제거한 뒤 남은 루트 태그를 `iiXml::parser::tag_parser`에 전달한다.

따라서 `GetStringToken`도 `DOCTYPE` 판정이나 전체 XML 검증을 통과하지 못한 문자열은 태그 파서로 넘기지 않는다.

## 동기 API

```cpp
#include "iiXml.h"

iiXml::writer::GetStringToken input;
auto result = input.parse_string("<!DOCTYPE XML>\n<XML><number>숫자</number></XML>");

if (result.status == iiXml::writer::get_string_token_status::parsed &&
    result.token.has_value()) {
    // result.token->tag_name == "XML"
    // result.token->value == "<number>숫자</number>"
}
```

반환값은 `iiXml::writer::get_string_token_result`이다. 실패한 경우에도 빈 `std::optional`만 반환하지 않고 `status`와 비어 있지 않은 `reason`을 함께 반환한다.

- `parsed`: 검증과 파싱을 모두 통과했다.
- `invalid_xml_file`: `DOCTYPE` 판정을 통과하지 못했다.
- `invalid_tag_closure`: 전체 XML 태그 종료 검증을 통과하지 못했다.
- `parser_rejected`: 검증된 루트 XML을 태그 파서가 토큰화하지 못했다.
- `exception_thrown`: 문자열 입력 처리 중 예외가 발생했다.

생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패, 예외 로그를 출력한다.

## Qt 슬롯/시그널 API

슬롯:

- `readString(const QString& input)`

시그널:

- `parsed(const QString& tag_name, const QString& value)`
- `failed(GetStringToken::Status status, const QString& reason)`
- `parseFailed(const QString& reason)`

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
