# LifeLens Project Progress Snapshot — 2026-09-15

> 목적: 실제 GitHub 상태를 기준으로 현재 구현 완료 범위, 병렬 작업, 다음 실행 순서, 남은 구조적 과제를 한 문서에서 확인한다.
>
> 이 문서는 제품 설계를 대체하지 않는다. 제품 방향은 각 canonical 문서를 따르고, 실제 최신 상태는 GitHub `main` / PR / Actions가 최우선 진실이다.

## 1. 현재 기준점

Latest merged product slice:
- PR #85 `[CORE] Add World Genesis WG-2 macro world and viable start region`
- merge SHA: `ed4bf34d4b5bd0eb917a8bfb7fc5da16f52a4907`
- validated final head: `eac5a50b83416e9f55e160d1e6ad0d022b3f926a`

Validation:
- Structural Preflight `34935360828`: PASS, including WG-2 validator
- Core Tests `34935360834`: **47/47 PASS**
- deterministic harness smoke: PASS
- Unreal Linux Compile `34935360862`: PASS
- UE 5.6 image verification / UHT / UBT / link: PASS
- PR comments/reviews/unresolved threads at merge checkpoint: 0

Previous product slices:
- #83 World Genesis WG-1 — DONE.
- #82 HumanWaste Environmental Visual Feedback — DONE.
- #80 Dug sanitation pit progression — DONE.

Documentation commits may advance `main` beyond the product-code merge SHA without changing runtime behavior.

## 2. Canonical design map

- Master product spec: `docs/LIFELENS_SPEC_v1.1.md`
- Civilization progression: `docs/CIVILIZATION_PROGRESSION_v1.md`
- Open-ended invention: `docs/OPEN_ENDED_INVENTION_v1.md`
- World affordance / environmental consequence: `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`
- **World genesis / chunk / migration: `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`**
- World visual environment: `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`
- Environmental consequence visual feedback: `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`
- Character appearance direction: `docs/CHARACTER_APPEARANCE_ROADMAP.md`
- Character appearance data contract: `docs/CHARACTER_APPEARANCE_DATA_CONTRACT.md`
- Whole-project verification baseline: `docs/WHOLE_PROJECT_VERIFICATION_2026-09-14.md`
- Collaboration / state: `docs/STATE_MANAGEMENT.md`, `docs/INTEGRATION_SPRINT.md`, `tasks/WORK_STATE.md`, `tasks/TEAM_BOARD.md`

## 3. 구현 완료 축

### Human / society Core

구축됨:
- Needs
- Personality
- Emotion
- Memory
- Belief
- 13차원 Relationship
- social cognition / witness / rumor
- autonomous social decisions
- Observer read models

### Family / generation

구축됨:
- Romance
- Engagement / Marriage
- Household
- Pregnancy
- Birth
- Genetics baseline
- Child growth / life stage
- Aging
- Death
- Life history
- Genealogy / generation continuity

### Civilization baseline

구축됨:
- natural resource nodes
- finite resource quantity / regeneration hooks
- personal inventory
- gather / store
- experiment / discovery
- crafting baseline
- personal knowledge
- witness / imitation / teaching transmission
- civilization observer DTOs
- Save/Load persistence

경계:
- 완전한 arbitrary artifact physics/runtime은 아직 아니다.
- `OPEN_ENDED_INVENTION_v1.md`의 generic material/component/connection artifact engine은 후속 구현이다.
- fixed global Tech Tree로 회귀하지 않는다.

### Core ↔ Unreal physical execution

#70~#74 및 후속 sanitation integration으로 구축됨:
- dynamic resident reconciliation
- dynamic ActivityAnchor refresh
- all LifeLens C++ paths UE compile gate
- external physical execution ACK contract
- Core physical outcome waits for World arrival/use
- Core use duration projected to World
- actual completion position written back to Core
- environmental consequence uses same acknowledged position
- Core Grid ↔ World baseline 100 uu/tile
- runtime position restore after Save/Load

## 4. Missing-infrastructure / environment causal loop

### #66 — World Affordance Fallback DONE
- `Preferred → Primitive → Natural → Emergency → Unavailable`
- missing modern facility remains missing.

### #68 — Environmental Residue DONE
- Core-owned HumanWaste residue.
- amount/intensity/radius/time.
- accumulation/decay/exposure.
- Save/Load and Unreal read path.

### #75 — Exposure / Memory / Avoidance DONE
- contamination perception.
- hygiene/emotional burden.
- location-specific sanitation Memory.
- duplicate-memory suppression.
- current + remembered contamination avoidance.
- deterministic low-exposure outdoor relief recommendation.

### #76 — World consumes Core sanitation recommendation DONE
- visible target and authoritative ACK/residue location agree.

### #77 — Sanitation Problem Recognition DONE
- repeated/salient direct evidence can become durable sanitation-problem Belief.
- no global unlock.

### #78 — DesignatedSanitationArea discovery DONE
- recognized problem + clean candidate site gates experiment.
- failure → Hypothesized, success → personal Reproducible knowledge.
- witness/teaching provenance and persistence.

### #79 — authoritative designated sanitation site DONE
- actual Craft required to materialize a persistent Core-owned site.
- stable site id / exact GridPos / active / useCount.
- exact site identity and position carried through World movement and ACK.
- snapshot binary v5 persists site state.

### #80 — Dug sanitation pit progression DONE
- personal `DugSanitationPit` discovery.
- actual designated site + useful improvement context required.
- knowledge does not instantly mutate facility.
- repeated Craft excavation work accumulates.
- same site id/GridPos upgrades `DesignatedArea → DugPit`.
- no fabricated shovel; current manual digging is slower, future Dig tool can accelerate same work contract.
- existing waste amount retained while containment reduces intensity/radius.
- future DugPit use still produces HumanWaste with lower exposure profile.
- sanitation extension v2 persists kind/progress/improver/minute while outer snapshot stays v5 and v1 sanitation data remains readable.
- no `LatrineUnlocked`, modern plumbing or magic waste deletion.

### #82 — HumanWaste Environmental Visual Feedback DONE
- authoritative HumanWaste residue now appears in Unreal world presentation.
- single HISM batches many residues; no unlimited heavy Actor/Niagara per residue.
- authoritative Core Grid position maps to world location with surface trace for visible Z placement.
- amount/intensity/radius/age drive footprint and per-instance presentation data.
- DesignatedArea residue (`0.42`, radius `3`) reads broader/stronger than DugPit residue (`0.16`, radius `1`).
- presentation consumes `GetEnvironmentObservation(...)` only and never owns residue/sanitation truth.
- Save/Load rebuilds visuals from Core-authoritative state.
- Android baseline uses cap + culling + signature-based refresh suppression.

The causal chain now reaches:

`Need → fallback behavior → residue → exposure/Memory → problem recognition → experiment/knowledge → actual persistent site → repeated physical improvement → measurable containment`

## 5. World Genesis / map scalability — WG-1 + WG-2 IMPLEMENTED / MILESTONE A NEXT

Canonical: `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`.

Implementation checkpoint:
- **WG-1 DONE via PR #83**, merge `f5c8cbab3aa41c6a37c3bae06838eeb583749771`.
- runtime has separate WorldSeed/PopulationSeed/GenerationVersion identity, stable ChunkCoord conversion and order-independent untouched chunk baseline seeds.
- **WG-2 DONE via PR #85**, merge `ed4bf34d4b5bd0eb917a8bfb7fc5da16f52a4907`.
- runtime now derives coherent macro elevation/moisture/temperature/biome/resource potential and deterministically selects a viable-but-unsolved start region.
- next is **World Generation Milestone A**: detailed natural chunks + selected start-region materialization + initial spawn integration + minimum no-reroll persistence boundary.

Major decision:
- the current gray bootstrap plane is **not** the final map architecture.
- LifeLens must not trap multi-generation population growth inside one permanently small fixed arena.

Canonical world chain:

`WorldSeed → Macro World → deterministic lazy chunks → persistent human/environmental history → carrying-capacity pressure → migration → additional settlements → regional society/civilization`

### Seed split

- `WorldSeed`
  - terrain/water/biome/resource natural baseline
  - macro world
  - order-independent chunk baseline

- `PopulationSeed`
  - initial 2 male + 2 female resident names/traits/genetics/appearance variation

This allows:
- ordinary NEW GAME: fresh world + fresh people
- same-world replay: fixed WorldSeed + new PopulationSeed
- exact debug replay: both fixed

### Initial world rule

NEW GAME starts with:

`natural environment + 2 male residents + 2 female residents + zero civilization infrastructure`

No house, road, toilet, farm, storage, workshop or plumbing may be silently created as simulation truth.

### Chunk rule

Untouched detailed chunk state must be deterministic from:

`Hash(WorldSeed, ChunkCoord, GenerationVersion)`

Exploration order must not change future geography.

Generated/modified chunks never silently reroll after unload/load.

Persistent history can include:
- resource depletion/regrowth
- vegetation changes
- HumanWaste/residues
- sanitation sites / DugPit
- structures/storage/work sites
- excavation
- paths/foot traffic
- cultivation
- abandonment/regrowth

### Population / land pressure

Population growth does not magically resize terrain.
Instead it changes dynamic carrying pressure based on:
- food/water
- production/storage
- shelter
- sanitation/contamination
- available land
- resource depletion
- congestion
- household crowding
- social conflict/safety
- knowledge/technology

Pressure/opportunity can lead to exploration, household/group migration, camps and additional settlements.

Do **not** hard-code rules such as `population 30 => settlement B`.

### Android scaling

Logical world size does not imply full 3D rendering everywhere.

Presentation tiers:
- observed/near: full actors and detailed environment.
- nearby: reduced LOD/update cost.
- distant: Core logical simulation without all full 3D actors.
- untouched: macro facts only until detailed generation is required.

World Partition/streaming/PCG/HISM/HLOD are possible Unreal implementation tools, not simulation authorities.

### Production environment gate

Small test maps may continue for Core/Bridge/animation verification.

Before `World Visual Environment v1` becomes a permanent production-sized map:
- WG-1 deterministic world coordinates/chunk keys are **established via #83**.
- WG-2 macro world/start-site selection boundaries are **established via #85**.
- production World Visual must consume WG-1/WG-2 and the materialization boundary from World Generation Milestone A.

This prevents later rewriting a small hand-authored arena into a scalable world.

## 6. Character / Presentation state

### Character Presentation
- PR #63 DONE.

### Deterministic Appearance contract
- PR #65 DONE.

### Character Appearance v1 — PR #67 DONE

Owner: 다겸 / 다겸 AI

Final head / merge:
- final head: `982d930a53f199b33ebf7ca3d4f5b72f72f8a91b`
- main squash merge: `915906357d9752a5654b3dfeb85795419d885b59`

Delivered:
- Quaternius CC0 humanoids.
- deterministic #65 appearance mapping.
- hair / skin / body variation.
- UAL animation assets.
- Peasant outfit + head-only derivative to avoid clothing penetration.
- appearance construction after resident identity binding.
- same-resident appearance continuity across restart/load.
- tracked `__pycache__/*.pyc` removed and ignore rules added.
- duplicate includes removed.
- male Peasant exposed skin now matches deterministic body/head light/dark BaseColor + tint.

Final validation:
- Preflight `34929703738`: PASS.
- Unreal Linux Compile `34929703712`: PASS including UHT / UBT / link.
- helper integration Core Tests `34929611584`: 45/45 PASS + deterministic harness.

Known presentation limitation now promoted to next work:
- locomotion is not wired yet; Idle-looking slide is Motion Bootstrap scope.

## 7. Immediate next execution

### Jjun lane — READY_NOW

**World Generation Milestone A**

Goal:
- deliver the first detailed, materializable natural-world slice as one meaningful milestone instead of separate micro-PRs.
- include deterministic detailed chunks, selected start-region materialization, founder spawn integration and the minimum persistence/no-reroll boundary.
- preserve zero starting civilization infrastructure and Core world authority.

### Dagyeom lane — ACTIVE

1. **Motion Bootstrap PR #84 — CI PASS; PIE visual confirmation / final review / merge pending.**
2. World Genesis WG-1/WG-2 integration must be respected before production-sized World Visual map commitment.
3. World Visual Environment v1.
4. remaining Motion & Context.
5. Observer UX polish / mobile touch.

Motion is presentation-only: Core/World remains movement/action authority.

## 8. Environmental visual feedback

Canonical: `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`.

Rules:
- graphics are not authority.
- spatially meaningful environmental consequence gets a visual representation path.
- amount/intensity/radius/decay drives visual strength/range/lifetime.
- Save/Load reconstructs visuals from authoritative state.
- Android must avoid unlimited 1:1 heavy Actors.

Pending visuals:
- **HumanWaste baseline HISM ground feedback — DONE (#82); presentation material/mesh polish remains later.**
- resource depletion/regrowth
- fire/smoke/scorch
- foot-traffic path formation
- construction/damage stages

## 9. Open-ended invention

Canonical: `docs/OPEN_ENDED_INVENTION_v1.md`.

Direction:
- novel non-historical artifacts allowed where physically coherent.
- developer need not pre-author every exact final artifact form.
- material + component + connection + geometry/use result determine usefulness.
- failed experiments create learning evidence.
- success spreads personal experience → technique → imitation/teaching → culture.
- no global auto-unlock.
- no magic physical impossibilities.

Status: **design DONE / fully generic artifact runtime NOT YET IMPLEMENTED**.

## 10. Remaining structural risks

### A. Generic facility/resource authority — PARTIAL
Sanitation now has a strong one-id/one-position path, but every facility/resource does not yet use one shared generic target registry.

### B. World Genesis runtime — PARTIALLY IMPLEMENTED
WG-1 is DONE via #83 and WG-2 is DONE via #85. The next grouped delivery is World Generation Milestone A, combining detailed natural chunks, start-region materialization, initial spawn integration and a minimum persistence/no-reroll boundary.

### C. Birth physical position — NEEDS FIX
New child Core runtime position may still default to `{0,0}`.

### D. Dormant `ULLDecisionComponent` — REVIEW / REMOVE
Blueprint-callable competing chooser remains.

### E. Snapshot legacy fixtures — NEEDS TEST
Migration code exists; representative old binary fixture tests remain incomplete.

### F. Environment → health/pathogen — LATER
Current sanitation benefits are exposure/hygiene/behavior containment, not full disease simulation.

### G. Death presentation policy — UNDECIDED
Core death exists; Unreal actor/body/observer policy is separate.

### H. Android product validation — NOT DONE
Still required:
- cheap smoke package path
- Android Cook / Package
- APK artifact
- real device run
- performance / thermal / memory profiling

Do not revive the old multi-hour PR #2 path as normal iteration.

## 11. Current product sequence

1. Character Presentation #63 — DONE.
2. Appearance contract #65 — DONE.
3. World Affordance Fallback #66 — DONE.
4. Environmental Residue #68 — DONE.
5. Verification #69 — DONE WITH FINDINGS.
6. runtime resident/dynamic affordance #70 — DONE.
7. compile trigger coverage #71 — DONE.
8. external Core ACK #72 — DONE.
9. World ACK integration #73 — DONE.
10. runtime position restore #74 — DONE.
11. exposure/perception/avoidance #75 — DONE.
12. World sanitation recommendation #76 — DONE.
13. sanitation problem recognition #77 — DONE.
14. sanitation experiment/discovery #78 — DONE.
15. authoritative designated site #79 — DONE.
16. Dug sanitation pit progression #80 — DONE.
17. HumanWaste visual feedback #82 — DONE.
18. Character Appearance #67 — DONE.
19. **Motion Bootstrap — ACTIVE #84; CI PASS, PIE/review/merge pending** (Dagyeom lane).
20. **World Genesis WG-1 — DONE #83; WG-2 — DONE #85; World Generation Milestone A — READY_NOW.**
21. World Visual Environment production map.
22. Character Motion & Context remainder.
23. integrated runtime verification.
24. Android smoke APK + device profiling.
25. MetaHuman comparison after mobile baseline only.
26. deeper production/health/open-ended invention/migration/multi-settlement systems.

## 12. Key interpretation rule

다음 표현을 혼동하지 않는다:
- Compile PASS ≠ feature DONE.
- design document exists ≠ runtime implemented.
- environmental state exists ≠ every environmental consequence already has polished visual presentation; HumanWaste now has the #82 baseline path.
- World Genesis WG-1 is implemented, but Macro World / detailed lazy chunks / persistent chunk history are not yet running.
- #67 is DONE; Motion Bootstrap needs its own branch/PR/validation before it is DONE.
- Quaternius baseline ≠ permanent final visual ceiling.

Actual current state is determined by GitHub + `WORK_STATE.md` + `TEAM_BOARD.md`.
