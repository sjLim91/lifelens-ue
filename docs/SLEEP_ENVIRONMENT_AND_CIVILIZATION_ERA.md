# 수면 환경과 현재 문명 단계 관찰 계약

기준 main: `66f623db9413ff626ec03dee433cec482c4e8efc` (#605/#611 포함).

## 수면 원인과 변경

기존 선택은 거리만 비교하고 동률에서는 SleepingPlace를 선호했다. 기존 회복에도 환경 감점은 있었지만 강한 비 자체를 시설 선택에 반영하지 않았다. 또한 근처 Shelter의 보호를 수면시설 실제 사용과 무관하게 회복에 적용했다. Health는 수면 중 Shelter와 노출 위치를 구분하지 않았다.

`SleepEnvironment.h`는 기존 DynamicEnvironment와 EnvironmentalConsequence를 평가한다. 노출은 precipitation, wetStress×0.8, wind×0.75, coldStress, heatStress 중 최댓값이다. 0.35부터 보호시설을 선호하고 0.60부터 보호 없는 수면을 ExposedEmergency로 관찰한다. 비·눈에 별도 가짜 상태를 만들지 않는다.

후보 cost = 거리×(1+Sleep Need) + 노출시설의 노출×24 + 내구도 감점×2. 동률은 기존 SleepingPlace 선호와 facility ID 정렬을 유지한다. 악천후 노출 또는 보호시설 이동의 이동 예산은 Sleep Need 0.60 이하에서 64 cells, 1.0에서 16 cells로 감소하며 평온한 날씨의 기존 settlement service radius는 유지한다. capacity reservation을 검사하고 실제 navigation route가 존재하며 예산 이내인 후보를 선택한다. 회복용 duration 계산은 도착 위치의 환경을 사용하되 실제 매 분 회복은 현재 환경으로 계산한다. 이동 중 회복 없음, 순간이동 없음.

매 recovery tick 현재 환경을 읽는다. 노출 응급 수면 중 보호시설 재검토는 15분 간격이며 cost가 4 이상 개선되는 실제 reachable Shelter만 선택한다. 기존 Hunger/Thirst/Bladder wake와 survival preemption이 우선한다. 실패 행동의 backoff는 극심한 피로에서 현재 위치 응급 취침을 금지하지 않는다. 지면 fallback도 실제 현재 위치를 고정한다.

건조한 날 gross recovery/minute: 지면 0.00230, 정상 SleepingPlace 0.00300 + 내구도×0.00020, Shelter 0.00300 + 내구도×0.00020. 악천후 노출 bedding는 지면 baseline을 사용한다. 노출 multiplier는 1−노출×0.15, 보호 multiplier는 1−노출×0.15×0.15이다. 회복은 0이 되지 않는다. 일반 Need decay와 환경·Health 압력은 별도로 유지한다. 보호는 실제 facility 위치에서만 적용하며 Health 감소는 실제 수면 interaction의 Protected 판정에만 적용한다.

## SleepingPlace 표현

기존 두꺼운 wood base + 직사각 mattress + pillow block 세 mesh를 낮은 섬유 중앙 매트, 비정형 양쪽 풀 가장자리, 느슨한 풀 묶음, 얇은 branch 세 개의 일곱 primitive part로 교체한다. 외부 asset, 새 geometry 종류, 무료 시설, 시대별 자동 reskin은 없다. 기존 공유 geometry/material과 구조 cache를 사용한다. 교체/제거/전체 dispose 때 instance matrix buffer를 해제하고 공유 geometry/material은 기존 lifecycle을 유지한다. 같은 material/geometry별 InstancedMesh 세 개로 묶어 시설당 draw call은 기존과 같은 3으로 유지한다. 실제 mobile GPU 측정은 별도 검증 대상이다.

중앙 매트 실제 top surface는 terrain 기준 **0.14 world units**이다. 시설 group의 terrain offset 0.025까지 포함한다. 수면 support constant도 0.14이다. #611 실제 UAL2 LayToIdle clip, skeleton calibration, endpoint fallback, slope support, snapshot refresh와 wake continuity 경로를 유지한다. Shelter 수면은 지면 support 경로를 유지한다.

## 현재 시대는 output

`CivilizationEraObservation.h`의 중앙 registry는 현재 observation에만 쓰인다. `CivilizationWorldObservation`을 만들 때 기존 technology population과 transformation 집계를 재사용한다. stable era/evidence ID만 bridge로 보내고 한국어 문구는 Web에서 변환한다. Save DTO, AI decision, technology prerequisite와 facility unlock에는 시대가 없다. 최고 도달 단계와 역사 기록을 덮어쓰지 않으며 현재 operational evidence가 사라지면 요약은 후퇴한다. 날짜는 입력 조건이 아니다.

정착 공통 조건:

- Operational/active/durability>0 SleepingPlace 또는 Shelter.
- storage/fire/shelter 종류 둘 이상 운영 또는 그 종류 하나 이상과 active ResourceBuffering.
- StoreGoods/ControlFire/CarryLiquid 중 둘 이상의 actual available capability.

각 단계의 정확한 조건:

| ID | 한국어 | 조건 |
|---|---|---|
| NaturalSurvival | 자연 생존기 | 다른 단계의 현재 운영 조건이 충족되지 않음 |
| EarlySettlement | 초기 정착기 | 정착 공통 조건 |
| AgrarianSettlement | 농경 정착기 | 정착 공통 + Cultivation operational resident + CultivateFood + operational CultivatedPlot + active ManagedFoodProduction(현재 파종 또는 수확 근거) |
| CopperMetallurgy | 초기 금속기 | 정착 공통 + CopperSmelting operational resident + SmeltMetal + operational Furnace + active MetallurgicalProduction(실제 현재 금속 산출/재고) |
| BronzeTechnology | 청동 기술기 | 정착 공통 + TinSmelting operational resident + BronzeAlloying operational resident + AlloyMetal + operational Furnace + active AdvancedTooling(실제 청동 도구) |

가장 높은 충족 definition을 현재 단계로 표시한다. 농경이 금속 단계의 강제 선행 gate는 아니다. operational은 기존 Core 의미(재현 지식과 실제 시설·재료 기회)를 재사용하며 Observed/Hypothesized/Understood만으로 상승하지 않는다. 현재 도구 보유와 operational 합금 재현성을 함께 요구한다.

DTO: `era.currentEra.{id,ordinal}`, `era.evidence[]`, `era.nextEra`, `era.nextEraRequirements[]`; evidence는 `{id,satisfied}`. 주민 presentation은 `sleepContext: None|Protected|Exposed|ExposedEmergency`를 받는다. Web은 시대나 악천후를 재판정하지 않는다.

상단은 기존 모바일 seed 여유 영역 안에서 wrap하며 터치 가능한 pill을 제공한다. native dialog의 modal top layer, focus trap/Escape, 한국어 근거/충족 상태/다음 단계 조건을 사용한다. Web은 bounded DTO만 표시하며 facility/technology 전체 목록을 시대 판정용으로 순회하지 않는다.

## 검증과 재개

새 Core 테스트: test_sleep_environment, test_civilization_era. 기존 수면 budget의 Shelter≥SleepingPlace 계약을 갱신한다. 기존 회귀를 포함한 16개 focused tests를 직접 g++로 검증한다. 새 Web test는 mat 실측 표면·geometry budget·한국어 era/evidence·dialog semantics를 검사한다. #611 resident-motion test의 bedding fixture를 0.14로 갱신하고 경사면 지면·매트 각각을 검사한다. 매트 support도 애니메이션 skeleton의 경사 지면 clearance와 함께 계산한다.

전체 CMake/CTest, Preflight, WASM, Web typecheck/build, resident-motion, 장기 run 결과는 PR 검증 완료 후 기록한다. 미실행 항목은 성공으로 취급하지 않는다. 상태는 tasks/WEATHER_SLEEP_ERA_WORK_STATE.md에 유지한다.

충돌 가능성이 높은 파일: Simulation.cpp, Simulation.h, SettlementProgression.h, CivilizationObserverReadModel.h, WebClientBridge.cpp, core-types.ts, facility-layer.ts, lifelens-contract.ts, observer-readout.tsx, resident-action-context.ts, styles.css. #605 resource projection 파일은 변경하지 않는다.

## 검증 기록 (2026-10-03)

커밋 47505d3e의 GitHub 검증: Preflight, Core Tests, Web WASM, Web Typecheck, Runtime Resilience, Character Asset Verification, Weather Sleep/Era Verification 모두 성공. custom long-run job의 전체 CTest는 **104/104 PASS** (39.55초). Web custom job은 한국어 DTO 표현, primitive mat 실측, #611 전체 resident-motion 및 build를 통과했다. 직접 g++ focused 16개도 PASS.

장기 검증은 원본 main과 동일 seed·시작 상태로 별도 Core 라이브러리를 빌드해 비교한다. 아래 합계는 모든 주민의 누적 포화 시간을 더한 resident-minutes이며 한 주민의 연속 포화와 다르다. 기존 정상 행동 우선순위는 유지한다. 실패한 행동의 재시도 backoff에서는 Sleep Need≥0.98인 주민에게 실제 현재 위치의 응급 취침만 허용한다. 이때 다른 시설 탐색/이동은 재시도하지 않고 기존 critical Hunger/Thirst preemption과 상대 Need wake 조건을 그대로 적용한다. 이 예외는 장기 run에서 발견된 실패한 배변 계획→강제 Idle→동일 실패 계획 반복의 피로 포화 경로를 막는다. 날씨 차이로 trajectory가 바뀌어 seed별 총량은 증가하거나 감소할 수 있다.

| seed / 365일 | main 최장 연속 / 총합 / 생존 | 변경 최장 연속 / 총합 / 생존 |
|---|---|---|
| 874213954 | 432 / 30268 / 3 | 671 / 142910 / 3 |
| 874213955 | 293 / 4905 / 4 | 501 / 18999 / 4 |
| 2 | 246 / 9339 / 4 | 278 / 1360 / 4 |

`Tools/verify_sleep_longrun.py`는 완료된 365일 이상 로그에서 연속 sleep saturation이 10,000분 이상이면 실패시킨다. 이 threshold는 user가 금지한 수만 분 starvation을 검출하는 CI acceptance guard이며 Core gameplay 값이 아니다. CI는 세 seed의 원본 로그와 JSON metrics를 artifact로 보존한다.

모바일 영향: sleeping place당 공유 geometry 2종, primitive part 7개를 InstancedMesh 3개로 묶어 기존과 같은 draw call 3개, sleep slope support는 snapshot 갱신 때 지면 sample 9개. Era registry는 Observer 갱신 때 한 번 평가하며 Web은 전달된 bounded 근거만 렌더한다. 모바일 badge는 44px touch target과 wrap, seed chip용 기존 right inset을 유지한다. 실제 기기 GPU/프레임 시간 및 before/after screenshot은 측정·촬영하지 않았고 성공으로 주장하지 않는다. Core/geometry/animation 검증이 실제 플레이 장면의 모든 시각 조건을 대신하지는 않는다.

변경 경계와 파일:

- Core authority: SleepEnvironment.h, PhysiologyBalance.h, SettlementProgression.h, Simulation.cpp, Simulation.h.
- Observer/bridge: PresentationDirective.h, CivilizationEraObservation.h, CivilizationObserverReadModel.h, WebClientBridge.cpp.
- Web DTO/presentation: core-types.ts, lifelens-contract.ts, facility-layer.ts, resident-world-layer.ts, resident-action-context.ts, localization/sleep-context.ts, state/observation-feed.ts, ui/civilization-era.tsx, ui/observer-readout.tsx, styles.css.
- 검증: CMakeLists.txt, test_sleep_environment.cpp, test_civilization_era.cpp, test_daily_physiology_budget.cpp, tests/resident-motion/run.mjs, tests/sleep-era/run.mjs, sleep-era-check.yml, verify_sleep_longrun.py.

main 재확인 SHA는 여전히 66f623db9413ff626ec03dee433cec482c4e8efc. GitHub 연결로 동일 main tree 위에 변경을 commit했고 terminal clone/pull을 다시 시도하지 않았다. PR #612만 생성하며 merge/auto-merge는 실행하지 않는다.

최종 local365 결과는 세 seed 모두 continuous-starvation guard PASS. 변경 전 중간 구현은 seed874213954에서83270분 연속 포화로 실패했으며 실패 계획의 backoff 응급 rest를 추가한 뒤671분으로 줄었다. main432분 대비 증가했으며 누적 resident-minutes는30268→142910으로 증가했다. seed874213955의 누적도4905→18999로 증가했다. 날씨를 반영한 현재 운영 상태의 trajectory 변화와 미해결 배변 affordance 실패 반복이 누적 피로 부담을 높인다. 연속 수만 분 포화 방지와 누적 부담 개선은 같은 지표가 아니며 총량이 개선됐다고 주장하지 않는다. 사용자 범위 밖의 배변/AI 전체 우선순위 재조정은 하지 않는다. raw 후보 로그와 JSON은 CI artifact에 보존하고 비교 요약은 tasks/WEATHER_SLEEP_ERA_LONGRUN.json에 저장한다. 최신 head의 remote365 gate 완료는 별도로 확인한다.
