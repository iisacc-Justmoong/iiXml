# Debug Logging

iiXml의 라이브러리 객체와 public 메서드는 Qt 표준 디버그 스트림인 `QDebug`/`qDebug()`로 디버깅 로그를 출력한다.

## 정책

- 객체 생성자는 생성 로그를 출력한다.
- public 동기 메서드와 Qt 슬롯은 진입 로그를 출력한다.
- 성공, 실패, 예외 반환 지점은 상태와 사유를 로그로 출력한다.
- 단순 실패 반환(`std::nullopt`, `false`)을 사용하는 기존 API도 실패 직전에 `qDebug()` 로그를 출력한다.
- XML 입력 전문은 기본 로그에 남기지 않고 입력 크기, 상태, 태그 이름, 값 크기 같은 메타데이터만 출력한다.

## 예시

```cpp
iiXml::writer::GetFile input;
auto result = input.parse_xml("<!DOCTYPE XML>\n<XML><number>1</number></XML>");
```

위 호출은 `iiXml::writer::GetFile::parse_xml begin`과 최종 상태 로그를 `qDebug()`로 출력한다.

라이브러리 로딩/초기 확인이 필요한 경우 `launch()`를 호출한다. `launch()`는 표준 출력 대신 `qDebug()`로 `iiXml::launch library launch` 로그를 출력한다.

## 검증

`debug_log_test`는 `qInstallMessageHandler`로 `qDebug()` 메시지를 수집하고 주요 객체와 public 메서드가 성공/실패 로그를 출력하는지 확인한다. 예외 처리 경로도 각 public 메서드에 `exception` 로그를 둔다.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
