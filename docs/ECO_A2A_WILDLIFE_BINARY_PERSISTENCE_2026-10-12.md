# ECO-A2a — 독립 야생동물 개체군 저장·복원 계약

Issue [#683](https://github.com/sjLim91/lifelens-ue/issues/683)

## 단일 권위 및 AI 선택권
개발 단계는 주민 행동의 고정된 순서가 아니다. Core는 동물의 위치·허기·성별·수명·물리적 제약·생태 자원과 행동 가능성을 제공한다. **사냥·관찰·먹이 제공·길들이기·사육 여부, 시점, 목표는 주민 AI의 경험·기억·성향·사회적 학습을 기반으로 자율 결정**해야 한다. 주민의 AI를 고정 스크립트나 해금 트리로 대체하지 않는다.

## 이번 PR 범위
- \`WildlifePopulationCodec.h\`: ECO-A1 standalone \`WildlifePopulation\`을 독립된 서식/개체 바이너리 v1에 직렬화.
- WorldSeed, generationVersion, day, nextSerial, Habitat/provenance, 개체별 GUID/위치/성별/나이/허기/생존 정보 정확한 round-trip.
- 엄격한 byte limit, count limit, species capacity, GUID 중복, finite values, freshwater fish 위치, magic/version, checksum 검사. 잘못된 복원은 기존 상태 변경 금지.
- 시드 같고 저장된 상태 같으면 1000일 중간 저장/복원 반복 후 생존·먹이·번식·포식·사망 결과가 완전히 같도록 검증.
- 기존 World/SimulationStateSnapshot v3/BinaryFormat v8를 그대로 유지. **이 새 코덱은 기존 저장 파일에 자동 포함되지 않으며 아직 실제 World 동물을 영속화하지 않는다.**

## 후속 ECO-A2b
정식 World/Simulation 일별 lifecycle에 개체군을 versioned opt-in으로 붙이고, 기존 snapshot과 atomic save/load 계약을 함께 갱신. 이때 기존 Legacy 저장 호환성·seed replay·2지역 확장 설계를 우선 점검. WASM 위치와 Core-authorized 행동/자원 DTO, Web 관찰 UI는 이후. 실제 지구의 지역 기후 및 야생동물 분포는 별도 검증된 Earth data pack 전에는 반영된 것으로 표시하지 않는다.
