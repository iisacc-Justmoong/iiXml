# GetFile

`Src/Input/GetFile.h`와 `Src/Input/GetFile.cpp`는 XML 입력을 검증한 뒤 파서로 넘기는 입력기이다.

## 처리 순서

`iiXml::writer::GetFile`은 다음 순서로 입력을 처리한다.

1. 파일 경로를 받은 경우 파일 전체를 읽는다.
2. `iiXml::elements::DOCTYPE`로 문서 상단 선언을 판정한다.
3. `iiXml::writer::InputValidator`로 전체 태그 종료 상태를 판정한다.
4. XML 선언과 DOCTYPE 선언을 제거한 뒤 남은 루트 태그를 `iiXml::parser::tag_parser`로 넘긴다.

첫 줄에 `<?xml version="..."?>` 선언이 있으면 문서 상단 선언으로 판정한다. 그 다음 줄의 `<!DOCTYPE ABCD>` 같은 DOCTYPE 이름은 `XML`로 고정하지 않고 유효한 이름이면 통과한다.

파일 확장자는 판정 조건이 아니다. `.xml`, `.iixml`, `.txt`, 확장자가 없는 파일처럼 어떤 이름이든 파일 내용이 XML 선언 또는 DOCTYPE 선언과 태그 종료 검증을 통과하면 실질적인 XML 입력으로 처리한다.

## 동기 API

```cpp
iiXml::writer::GetFile input;
auto result = input.parse_file("input.xml");
```

```cpp
auto result = input.parse_xml("<?xml version=\"1.0\"?>\n<!DOCTYPE ABCD>\n<ABCD><number>1</number></ABCD>");
```

반환값은 `iiXml::writer::get_file_result`이다.

- `parsed`: 파서까지 통과했으며 `token`에 태그 파서 결과가 들어 있다.
- `file_read_failed`: 파일을 열 수 없다.
- `invalid_xml_file`: `DOCTYPE` 객체를 통과하지 못했다.
- `invalid_tag_closure`: `InputValidator`를 통과하지 못했다.
- `parser_rejected`: 검증은 통과했지만 현재 태그 파서가 입력을 토큰화하지 못했다.
- `exception_thrown`: 입력 처리 중 예외가 발생했다.

`get_file_result`는 실패 시에도 비어 있는 반환값으로 끝나지 않는다. 실패한 경우 `token`은 없을 수 있지만, `status`와 비어 있지 않은 `reason`은 항상 반환된다.

생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패, 예외 로그를 출력한다.

## Qt 슬롯/시그널 API

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

## 예시

```cpp
#include "iiXml.h"

iiXml::writer::GetFile input;

QObject::connect(&input, &iiXml::writer::GetFile::parsed,
    [](const QString& tag_name, const QString& value) {
        // tag_name == "XML"
        // value == "<number>1</number>"
    });

input.readXml("<!DOCTYPE XML>\n<XML><number>1</number></XML>");
```

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
