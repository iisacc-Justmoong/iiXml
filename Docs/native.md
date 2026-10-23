# Qt 독립 XML 읽기·쓰기

`iiXml::Native`는 C++23 표준 값과 libxml2 >= 2.9로 구현한 메모리 기반 XML 경계이다. 기존 QObject API와 분리된 `iiXmlNative` 패키지이므로 Qt 없는 소비자가 기존 Qt 패키지를 간접 검색하지 않는다.

```sh
cmake -S . -B build -DIIXML_BUILD_QT_API=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/build/install"
```

```cmake
find_package(iiXmlNative 0.1 CONFIG REQUIRED)
target_link_libraries(backend PRIVATE iiXml::Native)
```

공개 include는 `<iiXml/Xml.h>`이다. 설치 경로는 기존 extensionless `<iiXml>` 파일과 충돌하지 않도록 `include/iiXmlNative/iiXml/`이며 타깃이 검색 경로를 전달한다. 기존 `<iiXml>`과 `iiXml::iiXml` 소비자는 `IIXML_BUILD_QT_API=ON`으로 유지할 수 있다. ON의 기존 Qt 6.8.3 제약은 그대로이다.

`Reader::open`은 UTF-8 소유 문자열을 유지하고 `next()`로 하나의 소유 이벤트를 반환한다. 종료는 성공한 빈 optional, 문법/정책/한도 위반은 expected 오류이다. 종료와 오류는 이후에도 유지된다. 이름, local name, namespace URI, 속성, 깊이, 빈 엘리먼트 여부를 구분한다. 빈 엘리먼트는 `empty=true`인 시작 이벤트 하나로 반환한다. 일반 텍스트·CDATA·공백, 주석, 처리 명령, 단순 DOCTYPE을 구분한다.

입력/출력 8 MiB, 텍스트 항목 1 MiB, 속성 128개, 깊이 64, 이벤트 100,000개가 기본 한도이다. DOM 전체를 만드는 API가 아니다. libxml2의 모든 내부 할당을 합산 제한하는 할당기는 아니다. 네트워크나 파일에서 XML을 읽는 API를 제공하지 않는다. 외부 식별자, 내부 DTD subset과 사용자 엔티티는 파서 생성 전 거절한다. FCPXML의 `<!DOCTYPE fcpxml>` 같은 단순 루트 선언만 허용한다. 전역 엔티티 로더/오류 핸들러를 변경하지 않는다. 스키마/DTD 검증과 XInclude는 제공하지 않는다.

`Writer`는 명시적 start/text/end 호출을 받아 UTF-8과 속성 escaping을 수행한다. `finish()`는 하나의 닫힌 루트와 namespace/문법 유효성을 다시 검사한 후 완성된 바이트만 반환한다. 네임스페이스 선언은 `xmlns` 속성으로 명시한다. 파일 저장은 iiFileProvider에 위임한다. 이 추가는 SharedCanvas의 기존 SVG/FCPXML Qt 어댑터를 자동으로 변경하지 않는다.

`Test/native_test.cpp`는 네임스페이스·Unicode·escaping·빈 엘리먼트·단순 DOCTYPE 왕복, 잘못된 XML, 외부/내부 DTD 거절, 속성/깊이/이벤트/바이트 한도, 불완전한 writer를 실행 검증한다.
