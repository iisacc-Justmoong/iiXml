# FileParser

`src/Parser/FileParser.h`와 `src/Parser/FileParser.cpp`는 호출자가 제공한
읽기 함수를 통해 경로에 해당하는 내용을 얻고 기존 태그 파서에 전달한다.
iiXml은 저장소나 다른 SDK를 직접 연결하지 않는다.

## 반환 형식

`iiXml::Parser::FileParser::ParseFile()`은 파일 전체를 읽은 뒤 `iiXml::Parser::TagParser::Parse()`에 넘긴다.

```cpp
std::optional<iiXml::Parser::TagValue>
```

파일을 열 수 없거나 파일 내용이 태그 파서를 통과하지 못하면 `std::nullopt`를 반환한다.

생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패 로그를 출력한다.

## 사용 예

```cpp
#include <iiXml>
#include "FileProviderReader.h" // example/FileProvider/App

iiXml::Parser::FileParser parser(fileProviderReader());
auto parsed = parser.ParseFile("input.xml");

if (parsed.has_value()) {
    // parsed->TagName
    // parsed->Value
}
```

읽기 함수는 경로에 대해 UTF-8 문자열 또는 실패를 뜻하는 `std::nullopt`를
반환한다. 호출자가 함수와 캡처한 객체의 수명 및 동시 접근을 관리한다. 기본
생성자로 만든 객체에는 읽기 함수가 없어 경로 입력이 실패한다. 기존 호출자는
앱에서 iiFileProvider 어댑터를 주입하고 라이브러리와 앱을 다시 빌드해야 한다.
실제 조합과 테스트는 `example/FileProvider/`를 참고한다.

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
