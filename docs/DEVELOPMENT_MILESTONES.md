# LifeLens Development Milestones

이 문서는 **LifeLens의 canonical execution roadmap**이다.

- 장기 제품/문명 방향: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`
- 시간/배속/환경 계약: `docs/TIME_AND_DYNAMIC_ENVIRONMENT.md`
- 현재 실행 상태: `tasks/WORK_STATE.md`
- ownership / locks / IR: `tasks/TEAM_BOARD.md`
- 2026-09-17 통합감사: `docs/INTEGRATED_AUDIT_2026-09-17.md` — historical point-in-time evidence로 보존

---

# 1. Product North Star

> **초기 인간 4명으로 시작하여, 외부 스크립트가 시대를 강제하지 않고 생존·지식·발견·사회 변화의 결과로 원시사회부터 현재 인류 수준을 지나 아직 현실세계가 마주하지 않은 미지의 미래문명까지 자율적으로 발전할 수 있는 관찰형 시뮬레이션.**

시대 이름은 Observer 요약일 뿐 Core의 강제 진행 게이트가 아니다.
장기 발전의 실제 기반은 다음이다.

`Environment / Needs / Social pressure`
→ `Knowledge`
→ `Capability`
→ `Technology`
→ `Civilization Transformation`

시간과 환경은 장식이 아니라 위 발전 루프의 핵심 입력이다.

---

# 2. Delivery / Authority Rules

> Same purpose + same layer + same validation scope = one milestone-sized PR.

고정 원칙:

- Core / World is the sole simulation authority.
- Presentation consumes read/action contracts and never fabricates outcomes.
- stale branch는 wholesale merge하지 않는다.
- latest `main`에서 실제로 빠진 변화만 안전하게 재적용한다.
- 기능 PR의 merge evidence는 exact-head CI를 기준으로 한다.
- 시간 경과만으로 기술/시대를 무료 해금하지 않는다.
- 자원/에너지/시설/지식/운영 능력이 없는 기술은 실제 Capability가 아니다.
- 쇠퇴, 지식 소실, 붕괴, 재발견을 허용한다.
- 시간 가속은 causality를 생략해서는 안 된다.
- 날씨/계절은 visual-only effect가 아니라 Core 생활/자원/행동에 실제 영향을 줘야 한다.

---

# 3. Current Baseline / Completed Foundations

완료된 주요 기반:

- Character / Appearance / World Affordance / Residue / Runtime ACK / Save-Load.
- World Genesis WG-1/WG-2 / generated natural world / production map integration.
- Context Action Contract v1 / spatial authority / Observer camera + resident detail.
- Lifecycle Core Correctness #115.
- Action Completion Unification #116.
- Social Communication & Localization #117.
- Facilities / Tools / PrimitiveStorage #118.
- Tool Effectiveness #119.
- DiggingStick / StoneHammer #120.
- Fire / Heat / FirePit #121.
- Furnace / Copper Smelting #122.
- World / Facility / Obstacle Authority Normalization #123.
- Legacy Authority Removal v1 #124.
- Lifecycle Presentation v1 #125.
- Social bubble readability #126.
- Integrated audit / roadmap reconciliation #127.
- Mac editor `-Wshadow` unblocker #128.
- Knowledge Transmission Spatial Authority v1 #130.
- Emotion Runtime Integration v1 #132 — **MERGED**.
- PR #100 World Readability source current-main reapplication via #134 — **MERGED**.

## PR #100 / #134 closeout

Original Dagyeom PR #100 was not merged wholesale after becoming stale.
Its validated `WorldPresentation` source changes were reconstructed on current main as PR #134.

#134 exact-head evidence:
- Preflight #723 — PASS.
- Unreal Linux Compile #207 — PASS.
- merge commit on main: `1195fddbaf3341ac9508347d07fab69740f02482`.

Therefore:
- #134 = DONE.
- original #100 = **superseded / closeout complete**.
- Dagyeom Presentation lane no longer waits for #100.

---

# 4. New Priority Order

이번 재정렬의 핵심은 **시간과 동적 환경을 정착/농업/장기 문명보다 먼저 안정화**하는 것이다.
이유는 농업, 수면, 저장, 장기 생존, 세대 진행, 건강, 기술 압력 모두 시간/환경 축을 공유하기 때문이다.

## Priority 0 — Documentation / Integration Closeout

1. #134 merge state를 canonical docs에 반영.
2. #133 roadmap/docs PR closeout.
3. #100은 superseded로 정리.
4. Android Gate B는 계속 PAUSED BY USER.

이 단계가 끝나면 다음 기능 작업은 아래 P0 Core 순서로 진행한다.

---

# 5. P0 Core / World Foundation Track

## Milestone T1 — Simulation Time Authority & Variable Speed

Owner: Jjun Core/World + Dagyeom Observer control consumer.
Canonical spec: `docs/TIME_AND_DYNAMIC_ENVIRONMENT.md`.

### Target rule

**기본 1x = 현실 8분 / LifeLens 1일.**

현재 main의 기존 값 `0.6 real sec / simulation minute`는 약 14분 24초/일이므로,
T1에서 target `0.333333... real sec / simulation minute`로 전환한다.

### Required controls

- Pause — 0x
- Observe — 1x — 8분/일
- Fast — 4x — 2분/일
- Faster — 16x — 30초/일
- Rapid — 64x — 7.5초/일
- History — adaptive long-run mode

### Scope

- Core simulation clock authority 명확화.
- Unreal render clock과 simulation clock 분리.
- speed preset/runtime config.
- pause/resume.
- speed change 중 pending ACK/action correctness.
- Save/Load deterministic continuation.
- per-frame catch-up budget.
- History mode가 사용할 batching hook 준비.

### Acceptance

- 속도를 변경해도 Needs/action/lifecycle 결과 순서가 깨지지 않는다.
- Presentation FPS가 simulation truth를 결정하지 않는다.
- 1x target이 8분/일 기준으로 동작한다.
- 64x에서도 한 프레임 무한 catch-up이 발생하지 않는다.

---

## Milestone E1 — Calendar + Day/Night Authority

Owner: Jjun Core provider -> Dagyeom WorldPresentation/UI consumer.

Scope:

- total simulation minute.
- minute/hour of day.
- day/year index.
- annual phase.
- derived season summary.
- daylight/darkness state.
- deterministic Save/Load.
- Core read model / Bridge exposure.

Gameplay hooks:

- sleep/circadian utility.
- night visibility/risk.
- outdoor work/travel efficiency.
- fire/lighting/shelter utility.

Presentation:

- sun movement.
- sky/atmosphere transition.
- night lighting/moon path where justified.
- Observer date/time indicator.

---

## Milestone E2 — Seasons + Dynamic Weather Core v1

Owner: Jjun Core/World provider.

Existing baseline to preserve:

- biome / natural surface.
- elevation.
- moisture.
- temperature.
- water potential.
- fertility potential.
- traversal ease.
- hazard potential.
- renewable resource regeneration.

Add time-varying layer:

- local air temperature.
- precipitation intensity/type.
- cloud cover.
- wind.
- humidity/wetness tendency.
- visibility modifier.
- soil/surface moisture modifier.
- continuous seasonal modifier.

Derived Observer labels may include:
Clear / Cloudy / Rain / Snow / Fog / Storm / Heat / Cold.

Rules:

- labels are summaries, not authority.
- weather is deterministic from seed/time/ruleset/region state.
- real-world weather API is not simulation truth.
- Save/Load reproduces continuation.

---

## Milestone E3 — Environmental Consequences v1

Owner: Jjun Core/Simulation provider -> Dagyeom Presentation consumer.

Connect environment to actual life:

- heat/cold exposure.
- shelter/fire/clothing value hooks.
- rain/wetness work/travel friction.
- water availability/replenishment.
- seasonal plant-food regeneration.
- agriculture/fertility modifier.
- fire reliability.
- storm/extreme-condition hazard hooks.
- environment memory/avoidance extension.

Acceptance:

- 비/눈/계절이 화면만 바꾸고 Core 결과가 같아서는 안 된다.
- 환경 변화가 survival utility와 resource pressure를 실제로 바꾼다.
- future technologies can reduce or transform those pressures through real capabilities.

---

## Milestone E4 — Dynamic Environment Presentation v1

Owner: Dagyeom WorldPresentation.

Provider dependency: E1/E2 authoritative read contracts.

Scope:

- day/night light + sky.
- weather visual state.
- rain/snow/fog/cloud presentation.
- seasonal ground/vegetation readability where assets/budget permit.
- Observer current date/time/season/weather summary.

Android policy:

- scalable quality tiers.
- light/sky parameter changes first.
- Niagara/weather VFX budgeted separately.
- distant/unobserved areas do not require full visual weather simulation.
- high speed may reduce VFX/UI refresh frequency.

---

# 6. P1 Survival / Settlement Track

## Milestone C1 — Settlement & Subsistence Foundation

Owner: Jjun Core/World provider -> Dagyeom presentation consumer.

This expands the old narrow `Civilization Phase 2` interpretation.
The current simulation reaches Copper Smelting but still lacks a durable settlement/food substrate for centuries of life.

Priority scope:

- SleepingPlace.
- Shelter.
- WorkSurface.
- facility effects on sleep/comfort/work efficiency.
- cultivation/agriculture foundation.
- renewable food production.
- water handling/storage.
- settlement resource pressure/maintenance.
- TinOre.
- Bronze/bronze tools after real prerequisites.

New environment dependency:

- crop growth depends on time/season/moisture/fertility.
- shelter value depends partly on temperature/weather.
- food storage pressure changes with harvest/season availability.
- water systems respond to environment rather than infinite static supply.

Rules:

- no free starting infrastructure.
- no era auto-spawn.
- every facility requires material/work/knowledge.
- farming does not create food without land/time/input constraints.

Acceptance:

- New Game can move from gathering toward reproducible settlement economy by actual actions.
- failed settlement may regress.
- seasonal/resource shocks can expose weak settlement design.

---

# 7. P1 Long-Run Reliability Track

## Milestone C2 — Long-Run Scale, History Fast-Forward & Cleanup

Owner: Jjun.

This remains ahead of large historical/future content expansion.

Scope:

- deterministic multi-generation / multi-century headless harness.
- explicit per-frame simulation work budget.
- History-mode daily/weekly/monthly batching where equivalence is proven.
- event-aware slowdown hooks.
- population CPU/memory stability.
- snapshot size/restore cost review.
- residue/HISM/environment refresh profiling.
- inactive/distant simulation strategy.
- remove legacy snapshot/Preflight compatibility baggage after verification.

Acceptance:

- centuries can be simulated without a single-frame death spiral.
- acceleration does not silently skip causal outcomes.
- long-run optimization does not create a second authority.

---

# 8. P2 Open-Ended Civilization Framework

## Milestone C3 — Open-Ended Civilization Framework v1

Owner: Jjun Core/Simulation/Save/Bridge.
Canonical architecture: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`.

Core model:

1. Knowledge — what individuals/groups know.
2. Capability — what they can actually do.
3. Technology — reproducible invention/process.
4. CivilizationTransformation — measurable social/production consequence.

Scope:

- stable Capability/Technology identity/versioning.
- existing primitive Technique compatibility mapping.
- prerequisite/effect graph.
- survival/environment/resource pressure -> research/experiment motivation.
- experiment -> discovery -> reproducibility.
- physical knowledge diffusion.
- adoption separate from discovery.
- loss/rediscovery.
- Observer read model.
- deterministic snapshot roundtrip.

Environment is now a first-class innovation driver:

- cold can pressure shelter/heating/clothing.
- drought can pressure storage/irrigation/wells/migration.
- night can pressure lighting.
- resource seasonality can pressure preservation/storage/trade.

---

# 9. P2 Population / Society Foundations

## Milestone C4 — Health / Disease / Population Resilience

Scope candidates:

- pathogens/infection.
- contaminated water/soil.
- sanitation-linked health.
- illness/recovery.
- temperature/weather exposure health effects.
- accident/environment mortality.
- immunity/resilience.
- health knowledge/care progression.

---

## Milestone C5 — Education / Recording / Specialization / Economy / Institutions

Scope:

- family/apprenticeship teaching expansion.
- records/writing precursor.
- education throughput.
- jobs/roles/specialization.
- production/storage demand signals.
- exchange/trade foundation.
- generic ownership/shared-resource policies.
- institutions/group coordination.
- technology adoption/acceptance.
- research/education organizations.

No one real-world political/economic system is a mandatory endpoint.

---

## Milestone C6 — Migration / Multiple Settlements / Trade Networks

Scope:

- carrying-capacity pressure.
- exploration.
- household/group migration.
- settlement founding/abandonment.
- multiple settlements.
- local knowledge/capability differences.
- trade routes/resource specialization.
- conflict/cooperation foundations.
- weather/resource pressure as migration drivers.

---

# 10. Presentation Track — Parallel, Not Blocking Core

## Milestone P2 — Character Context Motion v2

Owner: Dagyeom Character Presentation.

Scope:

- Sit / Stand / Lie / Wake only with compatible facilities.
- PickUp / Carry / Use.
- Gather / Cut / Chop / Dig / Strike.
- Craft / Build / Fire / Smelt.
- Parenting care.
- sanitation interaction.
- teaching/social context motion.
- gaze/body orientation from authoritative targets.

Asset policy:

- existing Quaternius CC0 first.
- never invent furniture/tools through animation.
- animation failure must not fabricate Core completion.

---

## Milestone P3 — Observer Readability, Real Scrolling & Time Controls

Owner: Dagyeom UI/Observer.

Scope:

- true scrollable Detail content.
- generated-world readability.
- pause / 1x / 4x / 16x / 64x controls from T1 contract.
- current simulation date/time.
- current season/weather summary from E1/E2.
- emotion/teaching/lifecycle/history authoritative labels.
- future civilization capability/history surfaces without cluttering main view.

---

## Milestone P4 — Lifecycle Event Presentation v2

Scope:

- pregnancy/birth visibility.
- growth transition visibility.
- death/history visibility.
- deceased inspection from Core history.
- truthful optional death animation.
- family/history feed integration.

---

# 11. Historical-to-Future Content Expansion

These are **content bands on top of C3**, not forced era gates.

## F1 — Advanced Metallurgy / Urban / Scientific Accumulation

- Bronze completion.
- Iron/high-temperature metallurgy.
- construction/material sophistication.
- urban infrastructure pressure.
- measurement/recording.
- structured experimentation.

## F2 — Mechanical / Industrial Civilization

- mechanical power.
- precision tooling.
- pumps/engines.
- scalable manufacturing.
- transport infrastructure.
- industrial externalities.

## F3 — Electrical / Chemical / Modern Infrastructure

- electricity generation/transmission/storage.
- advanced chemistry/materials.
- sanitation/medical infrastructure.
- mass production.
- motorized transport.
- high-throughput communications.

## F4 — Digital / Network / Information Civilization

- computation.
- electronics.
- data storage.
- communication networks.
- software/control abstractions as simulation capabilities.
- automation prerequisites.

## F5 — AI / Robotics / Advanced Automation

- machine perception/control.
- autonomous robotics.
- automated production.
- AI-assisted research/education.
- labor/production structure transformation.

No scripted singularity date.

## F6 — Advanced Energy / Materials / Biotechnology

- high-density energy.
- advanced materials.
- closed-loop production.
- genetics/regenerative medicine/artificial organs.
- longevity with demographic/social consequences.

## F7 — Planetary / Space / Interplanetary Civilization

- launch capability.
- orbital infrastructure.
- closed-loop life support.
- autonomous off-world industry.
- moon/planet/asteroid settlements.

## F8 — Open Future / Unknown Civilization

- compositional innovation from Capability + Knowledge + material + energy constraints.
- deterministic stable technology identity.
- concrete measurable effects.
- multiple future trajectories.
- acceleration, stagnation, collapse and rediscovery.

LifeLens should eventually surprise the observer **without abandoning causality**.

---

# 12. Reconciliation With Previous Roadmap

| Previous item | New disposition | Reason |
|---|---|---|
| Dagyeom #100 closeout | DONE via #134 | validated WorldPresentation source reapplied on current main |
| Character Context Motion v2 | KEEP HIGH / parallel | truthful action embodiment still needed |
| Observer Readability/Scrolling | EXPANDED | now also owns time-speed and environment readouts |
| Lifecycle Event Presentation | KEEP HIGH / parallel | multigeneration observation requires it |
| Civilization Phase 2 | EXPANDED -> Settlement & Subsistence | agriculture/shelter/water are long-run prerequisites |
| Time scale | PROMOTED TO P0 T1 | every lifecycle/environment/long-run system depends on authoritative speed |
| Day/Night / Seasons / Weather | PROMOTED TO P0 E1~E4 | environment must shape life before agriculture/long civilization grows |
| Cleanup / long-run performance | KEEP EARLY | centuries/future simulation requires scale before content explosion |
| Open-Ended Civilization Framework | KEEP BEFORE CONTENT BANDS | prevents hardcoded era ceiling |
| Health/Disease | KEEP AFTER CORE ENVIRONMENT | health can consume real environmental exposure |
| Education/Economy/Institutions | KEEP BEFORE INDUSTRIAL EXPANSION | coordination/knowledge scaling prerequisite |
| Migration/Multi-settlement | AFTER LOCAL FOUNDATION | depends on settlement/economy/long-run reliability |
| AI/advanced energy/biotech/space | LONG-RANGE CONTENT | built on open-ended capability framework |
| Unknown future innovation | NORTH STAR | progression beyond present humanity |

---

# 13. Current Parallel Dispatch

## Jjun Core / World lane

1. **T1 Simulation Time Authority & Variable Speed.**
2. **E1 Calendar + Day/Night Authority.**
3. **E2 Seasons + Dynamic Weather Core v1.**
4. **E3 Environmental Consequences v1.**
5. **C1 Settlement & Subsistence Foundation.**
6. **C2 Long-Run Scale + History Fast-Forward.**
7. **C3 Open-Ended Civilization Framework v1.**
8. C4 Health / Disease / Population Resilience.
9. C5 Education / Recording / Specialization / Economy / Institutions.
10. C6 Migration / Multiple Settlements / Trade Networks.
11. F1 -> F8 historical / modern / future expansion.

## Dagyeom Presentation lane

1. **P2 Character Context Motion v2.**
2. **P3 Observer Readability + Real Scrolling + Time Controls.**
3. **E4 Dynamic Environment Presentation v1** as E1/E2 provider contracts land.
4. **P4 Lifecycle Event Presentation v2.**
5. settlement/civilization/future presentation consumers as Core contracts land.

Dagyeom-owned changes may proceed and merge independently after CI when they do not conflict with active Core provider ownership.
Conflicts must be resolved by comparing both sides; do not overwrite current main blindly.

## Android / Device lane

Gate B remains **PAUSED BY USER**.
Do not launch long Android builds until explicitly resumed.

When resumed:

- prefer cached/fast path after verified engine cache.
- avoid blind repeat of full/seed builds.
- validate APK boot, four founders, Observer controls/data, movement/actions, Save/Load, FPS/memory/thermal baseline.

---

# 14. Immediate Next Feature

After this documentation PR is merged, the next Jjun-owned implementation target is:

> **T1 — Simulation Time Authority & Variable Speed**

The first delivery should establish the 8-minute/day target and safe pause/1x/4x/16x/64x Core contract without yet trying to implement the full weather stack in the same PR.

After T1 is stable, E1/E2 follow so that Settlement & Subsistence is built on a real calendar/day-night/season/weather foundation instead of receiving those systems retroactively.
