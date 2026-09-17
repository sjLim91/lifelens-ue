# LifeLens Development Milestones

이 문서는 **큰 개발 단위와 integration gate의 canonical roadmap**이다.

- 장기 제품/문명 아키텍처 방향: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`
- 현재 실행 상태: `tasks/WORK_STATE.md`
- ownership / locks / IR: `tasks/TEAM_BOARD.md`
- 2026-09-17 통합감사: `docs/INTEGRATED_AUDIT_2026-09-17.md` — 역사적 point-in-time evidence로 보존

## Product North Star

> **초기 인간 4명으로 시작하여, 외부 스크립트가 시대를 강제하지 않고 생존·지식·발견·사회 변화의 결과로 원시사회부터 현재 인류 수준을 지나 아직 현실세계가 마주하지 않은 미지의 미래문명까지 자율적으로 발전할 수 있는 관찰형 시뮬레이션.**

시대 이름은 Observer 요약일 뿐 Core의 강제 진행 게이트가 아니다.
장기 발전의 실제 기반은 `Knowledge -> Capability -> Technology -> Civilization Transformation`이다.

## Delivery rule

> Same purpose + same layer + same validation scope = one milestone-sized PR.

기본 원칙:
- Core / World is the sole simulation authority.
- Presentation consumes read/action contracts and never fabricates outcomes.
- stale branches are not merged wholesale.
- latest `main`에서 실제로 빠진 변화만 재구현한다.
- functional merge evidence는 exact-head CI를 기준으로 한다.
- 시간 경과만으로 기술/시대를 무료 해금하지 않는다.
- 자원/에너지/시설/지식/운영 능력이 없는 기술은 실제 Capability가 아니다.
- 쇠퇴, 지식 소실, 붕괴, 재발견을 허용한다.

---

## Completed foundations

주요 완료 기반:
- Character / Appearance / World Affordance / Residue / Runtime ACK / Save-Load — DONE.
- World Genesis WG-1/WG-2 / World Generation Milestone A / production map integration — DONE.
- Context Action Contract v1 / spatial authority / Observer camera + resident detail — DONE.
- Lifecycle Core Correctness #115 — DONE.
- Action Completion Unification #116 — DONE.
- Social Communication & Localization #117 — DONE.
- Facilities / Tools / PrimitiveStorage #118 — DONE.
- Tool Effectiveness #119 — DONE.
- DiggingStick / StoneHammer #120 — DONE.
- Fire / Heat / FirePit #121 — DONE.
- Furnace / Copper Smelting #122 — DONE.
- World / Facility / Obstacle Authority Normalization #123 — DONE.
- Legacy Authority Removal v1 #124 — DONE.
- Lifecycle Presentation v1 #125 — DONE.
- Social bubble readability #126 — DONE.
- Integrated audit / canonical roadmap reconciliation #127 — DONE.
- Mac editor `-Wshadow` unblocker #128 — DONE.
- Knowledge Transmission Spatial Authority v1 #130 — DONE.

### PR #132 — Emotion Runtime Integration v1 — CLOSEOUT

Owner: Jjun Core provider.
Branch: `jjun/emotion-runtime-integration-v1-20260917`.
Current exact head: `0327afb58caed550b8bb12f046b2ec6738157ef5`.

Delivered:
- Needs pressure drives bounded anxiety/fear.
- successful Need resolution drives relief/joy.
- repeated authoritative failures drive distress.
- civilization success/discovery/failure affects emotion.
- physical utility emotion influence is bounded to +/-8% so survival Needs remain dominant.
- existing social/family/loss/environment paths remain intact.
- production external physical completion now applies the same Need-resolution emotion relief as standalone Core execution.
- regression coverage verifies the production external physical relief path.

Semantics:
- authoritative lived failures may change emotion;
- stale/wrong/far ACK, restore-time cancellation and presentation/control-plane timeout do not fabricate emotional failure by themselves.

Validation on current head:
- Preflight — PASS.
- Core Tests — PASS.
- Unreal Linux Compile — exact-head closeout run still required before merge.

Do not mark DONE or merge until the final exact-head Unreal compile passes.

---

## Gate A — Integrated Runtime Checkpoint A — DONE

Acceptance already established:
- NEW GAME boots with four founders, 2 male / 2 female.
- initial world/population generation is deterministic.
- no free starting house/toilet/farm/storage/road/tool/modern infrastructure.
- missing sanitation creates authoritative HumanWaste residue.
- Save/Load restores state without identity reroll.
- continuation remains deterministic.

---

## Correctness / Authority Chain — DONE THROUGH #130, #132 CLOSEOUT

Completed authority chain:
1. Lifecycle Core Correctness — #115.
2. Action Completion Unification — #116.
3. Social Communication/Localization — #117.
4. Facilities/Tools/Fire/Smelting — #118~#122.
5. World/Facility/Obstacle Authority Normalization — #123.
6. Legacy Authority Removal v1 — #124.
7. Lifecycle Presentation v1 — #125.
8. Knowledge Transmission Spatial Authority v1 — #130.

Current closeout:
9. Emotion Runtime Integration v1 — #132.

Result:
- Social/Civilization/Parenting outcomes require completion-aware authority.
- Core owns target/resource/facility/lifecycle/knowledge/emotion truth.
- knowledge witness is spatially local rather than globally telepathic.
- direct teaching is a physical `KnowledgeTeaching` ContextAction and applies only after real approach/talking/ACK.
- pending teaching is not replayed after Save/Load.
- emotion increasingly reflects lived runtime rather than decorative presentation state.

---

# Immediate Product / Presentation Track

These items are high priority because the simulation truth already exists but the observed world must communicate it faithfully.

## Milestone P1 — Dagyeom World Readability Envelope — ACTIVE PR #100 / REFRESH REQUIRED

Owner: Dagyeom WorldPresentation.
Branch: `dagyeom/world-visual-readability-envelope`.
Last known head: `62b191a6afb2c0c5ae32ddbb1e0ae7876ae00556`.

Purpose:
- improve resident/facility readability without changing Core world/resource authority.

Old validation evidence:
- Preflight #707 PASS.
- Unreal Linux Compile #200 PASS.
- Mac editor build blocker removed by #128.

**Live repository state no longer reports this PR mergeable against current main.**
Therefore old “PIE only then merge” guidance is obsolete.

Required closeout order:
1. Dagyeom owner refreshes/reconstructs the still-needed WorldPresentation changes on current main.
2. resolve conflicts without importing stale Core authority.
3. fresh exact-head Preflight + Unreal Linux Compile.
4. Mac PIE visual confirmation.
5. merge only if visually and structurally acceptable.

Do not merge merely to clear the queue.

---

## Milestone P2 — Character Context Motion v2

Owner: Dagyeom Character Presentation.

Provider status:
- authoritative ContextAction target/ACK contracts are merged.
- KnowledgeTeaching has target resident + technique + token.
- physical actions must correspond to real facilities/tools/resources.

Scope:
- Sit / Stand / Lie / Wake only when compatible real facilities exist.
- PickUp / Carry / Use.
- Gather / Cut / Chop / Dig / Strike.
- Craft / Build / Fire / Smelt.
- Parenting care.
- sanitation interaction.
- teaching / social context motion.
- gaze/body orientation from authoritative targets.

Asset policy:
- existing Quaternius CC0 animations first.
- never show sitting/lying on invented furniture.
- animation failure must not fabricate Core completion.

---

## Milestone P3 — Observer Readability + Real Scrolling

Owner: Dagyeom UI/Observer.

Rules:
- old PR #98 is stale; do not merge wholesale.
- selectively reimplement useful concepts on current main.

Scope:
- true scrollable Detail content;
- generated-world readability tuning;
- QA resident framing helpers separated from production camera behavior;
- emotion / teaching / lifecycle / history labels consume authoritative provider data;
- prepare Observer surfaces for later civilization capability/history views without cluttering the main screen.

---

## Milestone P4 — Lifecycle Event Presentation v2

Owner: Dagyeom Presentation.

Already done:
- dead residents removed from living physical projection;
- stage growth updates body/capsule scale.

Next:
- pregnancy/birth visibility where authoritative data exists;
- growth transition visibility;
- death/history visibility;
- deceased inspection from Core DTO/history;
- truthful optional death animation without inventing corpse/grave authority;
- family/history feed integration.

---

# Civilization / Long-Run Core Track

## Milestone C1 — Settlement & Subsistence Foundation

Owner: Jjun Core/World provider -> Dagyeom presentation consumer.

This replaces the narrow old `Civilization Phase 2` interpretation.
The current simulation already reaches Copper Smelting, but it lacks the durable settlement/food substrate needed for centuries of population growth.

Existing stable progression:
- PrimitiveStorage;
- SharpFlake / StoneCuttingTool / DiggingStick / StoneHammer;
- FirePit / Heat / Charcoal;
- Furnace / CopperOre / CopperMetal / CopperSmelting.

Priority scope:
- SleepingPlace;
- Shelter;
- WorkSurface;
- facility effects on sleep / comfort / work efficiency;
- cultivation/agriculture foundation;
- renewable food production rather than indefinite gathering dependence;
- water handling/storage where needed;
- settlement resource pressure and maintenance;
- TinOre;
- Bronze / bronze tools after actual prerequisites exist.

Rules:
- no free starting infrastructure;
- no “era reached” auto-spawn;
- every facility requires actual material/work/knowledge;
- farming does not create food without land/time/input constraints.

Acceptance direction:
- a New Game can transition from gathering toward a reproducible settlement economy through actual actions;
- a failed settlement may regress rather than receiving rescue resources.

---

## Milestone C2 — Long-Run Scale & Cleanup

Owner: Jjun.

**Priority moved earlier.**
Open-ended civilization is meaningless if the simulation cannot survive centuries/many generations efficiently.

Scope:
- remove `SimulationSnapshotCodecLegacy.cpp` after compatibility checks are updated;
- remove stale Preflight compatibility markers;
- explicit per-frame simulation catch-up budget;
- residue/HISM refresh profiling;
- population growth CPU/memory stability;
- snapshot size/restore cost review;
- inactive/distant simulation strategy preparation where necessary;
- deterministic multi-generation / multi-century headless regression harness.

Acceptance principle:
- long time acceleration must not create a single-frame death spiral;
- performance optimizations must not create a second authority or silently skip required causal outcomes.

---

## Milestone C3 — Open-Ended Civilization Framework v1

Owner: Jjun Core/Simulation/Save/Bridge.
Canonical architecture: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`.

Purpose:
- stop scaling civilization by endlessly appending one-off era enums;
- provide a reusable framework able to carry primitive, historical, modern and future development.

Core model:
1. `Knowledge` — what residents/groups know.
2. `Capability` — what they can actually do with current facilities/resources/skills.
3. `Technology` — reproducible inventions/processes built from prerequisites.
4. `CivilizationTransformation` — measurable social/production consequences.

Scope v1:
- stable Capability/Technology identity and versioning;
- map existing primitive Techniques into the capability graph without breaking save ordinals;
- prerequisite/effect graph;
- problem/resource pressure as research/experiment motivation;
- experiment -> discovery -> reproducibility;
- physical/authoritative knowledge diffusion;
- adoption/diffusion state separate from discovery;
- technology/capability loss and rediscovery;
- Core read model for Observer;
- deterministic snapshot roundtrip.

Validation:
- no unlock without prerequisites;
- no output without required resources/energy/facility;
- same seed/state reproduces the same progression;
- different circumstances may branch into different development orders;
- knowledge/capability can decline;
- Save/Load does not duplicate discoveries.

---

## Milestone C4 — Health / Disease / Population Resilience

Owner: Jjun Core provider -> Dagyeom presentation.

Scope candidates:
- pathogens / infection;
- contaminated water/soil;
- sanitation-linked health;
- illness/recovery;
- environment/accident mortality;
- immunity/resilience where justified;
- health knowledge and care progression hooks;
- deterministic, testable effects.

Why here:
- multi-century civilization requires meaningful non-age mortality and population shocks;
- later medicine/biotech needs a real health substrate rather than decorative stats.

---

## Milestone C5 — Education / Recording / Specialization / Economy / Institutions

Owner: Jjun Core/Simulation/World provider -> Dagyeom Observer/Presentation.

**This scope is moved forward from the former final “economy/society” bucket.**
Industrial and future civilization cannot emerge naturally without coordination and knowledge scaling.

Scope:
- family/apprenticeship teaching expansion;
- persistent records / writing precursor when prerequisites exist;
- education/learning throughput;
- jobs/roles/specialization;
- production/storage demand signals;
- exchange/trade foundation;
- ownership/shared-resource policies at a generic simulation level;
- institutions/group coordination;
- technology adoption/acceptance effects;
- research/education organizations when capabilities permit.

Rules:
- no single real-world political/economic system is treated as the mandatory endpoint;
- systems emerge from generic resource/coordination/incentive mechanics.

---

## Milestone C6 — Migration / Multiple Settlements / Trade Networks

Owner: Jjun Core/World/Simulation.

Scope:
- carrying-capacity pressure;
- exploration;
- household/group migration;
- settlement founding/abandonment;
- multiple settlements;
- local knowledge/capability differences;
- trade routes and resource specialization;
- inter-settlement social continuity/conflict/cooperation foundations.

Depends on:
- trustworthy long-run scale;
- settlement economy;
- education/economy foundations.

---

# Historical-to-Future Content Expansion

These milestones add concrete content **on top of** the open-ended framework instead of creating a separate hardcoded era machine.

## Milestone F1 — Advanced Metallurgy / Urban / Scientific Accumulation

Examples:
- Bronze completion and improved tooling;
- Iron/high-temperature metallurgy;
- construction/material sophistication;
- urban infrastructure pressures;
- measurement/recording;
- structured experimentation and accumulated scientific knowledge.

The exact order is simulation-driven, not a fixed era checklist.

---

## Milestone F2 — Mechanical / Industrial Civilization

Examples:
- mechanical power transmission;
- precision tooling;
- pumps/engines;
- scalable manufacturing;
- transport infrastructure;
- industrial energy/resource externalities.

Technology must materially change production and society, not just add Observer labels.

---

## Milestone F3 — Electrical / Chemical / Modern Infrastructure

Examples:
- electricity generation/transmission/storage;
- advanced chemistry/materials;
- sanitation/medical infrastructure;
- mass production;
- motorized transport;
- high-throughput communication prerequisites.

---

## Milestone F4 — Digital / Network / Information Civilization

Examples:
- computation;
- electronics;
- data storage;
- communications networks;
- software/control abstractions represented as simulation capabilities;
- automation prerequisites.

---

## Milestone F5 — AI / Robotics / Advanced Automation

Examples:
- machine perception/control;
- autonomous robotics;
- automated production;
- AI-assisted research/education;
- labor/production structure transformation;
- adoption, regulation/acceptance and concentration/distribution effects as generic social mechanics.

No scripted “singularity date.”
Acceleration may emerge only when the feedback loop actually exists.

---

## Milestone F6 — Advanced Energy / Materials / Biotechnology

Examples:
- high-density energy;
- advanced materials;
- closed-loop production;
- genetics / regenerative medicine / artificial organs when health science supports them;
- longevity effects with real demographic/social consequences.

---

## Milestone F7 — Planetary / Space / Interplanetary Civilization

Examples:
- launch capability;
- orbital infrastructure;
- closed-loop life support;
- autonomous off-world industry;
- moon/planet/asteroid settlement when resources and transport permit;
- multi-settlement model generalized to off-world settlements.

---

## Milestone F8 — Open Future / Unknown Civilization

Purpose:
- allow credible capabilities beyond pre-authored modern/future milestones.

Direction:
- compositional innovation from existing Capability + Knowledge + resource/energy constraints;
- generated technology candidates must have deterministic stable identity and concrete effects;
- new technology must alter measurable simulation capability rather than exist as flavor text;
- allow multiple future trajectories such as distributed AI society, extreme automation, biological adaptation, advanced energy or other mechanically supported branches;
- allow acceleration, stagnation, collapse and rediscovery.

This is the point where LifeLens should be able to surprise the observer without abandoning causality.

---

# Previous Roadmap Reconciliation

| Previous item | New disposition | Reason |
|---|---|---|
| Dagyeom #100 closeout | KEEP, status corrected | live PR is no longer mergeable; refresh/current-main validation required |
| Character Context Motion v2 | KEEP HIGH | simulation actions need truthful visible embodiment |
| Emotion Runtime Integration | #132 CLOSEOUT | implemented; production external relief gap found/fixed before final merge |
| Observer Readability/Scrolling | KEEP HIGH | current detail overflow still needs real scrolling |
| Lifecycle Event Presentation | KEEP HIGH | multigeneration observation requires visible birth/growth/death history |
| Civilization Phase 2 | EXPANDED -> Settlement & Subsistence | housing/agriculture/renewable food are prerequisites for long civilization |
| Cleanup + long-run performance | MOVE UP | centuries/future simulation requires scale before content explosion |
| Health/Disease | KEEP, structured earlier | population shocks and future medicine need a real health substrate |
| Migration / Economy / Society | SPLIT | education/economy/institutions are needed before industrial/future progression; migration follows local foundation |
| Open-Ended Civilization Framework | NEW | prevents hardcoded era ceiling and future rework |
| Historical -> modern content bands | NEW | implemented on top of capability framework, not as forced era gates |
| AI/advanced energy/biotech/space | NEW | explicit long-range content path |
| Unknown future compositional innovation | NEW NORTH STAR | enables progression beyond present-day humanity |

---

## Gate B — Android Smoke / Real-device Baseline — PAUSED BY USER

Do not run Android builds until explicitly resumed.

When resumed:
- prefer cached/fast paths after verified engine cache;
- avoid blind repeat of long seed/full builds;
- validate APK boot, four founders, Observer controls/data, movement/actions, Save/Load, FPS/memory/thermal baseline.

MetaHuman comparison remains after this gate unless superseded by later presentation decisions.

---

## Current Parallel Dispatch

### Jjun Core/World lane
1. #132 exact-head closeout + merge.
2. Settlement & Subsistence Foundation.
3. Long-Run Scale & Cleanup.
4. Open-Ended Civilization Framework v1.
5. Health / Disease / Population Resilience.
6. Education / Recording / Specialization / Economy / Institutions.
7. Migration / Multiple Settlements / Trade Networks.
8. Historical -> industrial -> modern -> digital content.
9. AI / automation -> advanced energy / biotech -> space -> open future.

### Dagyeom Presentation lane
1. #100 current-main refresh / CI / Mac PIE closeout.
2. Character Context Motion v2.
3. Observer Readability + Real Scrolling.
4. Lifecycle Event Presentation v2.
5. presentation consumers for settlement, civilization capability/history and future systems as provider contracts land.

### Android/device lane
- Gate B remains **PAUSED** until explicit user resume.

Cross-lane request checkpoints from `tasks/TEAM_BOARD.md` remain mandatory at work start/end, long-build boundaries and before main-changing merges/rebases.
