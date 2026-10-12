# ECO-A1 — 독립 동물 생태계 Core 기반 (개발 중)

Issue: [#683](https://github.com/sjLim91/lifelens-ue/issues/683)

## 구현 범위
- \`WildlifeEcology.h\`: 동물종 (토끼·사슴·늑대·담수어), 개별 ID/성별/나이/배고픔/위치, 지역 수용력, 일별 먹이 섭취/자율 이동/포식/번식/자연사.
- \`deriveWildlifeHabitat()\`: 기존 Core procedural \`MacroRegionFacts\` + \`HydrologyFacts\`를 기반으로 한 서식 잠재력만 평가. 실제 지구 특정 종의 분포/지리·기온·강우 진실은 **아님**. \`geographicallyConditioned=false\`, \`regionalWeatherApplied=false\`.
- 생태용 난수는 WorldSeed·ChunkCoord·개체·날짜·사건 독립 도메인으로 계산한다. 기존 \`World::rng\`, Needs·Utility AI·자원·기후에 영향을 주지 않는다.
- 성별/성숙 여부를 확인한 번식, 개체수 상한, **근접 추적과 사냥 성공 시에만 포식**(청크 전체 원격 즉시 포식 금지), 굶주림·수명 사망 및 bounded within-chunk 이동. 담수어는 수원 중심에 유지하며 실제 유영 경로는 후속.
- 1회 명시적 일 단위 전진만 허용하여 중복 실행으로 개체 증식되지 않는다.
- 새 모델은 **opt-in isolated Core domain**으로, \`Simulation::step()\` 호출이나 주민 월드에 자동 등록되지 않는다.

## 구현되지 않은 범위 (반드시 별도 연결)
1. Core World 정식 엔티티, 저장/불러오기 Snapshot codec, 다중 지역 간 이동/개체군 보존.
2. 주민이 동물을 인지·추적·사냥하는 Utility AI, 부상 확률, 도구/기술, 시신·도축, 육류·가죽/뼈 유한 재고, 조리/섭취.
3. 실제 지구 지역별 서식 분포/실측 기온·강수·숲·하천, EARTH-3/4/6 climate truth.
4. WASM 동물 DTO, Web 3D 모습/애니메이션, 지역 선택 시 실제 동물 관찰.

## 검증
- \`g++ -std=c++17 -O2 -Wall -Wextra -Werror -UNDEBUG -I Source/LifeLensCore/include Source/LifeLensCore/tests/test_wildlife_ecology.cpp -o /tmp/eco && /tmp/eco\`
- 동일 Seed/Chunk/Day에서 결정론, PopulationSeed 무관성, 1,100일 복제 재현, 출생·사망, ocean 담수어 금지, 물고기 육상 이동 방지, 먼 거리 원격 포식 금지, 한 종별 수용력 상한, 1일 반복 재실행 금지.
- CMake Core test 등록, 기존 Core/Preflight/WASM 및 장기 검사 회귀를 모두 확인하고 PR 병합.
