# Qt 없는 소스 보존 마크업

`iiXmlMarkup` 패키지의 `iiXml::Markup`은 C++23 태그 파서와 HTML 블록 범위
추적기를 제공한다. `<iiXml/Markup/TagParser.h>`와
`<iiXml/Markup/BlockRangeTracker.h>`를 포함하고 해당 CMake 대상을 연결한다.
기존 `iiXml::Native`의 libxml2 기반 스트림 XML API 및 Qt 기반 호환 API와는
별도의 계약이다. 이 대상에는 Qt, libxml2 또는 iiGeneralDocument 연결이 없다.
전체 iiXml 빌드에는 별도 Native 대상 때문에 libxml2가 필요할 수 있다.

파싱은 입력 바이트와 범위를 보존하며, 중첩 태그 및 속성을 표현한다. 블록
추적기는 원본 범위를 기준으로 안정적인 식별자를 갱신한다. HTML 블록 범위는
교차하는 태그를 다룰 수 있지만, iiGeneralDocument의 XML 트리는 단일 루트와
겹치지 않는 계층을 추가로 요구한다. 이 API는 브라우저 HTML5 오류 복구나
DTD·엔티티 확장을 제공한다고 약속하지 않는다.

`iiXml.Markup.Contracts`는 중첩, Unicode, 갱신 후 식별자, 잘못된 입력 및
실패 시 보존 동작을 확인한다. 호환되는 기존 Qt 기반 파서는 별도 인터페이스로
계속 제공되며, 이 구성요소가 모든 파서 계약을 대체한다고 가정하지 않는다.
