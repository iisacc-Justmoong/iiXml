# InlineProperties

`Src/Elements/InlineProperties.h`와 `Src/Elements/InlineProperties.cpp`는 여는 태그 안의 attribute를 파싱한다.

## 지원 형식

하나의 여는 태그 안에 여러 속성을 둘 수 있다.

```xml
<resource title="hello" count=42 ratio=0.75 enabled=true visible=false>
```

`Parse()`는 속성을 열림 순서대로 반환한다. 각 속성은 다음 정보를 가진다.

- `Name`: 속성명
- `NameBegin`, `NameEnd`: 원본 입력에서 속성명 범위
- `HasValue`: 값 존재 여부
- `ValueBegin`, `ValueEnd`: 원본 입력에서 속성값 범위
- `ValueType`: `StringType`, `IntType`, `FloatType`, `BoolType`
- `TypeDeclared`: 명시 타입 토큰 사용 여부. 현재 표준 리터럴 규칙에서는 명시 타입 토큰을 쓰지 않으므로 `false`다.

속성값 문자열은 즉시 복사하지 않고 offset 범위로 보존한다.

## 타입 판정

- `"hello"`, `"42"`: 따옴표로 감싼 값은 문자열 타입
- `42`, `-7`: 따옴표 없는 정수 리터럴은 정수 타입
- `0.75`, `1e-3`: 따옴표 없는 실수 리터럴은 실수 타입
- `true`, `false`: 따옴표 없는 boolean 리터럴은 boolean 타입
- 그 외 따옴표 없는 값: 문자열 타입 추론
- 값이 없는 flag 속성은 문자열 타입으로 처리한다.

## 예시

```cpp
iiXml::Elements::InlineProperties properties;
std::string_view input = "<resource title=\"hello\" count=42 enabled=true>";
auto parsed = properties.Parse(input);

if (parsed.has_value()) {
    // (*parsed)[0].Name == "title"
    // (*parsed)[0].ValueType == iiXml::Elements::InlinePropertyType::StringType
    // (*parsed)[1].ValueType == iiXml::Elements::InlinePropertyType::IntType
    // (*parsed)[2].ValueType == iiXml::Elements::InlinePropertyType::BoolType
    // (*parsed)[0].TypeDeclared == false
}
```

생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패, 예외 로그를 출력한다.

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
