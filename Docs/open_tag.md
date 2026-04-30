# OpenTag

`Src/Elements/OpenTag.h`와 `Src/Elements/OpenTag.cpp`는 열린 태그 스택에서 닫는 태그를 처리하는 요소 모듈이다.

## 교차 종료 규칙

`iiXml::elements::OpenTag::close_open_tag()`는 닫는 태그 이름과 같은 열린 태그를 스택의 최상단부터 역순으로 찾는다. 같은 이름을 찾으면 그 항목을 제거하고 성공을 반환한다.

이 규칙은 표준 XML의 엄격한 중첩 규칙이 아니라 iiXml의 완화 정책이다. 따라서 다음 구조는 허용된다.

```xml
<a>
    <b>
</a>
    </b>
```

위 입력은 열린 태그 스택이 `a, b`인 상태에서 `</a>`가 먼저 들어와도 `a`를 찾아 닫고, 이후 `</b>`로 남은 `b`를 닫는다. 이 규칙은 두 태그에 한정되지 않으며, 열린 태그가 몇 개든 각 태그는 자기 이름의 닫기 태그가 나올 때까지 독립 범위로 생존한다.

## 태그 보존 파싱

`parse_open_tags()`는 입력 안의 모든 태그 쌍을 열림 순서대로 반환한다. 반환 항목은 `tag_name`, `value`, `raw`를 가진다.

```cpp
iiXml::elements::OpenTag open_tag;
auto parsed = open_tag.parse_open_tags("<a><b></a></b>");

if (parsed.has_value()) {
    // (*parsed)[0].tag_name == "a"
    // (*parsed)[0].raw == "<a><b></a>"
    // (*parsed)[1].tag_name == "b"
    // (*parsed)[1].raw == "<b></a></b>"
}
```

교차 종료 구조에서는 한 태그의 `raw`나 `value` 안에 다른 태그의 시작 또는 종료 마크업이 그대로 남을 수 있다. 이 동작은 입력을 자동으로 재배열하거나 정상 중첩 구조로 고치지 않고, 각 열린 태그가 자기 이름의 닫기 태그를 만날 때까지 전진해서 보존하기 위한 것이다. 열린 태그 프로세스는 짝이 되는 닫기 태그를 만나기 전까지 완료되지 않으며, 완료된 뒤에만 `tag_name`, `value`, `raw` 필드가 확정된다.

예를 들어 다음 입력은 `p`, `bold`, `italic`, 두 번째 `p`를 모두 별도 항목으로 보존한다.

```xml
<p><bold><italic>text</p><p>really</bold> useful</italic></p>
```

이 경우 `bold`는 첫 번째 `p`와 두 번째 `p`를 가로지르고, `italic`은 `bold` 닫힘 이후까지 계속 생존한다.

## 실패 조건

- 닫는 태그 이름이 비어 있으면 실패한다.
- 같은 이름의 열린 태그가 스택에 없으면 실패한다.
- 실패 시 열린 태그 스택은 변경하지 않는다.
- `parse_open_tags()`는 끝까지 닫히지 않은 열린 태그가 남아 있으면 실패한다.

생성자와 public 메서드는 `QDebug`/`qDebug()`로 진입, 성공, 실패, 예외 로그를 출력한다.

## 검증

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
