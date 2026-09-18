# LifeLens Whole-Source Audit — 2026-09-18

> Scope: LifeLens를 Jjun/Dagyeom 파트로 나누지 않고 하나의 제품으로 보고 Core / Simulation / AI / Save / World / Character / UI / WorldPresentation / CI를 통합 점검한 결과다.
>
> Audit base main: `87020ec85787f1dfb3d03c30dd8fafaeb7b2aafe`
>
> Priority rule: 아래 **P0 수정이 C1-B보다 먼저**다.

---

## 1. 검사 기준

이번 검사는 단순 compile 여부만 본 것이 아니다.

확인 범위:
- production NEW GAME authority / 4 founders / no-free-infrastructure path.
- Core minute stepping / Needs / Utility / Civilization / Social / Family / Lifecycle.
- Save/Load snapshot + deterministic continuation contract.
- ContextAction / external physical execution / ACK boundary.
- time / season / weather / environmental consequences.
- Character motion and referenced animation asset presence.
- Observer HUD / UMG overlays / input ownership.
- WorldPresentation / natural chunk / facility presentation.
- obstacle collision presentation.
- CI workflow coverage and recent green evidence.

최근 기능 baseline validation:
- Core Tests #707 — PASS.
- Core test suite: 67/67 PASS.
- deterministic harness — PASS.
- Preflight #774 — PASS.
- Unreal Linux Compile #235 — PASS.
- snapshot test includes restore 후 10,000 simulation minutes deterministic continuation comparison.

이 결과는 "실기기에서 버그가 절대 없다"는 뜻이 아니다. UHT/UBT/Core regression으로 잡을 수 없는 PIE/Android visual/input/performance 문제는 별도 runtime QA가 필요하다.

---

# 2. P0 — C1-B보다 먼저 수정

## P0-A — Time / Weather / Speed UI authority duplication

### 확인된 상태

현재 main에는 같은 역할의 두 presentation path가 동시에 존재한다.

1. `ALLRuntimeObserverHUD::DrawRuntimeChrome()`
   - date/time/season/weather/temperature.
   - Pause / 1x / 4x / 16x / 64x.
   - 직접 hit rect를 만들고 `ULLSimulationSubsystem::SetSimulationSpeedPreset()` 호출.

2. `ULLObserverTimeWeatherOverlay`
   - date/time/season/weather/temperature.
   - Pause / 1x / 4x / 16x / 64x.
   - `ULLObserverTimeWeatherPresentationSubsystem`이 local game world에서 자동 `AddToViewport(65)`.
   - 동일하게 `SetSimulationSpeedPreset()` 호출.

GameMode의 HUDClass는 현재 `ALLRuntimeObserverHUD`다. 따라서 runtime HUD가 살아 있는 상태에서 UMG overlay subsystem도 자동 생성되어 **같은 책임의 UI 두 벌이 동시에 활성화될 수 있다.**

### 영향

- 우측 상단 중복/겹침.
- 입력 hit area 이중화.
- 같은 기능의 스타일/상태가 두 곳에서 drift.
- "같은 책임은 canonical source 하나" 원칙 위반.
- 앞으로 HUD 변경 때 두 구현을 동시에 유지해야 하는 비용 발생.

### 수정 원칙

**하나의 canonical Observer time/weather/speed surface로 통합한다.**

권장 방향:
- 현재 production HUD hierarchy와 모바일 입력 계약을 기준으로 하나를 canonical로 결정.
- 다른 path는 제거하거나 canonical consumer 내부로 흡수.
- duplicate speed mutation path 제거.
- Core/Simulation time authority는 변경하지 않는다.

### Acceptance

- 화면에 time/weather/speed control surface가 정확히 한 벌만 존재.
- Pause / 1x / 4x / 16x / 64x 모두 동일 subsystem을 변경.
- PC/Android tap hit가 한 번만 처리.
- Preflight + Unreal Compile PASS.
- PIE visual/input smoke에서 중복 없음.

---

## P0-B — Environmental Need Pressure가 resident 위치가 아닌 start region 기준

### 확인된 상태

Core의 per-minute environment path는:

`Simulation::step()`
-> `advancePrimitiveFireOneMinute(world_)`
-> `applyStartRegionEnvironmentalNeedPressure(world)`

로 이어진다.

`applyStartRegionEnvironmentalNeedPressure(World& world)`는 start-region chunk 하나의 DynamicEnvironment를 계산한 뒤 **살아 있는 모든 character에게 동일한 environmental Need pressure를 적용한다.**

반면 travel friction은 `environmentAdjustedTravelTicks(world, from, to)`에서 실제 `from` GridPos의 chunk를 사용하고, facility work도 facility 위치의 environment를 사용한다.

즉 환경 시스템 내부에서도 위치 기준이 일관되지 않다.

### 영향

현재 4명이 시작 지역 근처에 있을 때는 크게 드러나지 않을 수 있다.
하지만 resident가 다른 chunk/기후권으로 이동하면:
- 사막에 간 주민이 시작 지역의 추위 영향을 받을 수 있음.
- 폭풍 지역 주민이 시작 지역 맑음 기준 Need pressure를 받을 수 있음.
- 장기 migration / multi-settlement / open world에서 simulation truth가 틀어짐.

이는 장기 설계인 **환경이 실제 causal pressure**라는 원칙과 직접 충돌한다.

### 수정 원칙

environmental Need pressure를 **resident authoritative runtime GridPos -> resident chunk -> DynamicEnvironment** 기준으로 적용한다.

주의:
- Core가 위치 authority를 가져야 한다.
- Presentation world location을 Core truth로 사용하지 않는다.
- external physical execution 중에도 ACK된 authoritative GridPos와 snapshot runtime position이 일치해야 한다.
- 같은 seed/snapshot continuation은 결정론적으로 유지한다.

### Acceptance

- 서로 다른 chunk에 있는 두 resident가 서로 다른 environment pressure를 받는 regression test.
- start-region resident는 기존 결과와 호환되는 범위 유지.
- Save/Load 후 동일 위치/환경 pressure continuation.
- Core Tests + deterministic harness + Preflight + Unreal Compile PASS.

---

# 3. P1 — P0 뒤 바로 정리할 구조적 위험

## P1-A — WorldPresentation / obstacle proxy가 materialized chunk를 initial-region ring으로 추정

현재:
- `ALLWorldPresentationActor`는 `MaterializedChunkCount`의 sqrt로 ring 크기를 추정하고 initial chunk 주변 좌표를 조회한다.
- `ALLWorldObstacleCollisionProxyActor`도 유사하게 count 기반 ring을 만들고 initial chunk 주변을 탐색한다.

이 방식은 현재 contiguous local materialization에서는 동작하지만,
향후 migration / distant exploration으로 **멀리 떨어진 non-contiguous chunk가 materialize**되면 count만으로 위치를 복원할 수 없다.

### 영향

- Core에는 chunk가 materialized 되었는데 Presentation이 안 그림.
- authoritative obstacle collision proxy가 누락될 수 있음.
- 멀리 떨어진 settlement/expedition가 보이지 않거나 물리 표현이 틀어질 수 있음.

### 수정 방향

Core bridge가 **materialized chunk coordinate 목록을 authoritative read DTO로 직접 제공**하고,
WorldPresentation / collision proxy가 그 목록을 consume한다.

이 항목은 현재 C1 시작지역 생활을 막는 P0는 아니지만,
**C2/C6 migration/multi-settlement 전에 반드시 수정**한다.

---

# 4. 확인상 정상 / 설계와 일치한 핵심

이번 감사에서 다음 기반은 현재 방향과 맞는다.

- production NEW GAME은 정확히 4 founders를 만들고 자연 start region으로 시작.
- production NEW GAME path는 compatibility storage fixture를 clear하여 free starting storage를 주지 않음.
- WorkSurface / SleepingPlace / Shelter는 #152에서 real material + work authority를 가짐.
- Core / Presentation action completion은 ContextAction / external ACK boundary를 사용.
- Save snapshot은 runtime state를 포함하고 deterministic continuation regression이 존재.
- dead resident runtime quiescence, dependent care, kinship romance prohibition, birth position, death cleanup regression이 존재.
- early survival은 실제 Water / PlantFood Gather 후 Eat/Drink를 수행.
- sanitation residue/memory/avoidance가 Core test로 검증됨.
- environmental travel friction / resource regeneration / fire reliability / construction friction이 Core-side 계산.
- Character Context Motion에서 참조하는 주요 Quaternius animation assets가 repository에 실제 존재.
- #148 dynamic canopy path는 ambient canopy를 가역적으로 숨기고 authoritative resource trees를 별도 취급.

---

# 5. 자동검사로 보장되지 않는 영역

아래는 별도 runtime QA가 필요하다.

- 실제 PIE HUD layout / safe area / overlay z-order.
- PC mouse gesture 충돌.
- Android touch tap/drag/pinch/pan 충돌.
- animation montage transition / hand-tool alignment / foot sliding.
- authored Niagara/material 실제 품질.
- rain/snow fallback의 Android GPU 비용.
- 주민 수 증가 시 WorldPresentation / Actor iteration / HISM update cost.
- 장시간 Unreal runtime memory/performance.
- APK packaging/device launch.

Android Gate B는 현재 사용자 요청으로 PAUSED 상태이므로 이 감사가 임의로 APK build를 재개하지 않는다.

---

# 6. 수정 우선순위

**AUDIT-0A**
Time/Weather/Speed UI duplicate authority 통합.

**AUDIT-0B**
Resident-local environmental Need pressure.

**AUDIT-0C**
0A/0B 이후 whole regression:
- Core Tests.
- deterministic harness.
- Preflight.
- Unreal Linux Compile.
- PIE smoke checklist.

**AUDIT-1A**
materialized chunk coordinate enumeration contract.

그 다음 원래 roadmap으로 복귀:

**C1-B Autonomous Settlement Need Recognition**
-> C1-C Facility Effects/Maintenance
-> C1-D Durable Subsistence
-> C1-E Emergent Settlement
-> C1-F Early Material Expansion.

---

# 7. Stop-the-line rule

P0-A/P0-B가 해결되고 regression이 green이 되기 전에는 C1-B 기능 확장을 main에 먼저 쌓지 않는다.

이유:
- P0-A는 presentation responsibility duplication이라 이후 UI 변경 비용을 계속 키운다.
- P0-B는 environment causality의 위치 기준 오류라 Shelter/settlement utility를 붙이기 전에 바로잡아야 한다.

즉 이번 감사 수정은 roadmap 이탈이 아니라 **C1을 잘못된 기반 위에 쌓지 않기 위한 선행 안정화 작업**이다.
