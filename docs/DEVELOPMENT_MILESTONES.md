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

Completed major authority/correctness chain:

- #115 Lifecycle Core Correctness.
- #116 Action Completion Unification.
- #117 Social Communication & Localization.
- #118 Facilities / Tools / PrimitiveStorage.
- #119 Tool Effectiveness.
- #120 DiggingStick / StoneHammer.
- #121 Fire / Heat / FirePit.
- #122 Furnace / Copper Smelting.
- #123 World / Facility / Obstacle Authority Normalization.
- #124 Legacy Authority Removal.
- #125 Lifecycle Presentation v1.
- #130 Knowledge Transmission Spatial Authority.
- #132 Emotion Runtime Integration.
- #134 World readability current-main reapplication.

Time/environment foundation is now implemented:

- #135 Simulation Time Authority & Variable Speed.
- #136 Calendar + Day/Night Authority.
- #137 Seasons + Dynamic Weather Core.
- #138 Environmental Consequences.
- #139 Dynamic Environment Presentation Foundation.
- #140 Visual Catch-up v2.
- #142 Observer runtime time/weather/speed chrome.
- #147 Observer time/weather/speed controls.
- #148 Dynamic Observer Canopy Visibility.
- #149 Visual Catch-up v3 packaged rain/snow fallback.

Presentation catch-up is substantially integrated:

- #143 Context Motion v2a.
- #144 Detail Scrolling v1.
- #145 Character Context Motion v2.
- #146 lifecycle live event feed.
- #150 Lifecycle Presentation v2 persistent family/life card and observed timeline.
- #151 Adaptive Observer Information Density.

Settlement foundation has started:

- #152 **C1-A Settlement Facility Authority Foundation** — MERGED.
  - WorkSurface / SleepingPlace / Shelter are now real constructible Core facilities.
  - no free starting infrastructure.
  - deterministic site selection.
  - real material delivery + construction work.
  - weather-sensitive work friction.
  - save/load persistence.

Historical PR #100 remains superseded by #134.
Stale PR #98 remains selective-salvage only and must not overwrite current HUD/controller wholesale.
Android Gate B remains paused by user.

# 4. New Priority Order

## Priority -1 — Whole-Source Audit Stabilization — ACTIVE NOW

Canonical audit:
`docs/SOURCE_AUDIT_2026-09-18.md`

C1-B보다 먼저 다음을 끝낸다.

### AUDIT-0A — Observer time/weather/speed duplicate UI consolidation

Current main contains both:
- `ALLRuntimeObserverHUD::DrawRuntimeChrome()`.
- `ULLObserverTimeWeatherOverlay` auto-created by its presentation subsystem.

Both render time/weather/speed and both can change the simulation speed preset.

Acceptance:
- exactly one production control surface.
- one input/hit path.
- Core time authority unchanged.
- PC/Android interaction checked.
- Preflight + Unreal Compile + PIE smoke green.

### AUDIT-0B — Resident-local environmental Need pressure

Current per-minute environmental Need pressure samples the initial start region once and applies it to all living residents.

Acceptance:
- resident authoritative GridPos -> chunk -> DynamicEnvironment.
- different chunks can produce different Need pressure in the same simulation minute.
- deterministic snapshot continuation remains valid.
- Core regression added.

### AUDIT-0C — Whole regression

Before resuming C1-B:
- Core Tests.
- deterministic harness.
- Structural Preflight.
- Unreal Linux Compile.
- PIE visual/input smoke checklist.

### AUDIT-1A — Materialized chunk enumeration — P1 follow-up

WorldPresentation / obstacle collision currently infer materialized coordinates from count + initial-region ring scanning.

Before migration / multi-settlement:
- expose authoritative materialized chunk coordinate list through Core read bridge.
- consume the list directly.
- remove count-based coordinate guessing.

**Stop-the-line:** AUDIT-0A/B/C must be green before C1-B is merged.

After this stabilization, return to the existing C1 sequence without reordering its product intent.

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

Goal:
move from opportunistic gathering toward a durable, reproducible settlement economy without a forced era switch.

### C1-A — Settlement facility authority — DONE (#152)

- WorkSurface.
- SleepingPlace.
- Shelter.
- deterministic Core-authored construction sites.
- real material requirements.
- real construction work.
- environment work friction.
- operational state + snapshot persistence.
- no free New Game facility.

### C1-B — Autonomous settlement need recognition — NEXT

Residents should choose settlement construction because their lived pressure makes it useful.

Required causal inputs:
- high/repeated sleep pressure or outdoor rest -> SleepingPlace value.
- rain/cold/heat exposure -> Shelter value.
- repeated craft/build demand -> WorkSurface value.
- incomplete project material requirements -> Gather demand for the missing materials.

Required action chain:
`Need/pressure -> Utility -> Plan -> spatial ContextAction -> DeliverMaterial -> Work -> ACK -> Operational`

Rules:
- facility does not appear because C1 is "unlocked".
- resident must possess/deliver actual material.
- Presentation never completes the facility.
- identical seed/snapshot continuation remains deterministic.

### C1-C — Facility effects and maintenance

- SleepingPlace improves sleep recovery/comfort versus outdoor fallback.
- Shelter reduces appropriate weather/environment penalties.
- WorkSurface improves relevant crafting/work throughput or success.
- durability and maintenance become meaningful.
- ruined/inactive facilities stop providing benefit.
- maintenance requires real material/labor rather than passive auto-repair.

### C1-D — Durable subsistence

- water handling / carrying / storage.
- food storage and spoilage pressure.
- cultivation/agriculture foundation.
- renewable food production.
- season/moisture/fertility constraints.
- local scarcity can trigger search/migration pressure.
- farming never creates food without land/time/input constraints.

### C1-E — Emergent settlement form

- frequently used facilities form activity centers through actual use.
- storage/fire/work/sleep functions may cluster naturally.
- sanitation remains outside dense living space.
- household living space may differentiate.
- no hard-coded "town center" authority object is required.

### C1-F — Early material progression after settlement stability

- TinOre.
- Bronze and bronze tools only after physical prerequisites.
- no automatic Bronze Age transition.

Acceptance:
- New Game can progress from no facilities to autonomous construction/use of durable settlement infrastructure.
- settlement economy can fail/regress under weak resources, weather or maintenance.
- seasonal/resource shocks expose real settlement weaknesses.
- Save/Load does not duplicate projects, materials or completion.

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

The 2026-09-18 Presentation Catch-up sprint is substantially complete and no longer blocks C1.

## Milestone P2 — Character Context Motion v2 — DONE / ITERATIVE QUALITY

Merged:
- #143 tool-specific gather motion.
- #145 authority-directive-based Context Motion v2.

Covered presentation categories include gather/tool use, build/fire/smelt, sanitation, parenting/teaching/social where compatible clips exist.

Remaining quality gaps:
- authored sleep/lie/wake assets.
- better carry/eat/drink clips.
- higher-quality age/child-specific anatomy and animation.
- runtime visual QA.

Authority rule remains:
animation never fabricates completion or bypasses Core ACK.

## Milestone P3 — Observer Readability, Real Scrolling & Time Controls — DONE / ITERATIVE QUALITY

Merged:
- #142 runtime chrome.
- #144 true Detail scrolling: PC wheel + Android one-finger drag.
- #147 time/weather/speed controls.
- #148 dynamic canopy visibility.
- #151 adaptive information density.

Remaining:
- selective panel-opacity salvage from stale #98 if still beneficial.
- device-specific safe-area/layout tuning.
- camera/readability QA.

## Milestone P4 — Lifecycle Event Presentation v2 — DONE / ITERATIVE QUALITY

Merged:
- #146 pregnancy/birth/growth/death event feed.
- #150 selected-resident persistent family/lifecycle card + observed session timeline.

Remaining:
- persistent full historical LifeHistory read model when Core exposes it.
- deceased inspection polish.
- optional truthful death-specific animation.
- better child/life-stage visual assets.

## Milestone P5 — Dynamic Environment Visual Quality — ACTIVE ASSET/POLISH PATH

Merged:
- #139 environment presentation foundation.
- #140 surface weather/Niagara/post-process/HISM budget.
- #149 packaged code fallback for visible rain/snow when authored Niagara systems are absent.

Next quality path:
- authored Niagara systems/materials.
- stronger wet/snow surface art.
- fire/furnace light/smoke polish.
- Android LOD/performance validation.

Presentation quality may continue in parallel, but should not create a second simulation truth or block C1 unless a real contract gap is found.

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

## Whole-product stabilization lane — FIRST

1. **AUDIT-0A** — consolidate duplicate Observer time/weather/speed UI.
2. **AUDIT-0B** — resident-local environmental Need pressure.
3. **AUDIT-0C** — full regression + PIE smoke.
4. **AUDIT-1A** — explicit materialized chunk enumeration before migration/multi-settlement.

Ownership labels do not split this audit. The repository is reviewed and stabilized as one LifeLens product.

## C1 lane — resumes after AUDIT-0C

1. C1-B Autonomous Settlement Need Recognition.
2. C1-C Facility Effects + Maintenance.
3. C1-D Water/Food Persistence + Cultivation.
4. C1-E Emergent Settlement Geometry.
5. C1-F Tin/Bronze after real prerequisites.
6. C2 Long-Run Scale + History Fast-Forward.
7. C3 Open-Ended Civilization Framework v1.
8. C4 Health / Disease / Population Resilience.
9. C5 Education / Recording / Specialization / Economy / Institutions.
10. C6 Migration / Multiple Settlements / Trade Networks.
11. F1 -> F8 historical / modern / future expansion.

## Presentation quality lane

The previous catch-up remains integrated through #151/#154.
Only incremental quality work continues unless it intersects AUDIT-0A.

## Android / Device lane

Gate B remains PAUSED BY USER.
Do not launch a long Android build until the user explicitly resumes it.

# 14. Immediate Next Feature

Current immediate implementation target:

> **AUDIT-0A — consolidate duplicate Observer time/weather/speed UI.**

Then:
`AUDIT-0B -> AUDIT-0C -> C1-B`

C1-B remains the next product feature milestone, but it is temporarily blocked by the audit stabilization gate so settlement is not built on a duplicated UI responsibility or incorrect environment-location basis.
