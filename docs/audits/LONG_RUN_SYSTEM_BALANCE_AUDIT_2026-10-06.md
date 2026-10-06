# 전 시스템 장기 균형·인과 감사 — 2026-10-06

사회/가족 진행의 첫 병목은 두 seed 모두 day1부터 관찰된 사회활동 선택 starvation이다. 생존에 많은 시간을 쓰지만, 사회 후보 자체가 eligibility에 못 미치는 별도 경로가 있다. 1000일 인구 전체 붕괴는 관찰하지 않았고 founder 사망은 각 seed 1명이다. 밸런스 수치를 수정하지 않았다.

## 기준과 재현

- main `67fbf1e2c133499b2271f885ef066df30efdf665`; #642 교역 persistence 기준. #643 shadow/#644 Earth 문서는 실제 main에 맞춰 확인했다. Earth multi-origin은 설계 단계이며 현재 run을 multi-origin 검증으로 표현하지 않는다.
- branch `audit/system-balance-causal-20261006`, PR [#646](https://github.com/sjLim91/lifelens-ue/pull/646). open PR 파일 overlap 확인; WORK_STATE/TEAM_BOARD 수정 없음. clone 없이 GitHub connector 사용.
- seeds 874213954,4242001; elapsed day1/7/30/100/365/1000. Core 달력 Day N과 elapsed day는 다르다. 모든 최초 발생일은 elapsed 기준. 추가 seed는 선택하지 않았다.
- 기존 ll_balance_audit 확장. 시작 후 새로 열린 #645(first-contact 수정)는 SocialUtility/tests만 변경하며 감사 lane과 overlap 없음. 이 미병합 PR의 동작은 이번 baseline에 포함하지 않는다.
- Core/src/include, ruleset, probabilities, utility weights, save schema, Web truth 변경 없음.
- native g++ C++17 -O2 -DNDEBUG, shared runner에서 병렬 측정; CMake 설치 부재로 동일 CMake 소스 목록 직접 빌드. Actions는 CMake Release로 실행. concurrent wall/RSS는 기계 성능 일반화 자료가 아니다.

```sh
cmake -S Source/LifeLensCore -B build/core -DCMAKE_BUILD_TYPE=Release
cmake --build build/core --target ll_balance_audit -j2
python -m unittest discover -s Tools/tests
python Tools/audit_system_balance.py run --executable build/core/ll_balance_audit --output longrun-results --seeds 874213954,4242001 --days 1000 --checkpoints 1,7,30,100,365,1000 --workers 2 --determinism-seed 874213954 --resume
```

완료 실행은 설정/바이너리/save hash 일치 시 재사용한다. 중단 실행은 감사 누적량이 save에 없으므로 처음부터 재실행한다. Snapshot에서 이어간 짧은 실행은 과거 1000일 누적 지표를 복원한 것으로 표현하지 않는다. raw metrics/events JSONL, long-format CSV, 요약 JSON/Markdown, snapshots, manifest는 GitHub Actions artifact에 보관(30일). 인수인계는 `tasks/LONG_RUN_SYSTEM_BALANCE_AUDIT_HANDOFF_20261006.md`.

## 실행 시간과 결정론

- 874213954: complete, runtime 1090.25초.
- 4242001: complete, runtime 1814.48초.
- 874213954-repeat: complete, runtime 1132.42초.
- 전체 동시 실행 wall 2236.50초; 3회 run runtime 합 4037.15초(동시 구간 중복).

원본 하네스 vs 확장 하네스 seed874 day1/7/30/100/365 snapshot SHA256 모두 동일. 대표 seed1000일 2회 비교는 wall/RSS만 제외하고 전체 metrics/events 레코드와 6개 save bytes를 비교한다. observer 보고 보정(critical interval 집계/재시도 timestamp 제거)은 Core 상태를 바꾸지 않는다. 1000일 native evidence는 observer afd922e 빌드이며, 수정된 reporting observer는 7일 반복 smoke byte/event/metric identity를 별도로 검증했다. 오래된 pooled temporal counters는 주민별 count 합/max streak로 summary에서 정확히 재구성했다.

결정론 결과: `{"passed": true, "excluded_nondeterministic_fields": ["elapsed_wall_ms", "peak_rss_kib"], "snapshots": {"day-1.llsave": {"sha256": "1ea2c9c9ce205750c0974348921700a32af24f54041ceffa4cef0f448958b6bb", "identical": true}, "day-100.llsave": {"sha256": "09146dfe7be02214a9b037728db0c1e9c9fe5d3fac2acfc4a14f122acf87a021", "identical": true}, "day-1000.llsave": {"sha256": "3e04d2ea7819f088d389a0ec55c280692e90c31da502f8825cd5174782b331f5", "identical": true}, "day-30.llsave": {"sha256": "e6b1ea6e206cca18e226ec52ec917f5073f41c75d49a45cff718601bb49a013c", "identical": true}, "day-365.llsave": {"sha256": "7f09ba483959216076d93517c48a5ced4c8531eea0e050df4538c6141a9d3185", "identical": true}, "day-7.llsave": {"sha256": "8c9656b54d7685779afed0a5f06c8f475fcb48ea14632ba7df538b6e05532341", "identical": true}}, "issues": []}`

기존 native test_determinism/test_snapshot_codec/test_intersettlement_trade/test_generation_continuity 통과; 신규 7개 factual invariant 테스트 통과. 원본 CMake Core push CTest 107/107 PASS. PR Core reuse 대기 timeout은 테스트 실패가 아니며 재실행했다. reporting 수정 SHA94eec882의 Core push 검증도 PASS, docs-only3e61fbe의 PR Core와 Web WASM도 PASS. 최신 CI 상태는 PR Checks에서 확인한다. 균형값 합격 임계값은 없다.

## 체크포인트 행동시간·인구

시간은 살아 있는 주민 관측분의 누적 비율. 사망 전 관측은 누적 분모에 보존한다. active commitment를 22개 exclusive category로 분류; 완료 token0 캐시 제외, final tick 포함. sub-minute planner profiler가 아니다. family는 parenting/health-care 포함, romance/marriage/institution/economy의 background tick 비용은 독립 분리 불가.

| seed | day | living | births | survival% | social% | family% | civilization% | idle% | retry% | facilities | chunks |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 874213954 | 1 | 4 | 0 | 57.57 | 0.0 | 0.0 | 24.48 | 17.9 | 0.05 | 3 | 2 |
| 874213954 | 7 | 4 | 0 | 54.22 | 0.0 | 0.0 | 32.94 | 12.83 | 0.01 | 10 | 2 |
| 874213954 | 30 | 4 | 0 | 51.51 | 0.0 | 0.0 | 36.53 | 11.96 | 0.0 | 12 | 2 |
| 874213954 | 100 | 4 | 0 | 60.31 | 0.0 | 0.0 | 30.32 | 9.33 | 0.03 | 34 | 32 |
| 874213954 | 365 | 3 | 0 | 63.02 | 0.0 | 0.0 | 29.09 | 7.8 | 0.09 | 66 | 62 |
| 874213954 | 1000 | 3 | 0 | 62.34 | 0.0 | 0.0 | 29.44 | 8.02 | 0.21 | 69 | 72 |
| 4242001 | 1 | 4 | 0 | 53.14 | 0.0 | 0.0 | 28.26 | 18.59 | 0.0 | 4 | 2 |
| 4242001 | 7 | 4 | 0 | 55.68 | 0.0 | 0.0 | 32.67 | 11.65 | 0.0 | 13 | 2 |
| 4242001 | 30 | 4 | 0 | 58.24 | 0.0 | 0.0 | 32.75 | 9.01 | 0.0 | 16 | 2 |
| 4242001 | 100 | 4 | 0 | 57.98 | 0.0 | 0.0 | 33.62 | 8.4 | 0.0 | 38 | 28 |
| 4242001 | 365 | 3 | 0 | 60.46 | 0.0 | 0.0 | 31.6 | 7.88 | 0.06 | 70 | 99 |
| 4242001 | 1000 | 3 | 0 | 60.02 | 0.0 | 0.0 | 32.27 | 7.62 | 0.09 | 81 | 104 |

Needs 통계(0.90 observational pressure band; 각 주민의 temporal entries 합/max streak):

| seed | Need | mean | p95/p99 | critical entries/minutes/max streak |
|---|---|---:|---|---|
| 874213954 | hunger | 0.4484 | 0.721/0.805 | 137/5914/167 |
| 874213954 | thirst | 0.4481 | 0.736/0.829 | 308/14984/317 |
| 874213954 | sleep | 0.4343 | 0.74/0.855 | 230/22207/527 |
| 874213954 | bladder | 0.4607 | 0.723/0.799 | 159/9776/288 |
| 874213954 | hygiene | 0.3482 | 0.703/0.845 | 307/23615/452 |
| 4242001 | hunger | 0.4457 | 0.728/0.822 | 187/12264/497 |
| 4242001 | thirst | 0.4491 | 0.747/0.865 | 486/31568/629 |
| 4242001 | sleep | 0.4428 | 0.766/0.882 | 332/37621/604 |
| 4242001 | bladder | 0.4580 | 0.726/0.819 | 183/14430/622 |
| 4242001 | hygiene | 0.3538 | 0.722/0.879 | 413/39239/769 |

Core 시작/완료 횟수(day1000; incomplete/pending 포함하므로 실패율로 바로 바꾸지 않음):

| seed | Eat | Drink | Sleep | Toilet | Wash |
|---|---|---|---|---|---|
| 874213954 | 11789/11789 (100.00%) | 14009/14005 (99.97%) | 11627/3311 (28.48%) | 16419/13144 (80.05%) | 7238/6907 (95.43%) |
| 4242001 | 11346/11346 (100.00%) | 13733/13731 (99.99%) | 11553/4137 (35.81%) | 15125/12816 (84.73%) | 7907/7240 (91.56%) |

Sleep 완료율은 각종 interrupt와 budget/회복 완료 판정의 영향을 받는다. 완료 횟수만으로 실제 휴식 회복 실패라고 판정하지 않는다. 이동 표본은 observed multi-tick session이며 Eat 즉시 완료는 표본 없음으로 남겼다.

4 founder→couple→pregnancy→birth chain은 두 seed에서 시작하지 못했다. dating/engaged/married/household/pregnancy/birth 모두0. generation count1, depth0, juvenile dependents0, worker3(stage canWork eligibility이며 illness capacity cutoff 아님), dependent/worker0. 태어난 자녀 생존/성장 실패를 원인으로 주장할 증거는 없다. 출생 후 성인18년=6570일이므로 1000일만으로 성인/다음 세대 재생산을 검증할 수 없다. 생존일 평균/중앙은 right-censored 관측기간이며 평균수명 추정이 아니다.

## 최초 발생과 causal evidence

- day1: Social Approach 후보 최대874=.1720,424=.1695, `minSocialUtility=.18` 미달. day7/30/100/365 pure saved-state probes에서도 미달. unified civilization utility .55~.95가 선택되는 시점 존재. probe는 실제 planning boundary 실행 증거와 구분하며 전후 encoded bytes 동일을 확인한다. `SocialUtility.h` eligibility / `Simulation.cpp::trySocialDecision`이 직접 선택 gate다. day1000 424 raw social candidate는 이미 사망한 id3을 반환했다(생존자의 utility .178102/.137909). chooser는 deceased filtering이 없고 trySocialDecision이 마지막에 dead target을 거절한다. 사망 주민이 행동했다는 P0 증거는 아니며 후보 유효성 부채다.
- day33.3444(874)/82.3861(424): TinOre 목적 일반 탐험 시작; hunger/thirst 비critical, migration candidate0. `CivilizationDecision.h::materialProgressDemand`는 CopperSmelting 이후 TinSmelting 미보유에 .78. 발견된 전역 node가 local 접근 가능한 공급이 된다는 보장은 없다.
- day1.729861 Taeyun이 (1138,528)에 HumanWaste residue27 생성 → day1.735417 Yerin이 인접한 자연수 접근점(1139,528)에서 exposure .3138, water dose 증가 .131859 관측 → 이후874 자연수 residue dose 관측 → day55.1667 Hayun dose .406854 후 pathogen .36655 → day57.1667 첫 illness. ambient sanitation 지식이 있어도 direct water dose는 별도 입력이다.
- day119.1667 Yerin illness → day141~142 pathogen1/severity1 → day143 새 물 dose0이어도 pathogen .918871 → day144 .836823, capacity .398392 → day145.1667 illness death. Core recovery branch는 pathogen<.28일 때만 진행; 입력이 멈춘 2일만으로 해당 경계에 못 내려가 사망 전 회복하지 못했다. 단순 illness label을 넘어 clearance/recovery 경계를 확인했다.
- illness capacity는 이동 tick penalty에도 들어가고 thirst/sleep decay 부담도 증가한다. adult CareNetwork membership은 치료 행동 완료를 뜻하지 않는다. Parenting HealthCare는 development health와 관련하며 adult pathogen 치료 action의 직접 증거가 아니다.
- day298874 복수 active settlement 등장. family/인구 증가/household migration가 없이 생활권 topology가 바뀐 사례다. 하루 단위 entity evidence로 기록; 후보 압력0만으로 모든 split이 accidental이라고 단정하지 않는다.
- day307.875874 교역 출발 →309.9708 CopperMetal/Stone exchange →310.6389 귀환(3980분). survival preemption9, 재개 관측6; 최종 active journey 고착 없음.
- day360.813194424 Soyeon Water Explore 시작(1470,288→1470,353, thirst .5641, migration pressure .480244) →360.901389 탐험 성공 완료(127분/63grid) →360.90625 Toilet →360.916667 Drink 시작(need .76) →day361 위치(1472,297) →361.166667 위치(1380,269), Drink 시작 후360분 미완료/이동 상태에서 thirst1/sleep.9785/hygiene1, illness.2223로 deprivation death. 탐험 중 죽은 것이 아니라 탐험 완료 후 물에 접근하는 단계에서 죽었다. `Simulation.cpp` EmergencyUse directNaturalWater는 advanceNavigation 도착 전 Need relief를 주지 않는다. `Health.h::deprivationFatalChance` severe thirst가 급성 위험을 만들고 deprivation 판정이 illness보다 먼저다. 직전 repeated Drink와 context route failure는 순서 증거이며 route geometry 실패 원인까지는 미확정.
- day1000 두 seed population3으로 유지; 전체 collapse는 관찰하지 않았다.

## 문제 우선순위 (11필드 machine-readable 별첨)

시설 확장의 first_day100은 문제로 기록한 checkpoint이며 정상 초기 기반 건설과의 경계가 객관적으로 확정된 최초 분은 아니다. 이 warning의 upstream은 미확정이다. P0 확인 없음. P1은 3개이며 TOP10을 채우려고 낮은 심각도를 P1로 올리지 않는다. 아래 10개는 P1/P2/P3를 분리했다.

### 1. P1 — 사회/가족 진행 starvation

- 최초일: 1
- 직접 원인: Social 후보가 .18 eligibility에 미달; 실제 social 0
- upstream: first-contact utility/관계 bootstrap와 civilization 후보 경쟁
- downstream: dating/marriage/pregnancy/birth 전체0
- 재현 seed: 두 seed
- 영향 주민/생활권: founder 전체
- 수정 layer: Core SocialUtility / unified arbitration
- 난이도: 중
- regression risk: 관계/utility 전반 높음

### 3. P1 — 오염 누적 후 founder 질병 사망

- 최초일: 57.1667
- 직접 원인: Yerin pathogen >=.28, severity1 유지, day145.1667 illness death
- upstream: 자연수 residue dose 반복, clearance보다 입력 우세; 물 오염 dose는 sanitation ambient 완화와 별도
- downstream: 작업 capacity 약.40, female founder 감소
- 재현 seed: 874213954
- 영향 주민/생활권: Yerin(id3); 최초 illness Hayun(id4)
- 수정 layer: Core Health / water access sanitation integration
- 난이도: 중~상
- regression risk: 건강/돌봄/생존 상호작용 높음

### 2. P1 — 물 탐색 후 급수 이동 중 founder 급성 결핍 사망

- 최초일: 361.1667
- 직접 원인: Soyeon thirst1 상태 noon deprivation deterministic roll
- upstream: day360.813 Water 탐험 시작→360.901 탐험 완료(127분/63grid)→360.916 Drink 시작→360분 동안 미완료/이동→361.166 noon 사망; frontier 완료가 local water 공급 확보를 보장하지 않음, 정확 급수 target/경로 원인은 미확정
- downstream: 생존3/4, 잠재 가족 풀 감소
- 재현 seed: 4242001
- 영향 주민/생활권: Soyeon(id3)
- 수정 layer: Core provision/navigation/health daily sampling
- 난이도: 상
- regression risk: 이동/급수/일일 사망 판정 높음

### 4. P2 — TinOre 발견→채집→Tin/Bronze 진행 단절

- 최초일: 33.3444
- 직접 원인: 874 Tin natural75인데 gathered0, TinMetal/Bronze0
- upstream: TinSmelting 미지식 materialProgressDemand .78, local supply 탐색; 전역 발견은 접근성 보장 아님
- downstream: Tin 탐험65회, 실사용 부재, chunk/시설 확장
- 재현 seed: 두 seed; 첫 탐험424 day82.3861
- 영향 주민/생활권: Hayun/Jaeho 등 탐험자
- 수정 layer: Core CivilizationDecision / local reachability
- 난이도: 상
- regression risk: 기술/탐험/수급 높음

### 5. P2 — 인구 증가 없는 시설 footprint 확대

- 최초일: 100
- 직접 원인: 874 시설34→66→69, 인구4→3; sleepers21/workSurface19
- upstream: 이동한 생활권/시설 후보 분포와 중복수요가 후보; repair 대신 rebuild 인과는 미확정
- downstream: 시설/자원/탐색 비용과 save 증가
- 재현 seed: 두 seed
- 영향 주민/생활권: 정착지 서비스 영역/시설 위치
- 수정 layer: Core Facility demand/settlement observation
- 난이도: 중
- regression risk: 생존 접근성/시설 공유 높음

### 6. P2 — survival tax 및 self-care 재시도

- 최초일: 1
- 직접 원인: day1000 survival 약60%대; 실제 Needs/max/streak CSV 참고
- upstream: needs decay+목적지 거리+행동시간+health 이동 penalty; 상대 기여 분해 아직 불가
- downstream: social 선택 기회 감소; 이것만으로 social0 설명 불가
- 재현 seed: 두 seed
- 영향 주민/생활권: resident별 budget/needs
- 수정 layer: Core utility/navigation/physiology, 진단 후 선택
- 난이도: 상
- regression risk: 모든 생존/사회 loop 높음

### 7. P2 — context route failure 누적

- 최초일: 1
- 직접 원인: 874 context route failure3146, timeout0
- upstream: event가 실패 context 종류/대상 경로를 충분히 노출하지 않아 upstream 미확정
- downstream: penalty idle/retry 비용; 순서만으로 탐험 원인 단정 금지
- 재현 seed: 두 seed
- 영향 주민/생활권: 실패 actor별 JSONL
- 수정 layer: Core navigation diagnostics; 수정은 재현 후
- 난이도: 중
- regression risk: routing/commitment 높음

### 8. P2 — 사회적 이주 없이 정착지 재분류

- 최초일: 298
- 직접 원인: 874 day298 복수 active 생활권; day1000 1곳, assigned2/3
- upstream: 가족/출산/household 이주0인 상태 위치/시설 topology 변화
- downstream: trade partner 생겼다가 생활권 합쳐짐; household migration 성공 아님
- 재현 seed: 874213954; 424 초기 inactive 영역 별도
- 영향 주민/생활권: settlement2/78/82/86
- 수정 layer: Core settlement topology / migration evidence
- 난이도: 상
- regression risk: trade/home assignment 높음

### 9. P3 — 시설·지식 사용 counter 해석 한계

- 최초일: 1
- 직접 원인: 일부 facility usage0, 살아 있는 주민 technique subsequent use300일+ 부재
- upstream: FirePit/Furnace/CultivatedPlot usageCount callsite 누락; passive sanitation knowledge는 실행 counter와 다른 효과
- downstream: 실제 dead-system false positive 위험
- 재현 seed: 두 seed
- 영향 주민/생활권: 공정 시설/각 resident knowledge
- 수정 layer: Core observer/event telemetry (truth 변경 없이)
- 난이도: 중
- regression risk: counter/schema와 제품 해석 중

### 10. P3 — gross resource flow 및 인과 세부 관측 누락

- 최초일: 1
- 직접 원인: production/consumption/spoilage/trade material flows null
- upstream: 재고 delta는 소비/손실/교역 구분 불가; Core event payload 부족
- downstream: 수급 demand/supply 회계와 exact shortage 인과 미확정
- 재현 seed: 두 seed
- 영향 주민/생활권: 16 material 전체
- 수정 layer: Core read-only event ledger / harness
- 난이도: 상
- regression risk: save schema 추가 없이 observer 구현 권장

## 자원 경제

전역 자연량은 materialized natural quantity이며 reachable/accessible 보장 없음. stock=살아 있는 carried+stored raw material; natural direct water는 제외. zero-stock duration은 공급부족 판정과 다르다. gross production/consumption/construction/repair/spoilage/trade flows는 정확한 원인 ledger가 없어 unavailable로 남겼다. Gather/Store/Retrieve만 Core 사건의 x단위 수치를 수집했다.

| seed | material | natural | carried living | stored | gathered | zero stock 분 |
|---|---|---:|---:|---:|---:|---:|
| 874213954 | Water | 8707 | 0 | 0 | 2212 | 1182994 |
| 874213954 | PlantFood | 17934 | 5 | 0 | 11791 | 7747 |
| 874213954 | Wood | 25963 | 4 | 0 | 670 | 33130 |
| 874213954 | Stone | 14406 | 11 | 0 | 123 | 92 |
| 874213954 | Flint | 7061 | 3 | 0 | 26 | 62 |
| 874213954 | Fiber | 15681 | 6 | 0 | 282 | 3898 |
| 874213954 | Clay | 14453 | 9 | 0 | 247 | 12 |
| 874213954 | Bone | 0 | 0 | 0 | 0 | 1440000 |
| 874213954 | Hide | 0 | 0 | 0 | 0 | 1440000 |
| 874213954 | CopperOre | 1611 | 0 | 0 | 7 | 1435190 |
| 874213954 | TinOre | 75 | 0 | 0 | 0 | 1440000 |
| 874213954 | IronOre | 0 | 0 | 0 | 0 | 1440000 |
| 874213954 | Charcoal | 0 | 286 | 0 | 0 | 6530 |
| 874213954 | CopperMetal | 0 | 4 | 0 | 0 | 47196 |
| 874213954 | TinMetal | 0 | 0 | 0 | 0 | 1440000 |
| 874213954 | Bronze | 0 | 0 | 0 | 0 | 1440000 |
| 4242001 | Water | 10234 | 0 | 0 | 3447 | 1046705 |
| 4242001 | PlantFood | 29828 | 5 | 7 | 11404 | 1438 |
| 4242001 | Wood | 45088 | 5 | 0 | 665 | 10147 |
| 4242001 | Stone | 18910 | 5 | 0 | 85 | 1014 |
| 4242001 | Flint | 8282 | 2 | 0 | 28 | 50 |
| 4242001 | Fiber | 28321 | 5 | 0 | 378 | 41130 |
| 4242001 | Clay | 23903 | 11 | 0 | 100 | 20 |
| 4242001 | Bone | 0 | 0 | 0 | 0 | 1440000 |
| 4242001 | Hide | 0 | 0 | 0 | 0 | 1440000 |
| 4242001 | CopperOre | 1499 | 0 | 0 | 3 | 939959 |
| 4242001 | TinOre | 11 | 0 | 0 | 0 | 1440000 |
| 4242001 | IronOre | 0 | 0 | 0 | 0 | 1440000 |
| 4242001 | Charcoal | 0 | 244 | 4 | 0 | 21902 |
| 4242001 | CopperMetal | 0 | 1 | 0 | 0 | 118292 |
| 4242001 | TinMetal | 0 | 0 | 0 | 0 | 1440000 |
| 4242001 | Bronze | 0 | 0 | 0 | 0 | 1440000 |

TinOre→TinMetal→Bronze은 실제 진행 병목 후보다. Bone/Hide/IronOre 공급0은 현재 활성 요구가 없으면 자동 병목으로 분류하지 않는다. Charcoal 재고 축적은 excess 경고; Wood/food 등의 임시 0은 duration과 실패 이벤트를 함께 봐야 한다. 전역 물0만으로 물 부족을 선언하지 않는다.

## 시설·탐험·정착지·교역

| seed | facility counts(day1000) | exploration attempts/success | chunks | discovered/depleted nodes | trade departure/exchange/return |
|---|---|---|---|---|---|
| 874213954 | {'WorkSurface': 19, 'SleepingPlace': 21, 'CultivatedPlot': 12, 'PrimitiveStorage': 7, 'FirePit': 6, 'Shelter': 2, 'Furnace': 2} | 70/70 | 72 | 1041/170 | 1/1/1 |
| 4242001 | {'PrimitiveStorage': 8, 'SleepingPlace': 31, 'WorkSurface': 22, 'CultivatedPlot': 16, 'Shelter': 1, 'FirePit': 2, 'Furnace': 1} | 112/102 | 104 | 1562/167 | 0/0/0 |

시설 상태와 실제 counter coverage:

- 874213954: operational 68, ruined 1, daily durability increase observations 129; WorkSurface zero-use 6, SleepingPlace zero-use 0.
- 4242001: operational 81, ruined 0, daily durability increase observations 179; WorkSurface zero-use 16, SleepingPlace zero-use 0.

874 SleepingPlace21/WorkSurface19/Storage7/FirePit6/Shelter2/Furnace2/Plot12. 성장없는 시설 확장은 확인됐지만 후반69로 plateau; 무한 runaway라고 부를 증거 없음. sleepers는 실제 사용 흔적이 있고, repair 활동도 존재하므로 일괄 rebuild loop로 단정할 수 없다. 시설 repair 횟수 대신 daily durability-increase observations를 남겼다. zero usage로 판단할 수 없는 공정 시설을 별도 표시한다.

874 탐험70회 중 Tin65/Water5, critical pressure-band0(Need >=.90 기준; Core long-range opportunity 분기와 다름). 완료거리2524 grid(평균36.06), radius11 chunks, 신규 nodes1041 중 이후 quantity 감소170(16.33%). 424 Water 탐험의 critical 표시는 Need pressure-band이며 ordinary/long-range planner 분기를 확정하는 지표가 아니다. migration pressure .48 이상인 탐험도 있으므로 혼동하지 않는다. chunk growth에는 실제 기술/급수 목적이 있으나 Tin 발견의 실사용 연결 부재 때문에 생산적 발견만으로 설명할 수 없다. path failure 이후 탐험도 event 순서로 확인하되 인과 확정하지 않는다. 알려진 node 방문/재방문 counts와 materialized chunks를 구분한다.

교역874은 #642 귀환 persistence와 생존 preemption 후 재개를 정상 확인했다. 424에서 출발0은 고착이 아니다. 둘 다 종료 checkpoint 미완료 journey는 별도 trade_active record로 확인하며 유한 horizon에서 영원한 고착을 추정하지 않는다. household migration chain은 관찰0으로 미검증이다.

## 건강·지식·사회·기관

- 874213954: illness incidents 11, recoveries 10; 300일 subsequent-use 없음 6개 resident/technique records: [('2', 'SharpFlake'), ('4', 'DiggingStick'), ('2', 'ChippedStoneTool'), ('1', 'SharpFlake'), ('1', 'ChippedStoneTool'), ('4', 'DugSanitationPit')].
  - `sanitation`: `{"residue_records": 19, "waste_amount": 12.077350000000651, "active_sites": 1, "usage_count": 961}`
  - `society`: `{"specialized_residents": 3, "educators": 0, "caregivers": 0, "producers": 3, "storekeepers": 0, "exchange_facts": 7, "apprenticeships": 0, "institution_memberships": 15, "active_institutions": 2, "shared_contribution_facts": 513, "durable_record_facts": 0, "record_media_units": 0, "coordinated_residents": 3}`
  - `technology_population`: `{"known_types": 12, "reproducible_types": 12, "common_technologies": 5, "lost_technologies": 0, "declining_technologies": 2, "established_technologies": 5}`
- 4242001: illness incidents 12, recoveries 11; 300일 subsequent-use 없음 5개 resident/technique records: [('1', 'DesignatedSanitationArea'), ('2', 'DesignatedSanitationArea'), ('1', 'DugSanitationPit'), ('2', 'SharpFlake'), ('2', 'ChippedStoneTool')].
  - `sanitation`: `{"residue_records": 16, "waste_amount": 10.455300000000534, "active_sites": 1, "usage_count": 1809}`
  - `society`: `{"specialized_residents": 3, "educators": 0, "caregivers": 0, "producers": 3, "storekeepers": 0, "exchange_facts": 31, "apprenticeships": 0, "institution_memberships": 16, "active_institutions": 3, "shared_contribution_facts": 715, "durable_record_facts": 0, "record_media_units": 0, "coordinated_residents": 3}`
  - `technology_population`: `{"known_types": 12, "reproducible_types": 12, "common_technologies": 7, "lost_technologies": 0, "declining_technologies": 1, "established_technologies": 7}`

생존 주민만 300일 미사용 경고에 포함한다. 사망 주민의 usage 정지는 dead-system 증거가 아니다. 300일 미사용은 경고이며 knowledge counter가 passive sanitation 효과나 발견 시 successfulUses를 표현하는 한계가 있다. discovery→first subsequent increment latency를 수집했지만 first practical product latency는 일부만 확정 가능하다. 사회행동0과 social facts/지식전달0은 같은 뜻이 아니다. teaching 시간을 별도 수집하며 specialization/기관 facts도 존재한다. participation fact만으로 선택 행동시간 인과를 증명할 수 없다. grief/social response 미관찰은 기능 부재와 구분한다.

## 성능·상태 성장

| seed | day | snapshot bytes | peak RSS KiB | resident/facility/nodes/chunks | memories/beliefs/social/receipts/relations/events | wall ms |
|---|---:|---:|---:|---|---|---:|
| 874213954 | 1 | 58087 | 15016 | 4/3/33/2 | 46/34/22/32/12/353 | 349 |
| 874213954 | 7 | 237159 | 15016 | 4/10/33/2 | 266/74/144/161/12/2107 | 1552 |
| 874213954 | 30 | 524737 | 15016 | 4/12/33/2 | 1134/110/727/796/12/8694 | 7301 |
| 874213954 | 100 | 848459 | 15016 | 4/34/513/32 | 2319/128/1020/1099/12/28645 | 80952 |
| 874213954 | 365 | 1719639 | 19328 | 3/66/943/62 | 5826/135/1733/1816/12/87958 | 341552 |
| 874213954 | 1000 | 3489811 | 36256 | 3/69/1074/72 | 13886/135/2663/2747/12/221473 | 1089650 |
| 4242001 | 1 | 55369 | 15016 | 4/4/32/2 | 45/26/28/32/12/329 | 705 |
| 4242001 | 7 | 202357 | 15016 | 4/13/32/2 | 188/55/84/92/12/1999 | 3080 |
| 4242001 | 30 | 343802 | 15016 | 4/16/32/2 | 655/93/318/335/12/7738 | 13583 |
| 4242001 | 100 | 854814 | 15016 | 4/38/432/28 | 2265/146/1180/1236/12/26150 | 50894 |
| 4242001 | 365 | 1967818 | 21404 | 3/70/1508/99 | 6547/163/2061/2127/12/94176 | 586845 |
| 4242001 | 1000 | 3996276 | 38300 | 3/81/1594/104 | 14588/173/4351/4433/12/217380 | 1811699 |

Checkpoint JSON에 구간별 chunks/day, snapshot bytes/day, memory entries/resident-day, wall ms/day를 포함했다. monotonic 성장만으로 성능 붕괴 시점을 단정하지 않는다. 현재 4명 start→3명 규모이며 큰 인구 규모를 검증한 성능 결과가 아니다. Core persisted log tail cap과 observer 전체 event archive 크기를 구분한다.

## A–M 관측 범위와 남은 빈칸

| 영역 | 수집/증거 | 한계 |
|---|---|---|
| A Population/Family | resident stage/parents/founders/generation, overview, life history, death events | adult descendant horizon>=6570일 필요 |
| B Needs | resident/global histogram mean/max/p50/95/99, entries/time/streak, preemption/failure, Core starts/done와 완료율, sampled travel sessions | instantaneous Eat 세션 거리/시간 표본 없음; onset/completion event는 legacy JSON에 포함 |
| C Budget | 22 exclusive categories, resident/전체 누적 % | background romance/family/economy/institution/migration 비용 독립 분리 불가 |
| D Resources | 16 materials natural/carried/stored/Gather/Store/Retrieve/zero duration/construction current demand | accessible/shortage demand·gross flow ledger 없음 |
| E Facilities | kind/state/durability/use/last-use/location/start/end | 전체 시설 usage coverage·정확 repaired/abandoned event 부족 |
| F Exploration | token starts/ends/cause material/pressure/route evidence/distance/chunk coords/new-node depletion/visits | materialized frontier 노드 감소는 정확 consumption 아님; max settlement-distance는 별도 causal probe 필요 |
| G Settlement | daily active/assigned/empty/시설/storage service area, resident migration pressure/candidates | household/group actual migration 본 seed0; pressure duration은 legacy resident counters |
| H Trade | raw departure/exchange/return/preemption/resume/end duration/repeated actor/active phase | candidate/outbound-return start 이벤트 전체를 구조화한 ledger 아님 |
| I Health | water dose/local exposure/noon before-after/pathogen/severity/immunity/capacity/care/Needs/deaths/recovery/sanitation | pathogen 종류·care attempt 치료 effectiveness 상세 없음 |
| J Knowledge | resident knowledge levels/first/subsequent increment, population tech/capabilities/roles/institutions/records | practical usage 모든 기술에 대한 product linkage 없음 |
| K Social | actual social budget/Core events/pair interaction minutes/romance overview/parenting/teaching | same-pair 취합은 action-kind별 상호작용 분리 아님; avoidance/conflict 세부 ledger 없음 |
| L Performance | wall/peak RSS/save bytes/nodes/chunks/facts/memory/beliefs/receipts/relationships/events/slopes | concurrent machine, observer/encoding overhead 포함, small population; 대규모 collapse 임계점 미검증 |
| M Determinism | complete marker/invariants/full streamed records/save SHA256 + baseline identity | wall/RSS 제외; reporting-only writer 보정 smoke 별도 |

## 정상 확인과 실제 다음 수정 TOP5

- 정상: crash 없음, 완료 snapshot decode/reencode/restore, 음수 inventory/dead resident action 불변식, Core observer state identity, 874 교역 교환·귀환·preemption 재개, 실제 crafting/cultivation/teaching와 retained knowledge, founder 생존자 유지.
- 미검증: 출생자 성인/다음 세대 reproduction, household migration, 큰 인구의 scale, 모든 resource gross flow.

1. Social first-contact eligibility/관계 bootstrap 선택 경로를 먼저 재현·수정. fertility probability부터 바꾸지 않는다.
2. 424 Soyeon 급수 목적지/route/해결량과 critical thirst 유지 경로를 세부 trace로 고정한 후 provision/navigation 수정.
3. 오염수 dose와 sanitation/care 실제 효과의 통합 경계, clearance/recovery 지연을 수정 검토.
4. Tin discovery→local accessible acquisition→smelting 실사용 연결을 복구.
5. 이동 캠프별 시설 수요·공유·repair/rebuild topology를 먼저 계측하고 중복건설 원인을 수정.

수정 제안은 별도 PR이다. 이번 PR에서 decay/threshold/probability/utility/cost/speed/duration/cap/scripted seed 예외를 변경하지 않았다. P3 gross resource ledger와 시설/process counters는 위 수정들의 정확한 regression evidence를 위해 함께 보강할 대상이다.
