# M8 인수 증거 — 2026-11-14

기능 검증 소스: `8dcdcbdb1ff0a99085913c804855fd7f4509eba1`. 최종 main/Runtime/Pages/외부 probe는 `release-proof.json`에 별도로 기록한다.

- `native-unit.log`: M8-01~08 물리·개인 지식·실패·재발견·옛 저장/손상 거부 회귀.
- `native-production-replay.log`: 실제 Core 14분/3원료, 채집 사용, 두 번의 시범→도제와 중간 저장. 욕구 증가를 멈춘 테스트 rules·낮은 시작 욕구·높은 호기심/제작 숙련·초기 원료·첫 토큰을 고정하는 통제 fixture다. 이후 실제 채집/시범 회귀는 원천·위치·시범 간격/관계도 통제하며 이 사실을 무개입 검증과 섞지 않는다.
- `wasm-proof.json`: 같은 native fixture를 실제 WASM으로 실행한다. 기존 난수 스트림 텍스트만 libc++/libstdc++ 표현이 다르며, 나머지 byte 및 코덱 정규화 후 전체 byte를 비교한다.
- `audit-build.json`, `natural-*.jsonl`: 위 소스의 Release Core, 변경하지 않은 새 게임 세 시드의 100/365/1000일. 인구/실험/실사용/시범/시설과 실제 바이너리 저장 왕복이다. 세계·재료·목표·기억을 주입하지 않는다.
- `browser-fixture-proof.json`, `invention-320.png`: native production 결과를 실제 React UI에 표시한 네 화면 크기 검사. 통제 fixture를 자연 발생 세계로 표시하지 않는다.
- `local-app-proof.json`: 최종 WASM 앱의 실제 네 명 새 게임·개인 발명 패널·모바일/가로 화면, 지원되는 2D fallback. 물리 Android/GPU/발열 검사가 아니다.
- `evidence-source.json`: 위 원시 파일의 소스/범위와 SHA-256. 원격 배포 후 추가된 파일은 `release-proof.json`의 출처를 따른다.
- `al1-*.log`: 이전 main의 옛 자연 시드와 새 Core의 자연 시드 4. 90일·양수 재고·한 건의 기억 제거·실제 물리 회복 조건을 보존하며 목표 달성 검증을 추가했다.
- `proposal-*`: 매 제안 안의 반복 스캔 제거 전후 192개 입력 선택 ID/정확한 부동소수 점수 동등성과 제안 함수만의 비용 대조. 전체 게임·Android 성능 수치로 확대하지 않는다.

재현: `tasks/M8_EMERGENT_ARTIFACTS_2026-11-14.md` 및 저장소의 실제 native/WASM/Web 검증 도구를 따른다. 실기기/발열 제외는 이전 사용자 지시를 유지한다.

공개 배포의 실제 발명품 복원·자동 저장·새로고침 뒤 개인 지식 보존: `public-artifact-persistence.json`, `public-invention-mobile.png`. 실제 사용자 저장소와 분리한 새로운 브라우저 context를 사용한다. `verify-public-bytes.py <runtime-main-sha> <output-dir>`와 `review-persisted-app.cjs`는 선택적 재현 도구다. 후자는 `LIFELENS_REVIEW_URL`, `LIFELENS_REVIEW_OUTPUT`, `LIFELENS_ARTIFACT_FIXTURE_DIR`, `LIFELENS_PLAYWRIGHT_MODULE`, `LIFELENS_EXPECTED_RUNTIME_SHA`를 받는다.
