# Logging

iiXml의 라이브러리 객체와 public 메서드는 Qt 표준 디버그 스트림인 `QDebug`/`qDebug()`로 운영 및 진단 로그를 출력한다.

## 정책

- 객체 생성자는 생성 로그를 출력한다.
- public 동기 메서드와 Qt 슬롯은 진입 로그를 출력한다.
- 입력 로그는 `input` 이벤트로 출력하며 입력 크기, 라인 수, 짧은 preview를 포함한다.
- 파싱 진행 로그는 `parsing` 이벤트로 출력하며 이벤트명, offset, line, column, 주변 context를 포함한다.
- 출력 로그는 `output` 이벤트로 출력하며 성공/실패 상태와 반환 결과 요약을 포함한다.
- 성공, 실패, 예외 반환 지점은 상태와 사유를 로그로 출력한다.
- 단순 실패 반환(`std::nullopt`, `false`)을 사용하는 기존 API도 실패 직전에 `qDebug()` 로그를 출력한다.
- XML 입력 전문과 출력 값 전문은 기본 로그에 남기지 않고 입력 크기, 상태, 태그 이름, 값 크기 같은 메타데이터와 짧은 snippet만 출력한다.
- 파싱 실패 로그는 `reason`, `offset`, `line`, `column`, `context`를 함께 출력한다.
- `offset`과 `column`은 입력 `std::string_view`의 byte 기준 위치다. UTF-8 문자는 보존하지만 column 계산은 byte offset 계약과 맞춘다.
- `context`는 실패 위치 주변의 짧은 snippet만 남기며, 개행과 탭은 `\n`, `\t`처럼 escape해서 한 줄 로그로 유지한다.
- 공통 로그 구현은 `Src/Logging/XmlLog.h`와 `Src/Logging/XmlLog.cpp`에 둔다.

## 예시

```cpp
iiXml::Writer::GetFile input;
auto result = input.ParseXml("<!Doctype XML>\n<XML><number>1</number></XML>");
```

위 호출은 `iiXml::Writer::GetFile::ParseXml begin`, `input`, `parsing`, `output` 로그를 `qDebug()`로 출력한다.

정상 파싱 로그는 다음과 같은 진단 필드를 포함한다.

```text
iiXml::Parser::TagParser::Parse input input_size= 19 line_count= 1 preview= <number>42</number>
iiXml::Parser::TagParser::Parse parsing event= opening_tag offset= 0 line= 1 column= 1 context= <number>42</number>
iiXml::Parser::TagParser::Parse output status= parsed summary= tag=number value_size=2 raw_size=19
```

파싱 실패 예시는 다음과 같은 진단 필드를 포함한다.

```text
iiXml::Parser::TagParser::Parse failed reason= closing tag mismatch offset= 11 line= 1 column= 12 context= <number>42</text>
```

라이브러리 로딩/초기 확인이 필요한 경우 `Launch()`를 호출한다. `Launch()`는 표준 출력 대신 `qDebug()`로 `iiXml::Launch library Launch` 로그를 출력한다.

## 검증

`debug_log_test`는 `qInstallMessageHandler`로 `qDebug()` 메시지를 수집하고 주요 객체와 public 메서드가 성공/실패 로그를 출력하는지 확인한다. `debug_diagnostics_test`는 입력, 파싱, 출력 로그와 실패 로그에 `reason`, `offset`, `line`, `column`, `context`가 포함되는지 확인한다. 예외 처리 경로도 각 public 메서드에 `exception` 로그를 둔다.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
