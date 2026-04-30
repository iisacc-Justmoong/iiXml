# Parser 모듈

`Src/Parser/` 모듈은 iiXml 커스텀 XML 규약의 입력 문자열에서 단일 여는 태그와 닫는 태그를 인식한다.

## 태그 파서

`iiXml::parser::tag_parser`는 하나의 태그 쌍을 읽어 `tag_name`과 `value`를 반환한다.

```cpp
#include "iiXml.h"

iiXml::parser::tag_parser parser;
auto parsed = parser.parse("<number>숫자</number>");

if (parsed.has_value()) {
    // parsed->tag_name == "number"
    // parsed->value == "숫자"
}
```

`parse_all()`은 입력 안의 태그 쌍을 모두 읽어 `std::vector<tag_range>`로 반환한다. 이 API는 `iiXml::elements::OpenTag`의 완화된 교차 종료 규칙을 사용하므로, `<a><b></a></b>`에서도 `a`와 `b`를 모두 보존한다. 태그 수는 두 개로 제한하지 않으며, 여러 스타일 태그와 문단 태그가 교차해도 열린 순서대로 모든 태그를 반환한다.

```cpp
iiXml::parser::tag_parser parser;
std::string_view input = "<a><b></a></b>";
auto parsed = parser.parse_all(input);

if (parsed.has_value()) {
    // (*parsed)[0].tag_name == "a"
    // input.substr((*parsed)[0].raw_begin,
    //     (*parsed)[0].raw_end - (*parsed)[0].raw_begin) == "<a><b></a>"
    // (*parsed)[1].tag_name == "b"
    // input.substr((*parsed)[1].raw_begin,
    //     (*parsed)[1].raw_end - (*parsed)[1].raw_begin) == "<b></a></b>"
}
```

## 현재 계약

- `parse()` 입력은 `<tag>value</tag>` 형태의 단일 태그 쌍이어야 한다.
- `parse_all()`은 여러 태그를 열림 순서대로 모두 반환하고, 각 항목의 `value_begin/value_end`와 `raw_begin/raw_end`에는 자기 닫기 태그까지 전진한 원본 offset 범위를 보존한다.
- 열린 태그 프로세스는 짝 닫기 태그를 만나기 전까지 완료되지 않으며, 완료된 뒤에만 `tag_range` 필드가 확정된다.
- 태그 매칭은 태그 이름 기준이며, 같은 이름이 여러 번 열려 있으면 가장 최근에 열린 같은 이름 태그가 먼저 닫힌다.
- 태그 이름은 영문자 또는 `_`로 시작하고, 이후에는 영문자, 숫자, `_`, `-`, `.`, `:`를 사용할 수 있다.
- 입력 문자열의 바깥쪽 공백은 무시하지만, 태그 내부 값의 공백은 보존한다.
- 닫는 태그가 없거나 여는 태그와 닫는 태그 이름이 다르면 `std::nullopt`를 반환한다.
- `parse_all()`에서 닫히지 않은 열린 태그가 남으면 `std::nullopt`를 반환한다.
- 속성의 의미 분석과 XML 이스케이프 해석은 아직 수행하지 않는다.
- 생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패 로그를 출력한다.

## 검증

다음 명령으로 빌드와 테스트를 실행한다.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
