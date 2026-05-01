# GetFile

`Src/Input/GetFile.h`와 `Src/Input/GetFile.cpp`는 XML 입력을 검증한 뒤 파서로 넘기는 입력기이다.

## 처리 순서

`iiXml::Writer::GetFile`은 다음 순서로 입력을 처리한다.

1. 파일 경로를 받은 경우 파일 전체를 읽는다.
2. `iiXml::Elements::Doctype`로 문서 상단 선언을 판정한다.
3. `iiXml::Writer::InputValidator`로 전체 태그 종료 상태를 판정한다.
4. XML 선언과 Doctype 선언을 제거한 뒤 남은 루트 태그를 `iiXml::Parser::TagParser`로 넘긴다.

첫 줄에 `<?xml version="..."?>` 선언이 있으면 문서 상단 선언으로 판정한다. 그 다음 줄의 `<!Doctype ABCD>` 같은 Doctype 이름은 `XML`로 고정하지 않고 유효한 이름이면 통과한다.

파일 확장자는 판정 조건이 아니다. `.xml`, `.iixml`, `.txt`, 확장자가 없는 파일처럼 어떤 이름이든 파일 내용이 XML 선언 또는 Doctype 선언과 태그 종료 검증을 통과하면 실질적인 XML 입력으로 처리한다.

태그 종료 검증은 `iiXml::Elements::OpenTag`의 완화 정책을 사용한다. 예를 들어 `<a><b></a></b>`처럼 표준 XML에서는 허용되지 않는 교차 종료 구조도 iiXml 입력으로 통과할 수 있으며, 루트 태그 값은 원본 내부 문자열 그대로 보존된다.

## 동기 API

```cpp
iiXml::Writer::GetFile input;
auto result = input.ParseFile("input.xml");
```

```cpp
auto result = input.ParseXml("<?xml version=\"1.0\"?>\n<!Doctype ABCD>\n<ABCD><number>1</number></ABCD>");
```

반환값은 `iiXml::Writer::GetFileResult`이다.

- `Parsed`: 파서까지 통과했으며 `Token`에 태그 파서 결과가 들어 있다.
- `FileReadFailed`: 파일을 열 수 없다.
- `InvalidXmlFile`: `Doctype` 객체를 통과하지 못했다.
- `InvalidTagClosure`: `InputValidator`를 통과하지 못했다.
- `ParserRejected`: 검증은 통과했지만 현재 태그 파서가 입력을 토큰화하지 못했다.
- `ExceptionThrown`: 입력 처리 중 예외가 발생했다.

`GetFileResult`는 실패 시에도 비어 있는 반환값으로 끝나지 않는다. 실패한 경우 `Token`은 없을 수 있지만, `Status`와 비어 있지 않은 `Reason`은 항상 반환된다.

생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패, 예외 로그를 출력한다.

## Qt 슬롯/시그널 API

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

## 예시

```cpp
#include <iiXml>

iiXml::Writer::GetFile input;

QObject::connect(&input, &iiXml::Writer::GetFile::Parsed,
    [](const QString& TagName, const QString& Value) {
        // TagName == "XML"
        // Value == "<number>1</number>"
    });

input.ReadXml("<!Doctype XML>\n<XML><number>1</number></XML>");
```

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
