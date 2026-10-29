# GetTagInfo

`Src/Parser/GetTagInfo.h`와 `Src/Parser/GetTagInfo.cpp`는 XML 문자열을 받아
`iiXml` 파서와 호환되는 방식으로 태그 값과 필드를 파싱해 객체 내부에 저장한다.

## 책임

- XML 원문은 설치된 `~/.local/SDK`의 `iiXml::iiXml` 타깃과 공개 헤더 `iiXml.h`를 통해 해석한다.
- 내부 파싱은 `iiXml::Parser::TagParser::ParseAll()`을 사용한다.
- 본문 태그 앞의 XML 선언과 DOCTYPE은 태그 range가 아니므로 파싱 전에 제거한다.
- 첫 번째 루트 태그의 이름, 원문, 값, 필드 목록을 빠른 조회용 상태로 저장한다.
- 전체 루트 태그 목록과 자식 태그 계층은 `GetTags()`와 `GetRoot()`로 조회한다.
- `iiXml`의 range 모델을 보존하기 위해 모든 태그의 `raw_begin`, `value_begin`,
  `value_end`, `raw_end`와 원문 조각을 `GetRanges()`로 조회할 수 있다.
- `<paragraph><bold>...</paragraph></bold>` 같은 iiXml의 교차 종료 구조는 트리만으로
  완전히 표현하지 않고, 겹치는 range 목록으로 원본 구조를 보존한다.
- 파싱 실패 시 이전 성공 상태를 제거하고 `GetError()`에 실패 사유를 저장한다.

## 사용 예시

```cpp
#include <iiHtmlBlock>

iiHtmlBlock::GetTagInfo info;

if (info.Parse("<XML id=\"main\" order=7><body>Hello</body></XML>")) {
    // info.GetTagName() == "XML"
    // info.GetValue() == "<body>Hello</body>"
    // info.GetFields()[0].name == "id"
    // info.GetFields()[0].value == "main"
    // info.GetRanges()[0].tag_name == "XML"
}
```

`ParseXml(const QString&)` 슬롯을 사용하면 성공 시 `Parsed(tag_name, value)`,
실패 시 `ParseFailed(reason)` 시그널을 내보낸다.

## 빌드와 검증

`iiXml` 설치본은 `~/.local/SDK` 아래의 CMake 패키지로 찾는다. 빌드는 반드시 저장소의
`build/` 디렉터리를 사용한다.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
