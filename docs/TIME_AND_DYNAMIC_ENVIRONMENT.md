# LifeLens Simulation Time & Dynamic Environment

> Status: **Canonical design contract for simulation time, acceleration, calendar, day/night, seasons and weather**
>
> Execution order: `docs/DEVELOPMENT_MILESTONES.md`
>
> Long-range civilization direction: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`

---

## 1. 목적

LifeLens의 시간과 환경은 단순한 화면 효과가 아니다.
시간의 흐름은 Needs, 수면, 성장, 임신, 출산, 노화, 자원 재생, 농업, 지식 전승, 기술 발전과 문명사의 공통 축이며,
환경은 생존 압력과 행동 선택, 자원 생산성, 정착지의 성패와 기술 혁신 동기를 만드는 Core 입력이다.

따라서 다음 원칙을 고정한다.

- **Core simulation time is authoritative.**
- Unreal Presentation은 Core 시간/환경 상태를 읽어 표현하며 독자적인 날짜·날씨 진실을 만들지 않는다.
- 같은 WorldSeed / ruleset / snapshot / 입력이면 같은 시간·환경 진행을 재현할 수 있어야 한다.
- 시간 가속이 causality를 생략하거나 결과를 fabrication해서는 안 된다.
- 날씨/계절은 시각 효과에 그치지 않고 실제 생활·자원·행동에 영향을 줘야 한다.

---

## 2. 공식 관찰 속도 기준

### 2.1 기본 1x

**목표 기본값: 현실 8분 = LifeLens 1일(24시간).**

계산상:
- 1 simulation day = 1,440 simulation minutes
- 480 real seconds / 1,440 simulation minutes
- target `RealSecondsPerSimulationMinute = 0.333333...`

현재 main의 기존 기본값 `0.6 sec / simulation minute`는 약 **14분 24초 / simulation day**이므로,
시간 시스템 milestone에서 새 canonical target으로 전환한다.

### 2.2 Observer speed presets

| Mode | 배율 | 현실 시간 기준 LifeLens 1일 | 용도 |
|---|---:|---:|---|
| Pause | 0x | 정지 | 관찰/검토 |
| Observe | 1x | 8분 | 일상 생활 관찰 |
| Fast | 4x | 2분 | 짧은 진행 |
| Faster | 16x | 30초 | 며칠~몇 주 관찰 |
| Rapid | 64x | 7.5초 | 계절/단기 세대 진행 |
| History | adaptive | 렌더 프레임과 분리 | 수년~수백년 이상 장기 진행 |

정확한 UI 명칭은 Presentation 단계에서 바꿀 수 있으나 의미와 Core 계약은 유지한다.

---

## 3. History / Long-run Fast-Forward

미래문명까지 가기 위해 단순히 Unreal 전체를 수백/수천 배 재생해서는 안 된다.

History mode는 다음 원칙을 따른다.

1. Core simulation clock과 Unreal rendering clock을 분리한다.
2. 가까운 시점/중요 사건은 minute-level causality를 유지한다.
3. 장기 구간은 검증된 daily/weekly/monthly coarse stepping 또는 event-aware batching을 사용할 수 있다.
4. batching은 합산 결과를 임의 생성하지 않고 동일 규칙의 결정론적 축약이어야 한다.
5. 한 프레임에 무제한 catch-up을 하지 않고 simulation work budget을 둔다.
6. 고배속에서 Character animation, HISM refresh, weather VFX, UI refresh는 낮은 빈도로 갱신할 수 있다.
7. 출생, 사망, 재난, 핵심 발견, 정착지 붕괴, 중요한 사회 사건에서 Observer가 자동 감속/일시정지하는 기능은 선택 옵션으로 둔다.

Acceptance:
- 고배속과 저배속이 규칙적으로 동일한 원인-결과 관계를 보존한다.
- Save/Load 후 속도와 무관하게 동일 snapshot에서 동일 결과를 재현한다.
- 장기 가속 때문에 한 프레임 CPU spike가 무한히 커지지 않는다.

---

## 4. Calendar contract

Core는 최소한 다음 시간을 authoritative state로 제공한다.

- total simulation minute
- minute of day / hour of day
- day index
- year index
- day-of-year or normalized annual phase
- derived season summary

초기 구현은 지구형 24시간/일과 연간 cycle을 사용한다.
달력 명칭이나 월 체계는 장기적으로 문명/문화가 발달하며 별도 표현될 수 있지만,
물리 시뮬레이션의 안정적인 time axis와 혼동하지 않는다.

Season은 단순 `Spring/Summer/Fall/Winter` enum만으로 모든 계산을 결정하지 않는다.
가능하면 **continuous annual phase + 지역 climate parameters**로 온도/일조/강수 경향을 계산하고,
4계절 이름은 Observer용 derived summary로 사용한다.

---

## 5. Day / Night

낮밤은 Presentation 조명만 바꾸는 장식이 아니다.

Core consequences 후보:
- daylight / darkness level
- 야간 이동 및 작업 효율
- 시야/위험 인지
- 수면 utility와 circadian preference
- 야외 활동 위험도
- 불/조명/주거의 utility
- 향후 artificial lighting 기술의 실질적 효과

Presentation responsibilities:
- Sun directional light
- sky/atmosphere
- moon/night light where justified
- exposure tuning
- shadow/readability

Presentation은 Core의 시간 상태를 읽어 표현한다.

---

## 6. Dynamic Environment model

### 6.1 이미 존재하는 기반

현재 자연 월드는 biome/surface와 지역별 다음 baseline을 가지고 있다.

- elevation
- moisture
- temperature
- water potential
- fertility potential
- traversal ease
- hazard potential
- renewable resource regeneration

Dynamic Environment는 이 baseline을 버리지 않고 **time-varying modifier layer**를 추가한다.

### 6.2 최소 authoritative weather state

초기 v1은 지역/청크 단위로 다음 연속 값을 우선한다.

- air temperature
- precipitation intensity/type
- cloud cover
- wind intensity
- humidity / wetness tendency
- visibility modifier
- soil/surface moisture modifier

Observer용으로 Clear / Cloudy / Rain / Snow / Fog / Storm / Heat / Cold 등의 summary label을 파생할 수 있다.
Label 자체가 simulation authority가 되어서는 안 된다.

### 6.3 결정론

날씨 sequence는 WorldSeed + region/chunk identity + time state + ruleset에서 결정론적으로 생성한다.
실제 인터넷 날씨 API는 Core 진실로 사용하지 않는다.

Save/Load는 현재 환경 상태와 필요한 generator state/version을 보존하여 동일 continuation을 재현해야 한다.

---

## 7. Season / Weather gameplay consequences

환경 시스템의 핵심은 **실제 결과**다.

### 온도
- 추위/더위 exposure
- shelter/fire/clothing value
- sleep/work efficiency
- 향후 건강 위험

### 강수/습도
- surface wetness
- water availability / replenishment
- travel/work friction
- fire reliability
- agriculture/fertility modifier

### 계절
- 식물성 식량 availability/regeneration
- 재배/수확 가능성
- daylight duration tendency
- 평균 기온/강수 경향
- 저장 필요성

### 폭풍/극단 환경
- 이동 위험
- 시설 손상/유지보수 압력
- 화재/홍수/가뭄 등 미래 hazard extension hook

---

## 8. Civilization feedback loop

Dynamic Environment는 Open-Ended Civilization의 연구 동기와 직접 연결한다.

예:

`겨울 추위`
→ 체온/수면/작업 부담
→ 불·주거·의복 가치 증가
→ 재료/연료 탐색
→ 개선된 shelter/heating capability

`가뭄`
→ 물/식량 감소
→ 저장/관개/우물/이주 pressure
→ 관찰/실험
→ 새로운 기술/정착 전략

`야간 작업 한계`
→ 생산 시간 제약
→ 불/램프/전기조명 capability의 가치
→ 산업/현대 기술이 실제 생활 시간을 확장

이렇게 환경이 기술 발전의 실질적인 이유가 되어야 한다.

---

## 9. Presentation / Android budget

환경 표현은 Android 우선 정책을 유지한다.

- 낮밤: light/sky parameter 중심
- 비/눈: scalable Niagara/VFX budget
- cloud/fog: device quality tier별 단계화
- 멀리 있는 지역의 환경은 Core state만 유지하고 고비용 VFX는 생략 가능
- 고배속에서는 VFX refresh frequency를 낮출 수 있음
- 환경 Presentation이 Core tick을 block하지 않도록 분리

PC high-quality path는 같은 authoritative state를 더 높은 시각 품질로 표현한다.

---

## 10. Implementation order

1. **Time Authority & Speed Control**
   - 8분/일 target
   - pause / 1x / 4x / 16x / 64x
   - Core/Presentation clock separation
   - Save/Load speed-safe contract
2. **Calendar + Day/Night Core contract**
3. **Day/Night Presentation**
4. **Seasonal climate modifier**
5. **Deterministic Weather Core v1**
6. **Weather Presentation v1**
7. **Environmental effects on Needs/resources/actions**
8. **Agriculture/shelter/fire/clothing integration**
9. **History Fast-Forward / long-run batching**
10. **Hazards / climate trends / advanced environment extensions**

일부 단계는 병렬 진행할 수 있지만 Core authority가 먼저이며 Presentation은 provider contract를 소비한다.

---

## 11. Non-goals for v1

- 실제 지구 기상 예보 재현
- CFD 수준 대기 시뮬레이션
- 모든 청크에 매 프레임 고비용 기상 계산
- 날짜가 지나면 자동으로 기술을 해금하는 era clock
- 계절에 맞춰 자원을 presentation-only로 생성하는 것

LifeLens의 목표는 기상 시뮬레이터 자체가 아니라,
**시간과 환경이 인간 생활과 문명 발전에 의미 있게 작용하는 자율 생활/문명 시뮬레이션**이다.
