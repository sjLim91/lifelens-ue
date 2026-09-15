# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

> **LIVE LOCK STATUS (main): active assist locks = 0.**
> 실제 `main` / branch / PR / Actions가 문서보다 우선한다.
> 전체 상태 기준: `tasks/WORK_STATE.md`.

## 최우선 규칙

- 기능 작업 전 `WORK_STATE.md` → 이 보드 → `HANDOFF_LOG.md`를 맞춘다.
- ACTIVE_LOCK 파일은 lock owner 외 수정 금지.
- 실제 즉시 착수 가능 상태는 `READY_NOW` 또는 `PARALLEL_SAFE_NOW`뿐이다.
- `NEXT` / `AFTER` / `HIGH PRIORITY` / `READY_AFTER_*`는 즉시 착수 허가가 아니다.
- 쭌이 다겸 작업을 도울 때는 `docs/INTEGRATION_SPRINT.md`를 따른다.
- 기본 지원은 REVIEW_ONLY이며 `dagyeom/*` direct push 금지.
- 검증 전용 PR은 제품 코드로 병합하지 않는다.
- compile PASS만으로 DONE 처리하지 않는다. merge + state sync까지 완료되어야 DONE이다.

## Canonical product references

- Master: `docs/LIFELENS_SPEC_v1.1.md`
- Civilization: `docs/CIVILIZATION_PROGRESSION_v1.md`
- Open-ended invention: `docs/OPEN_ENDED_INVENTION_v1.md`
- World affordance/environment: `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`
- **World genesis/chunks/migration: `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`**
- World visual presentation: `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`
- Environmental visual feedback: `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`
- Character appearance: `docs/CHARACTER_APPEARANCE_ROADMAP.md`

## Active / ready work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | new branch after preflight | HumanWaste Environmental Visual Feedback | Core read contract + World presentation support | **READY_NOW** |
| 다겸 + 다겸 AI | PR #67 `dagyeom/character-appearance-v1` | Character Appearance v1 closeout | Character appearance + `Content/Characters/**` | **ACTIVE / CLOSEOUT** |
| 다겸 + 다겸 AI | after #67 | Character Motion Bootstrap | Character locomotion presentation | READY_AFTER_#67 |
| 쭌 + 다겸 lanes | before production World Visual | World Genesis WG-1/WG-2 integration gate | Core world coordinates + environment architecture | DESIGN FIXED / REQUIRED GATE |
| 다겸 + 다겸 AI | after Motion + world-genesis gate | World Visual Environment v1 | Environment/Maps/WorldPresentation | HIGH PRIORITY |
| 다겸 + 쭌 Bridge support as needed | after World Visual v1 | Character Motion & Context remainder | Character presentation/animation | AFTER WORLD VISUAL v1 |
| 쭌 + 쭌 AI | PR #2 | old Android validation path | Bridge/build | FROZEN |

## Current Assist Locks

**0개.** Previous appearance/presentation assist locks are released.

## Latest Jjun product checkpoint — PR #80 DONE

PR #80 `[CORE] Add dug sanitation pit progression v1`
- merge SHA: `291926cf78d12c1c61284eb9c59a50e7c70e54e7`
- validated head: `fc918693bcd265f3c021f0bd826f6ba5a7113678`

Validation:
- Structural Preflight `34926841905`: PASS.
- Core Tests `34926841895`: **PASS, 45/45**.
- deterministic harness smoke: PASS.
- Unreal Linux Compile `34926841907`: PASS.
- actual UE 5.6 image verification / UHT / UBT / link: PASS.

Delivered:
- personal `DugSanitationPit` knowledge and `DigSanitationPit` experiment.
- no site / no meaningful sanitation-improvement context → no pit progression.
- knowledge discovery alone does not mutate facility kind.
- repeated Craft work accumulates excavation progress.
- existing `PrimitiveSanitationSite` keeps the exact same site id and Core GridPos and upgrades `DesignatedArea → DugPit`.
- current no-Dig-tool world supports slower manual excavation without inventing a shovel.
- future Dig-capable tools can accelerate the same work contract.
- pit completion contains existing HumanWaste by lowering exposure intensity/radius without deleting waste amount.
- DugPit use still creates HumanWaste with reduced exposure profile.
- exact Core site id + GridPos remains required through World movement and ACK.
- outer snapshot remains v5; sanitation sub-extension v2 persists kind/progress/improver/minute and accepts v1 data.
- Unreal civilization read DTO includes `DesignatedSanitationArea` and `DugSanitationPit`.
- no global `LatrineUnlocked` or automatic modern plumbing.

## Jjun next lane — HumanWaste Environmental Visual Feedback — READY_NOW

Goal:
- make the authoritative sanitation consequence visually observable.
- keep Core/environment state authoritative.
- visual state follows create/update/decay/removal/SaveLoad restore.
- open designated-area contamination and DugPit-contained contamination must be distinguishable without misleading the player into thinking waste disappeared.

Expected boundary:
- inspect `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md` against #80.
- expose only the minimum read facts missing for visual reconstruction.
- use pooled/instanced/cullable Android-safe presentation.
- no UI/World code may invent residues or mutate sanitation state.

Acceptance:
- visible location agrees with authoritative Core/Grid position.
- visual lifetime agrees with authoritative residue lifetime.
- lower DugPit exposure has a coherent visual cue.
- Save/Load rebuilds visuals from Core state.
- relevant Preflight/Core/UE compile checks pass.

## World Genesis / Chunk / Migration — CANONICAL DESIGN FIXED

Canonical: `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`.

Decision:
- the current flat bootstrap plane is **not** the final map architecture.
- LifeLens uses a large logical world with lazy detailed chunk generation.
- canonical chain:
  `WorldSeed → Macro World → deterministic lazy chunks → persistent history → carrying-capacity pressure → migration → multiple settlements`.

Seed split:
- `WorldSeed`: natural world baseline.
- `PopulationSeed`: initial names/traits/genetics/appearance randomization.
- ordinary NEW GAME can create both fresh.
- future replay can pin WorldSeed while regenerating PopulationSeed.

World rules:
- same `WorldSeed + GenerationVersion + ChunkCoord` must create the same untouched chunk independent of exploration order.
- initial state is natural environment + 2 male + 2 female residents + **zero civilization infrastructure**.
- unvisited regions need not render or exist as full Actors.
- generated/modified regions never silently reroll after unload/reload.
- human activity accumulates as persistent history: residue, pits, paths, resource depletion, construction, cultivation, abandonment/regrowth.
- carrying capacity is dynamic; population pressure should lead to intensification/exploration/migration rather than permanent crowding in one fixed arena.
- distant residents/settlements can remain logically alive without every 3D Actor staying active.

Implementation gate:
- bootstrap/test maps remain valid for current feature verification.
- before `World Visual Environment v1` becomes a permanent production-sized map, WG-1/WG-2 must be implemented or explicitly integrated:
  - WG-1 deterministic world coordinates/chunk keys.
  - WG-2 macro world + viable start-site selector.
- do not hard-lock Environment/Maps work to a small hand-authored arena.

## Character Appearance v1 — ACTIVE / PR #67 CLOSEOUT

Owner: 다겸 / 다겸 AI
PR: #67 `dagyeom/character-appearance-v1`
Latest checked head: `e034fe785ca46dd5cb39fd7d7e8710d677994a38`

Latest known CI on that head:
- Preflight `34919060096`: PASS.
- Unreal Linux Compile `34919060115`: PASS.

Jjun review remains `CHANGES_REQUESTED` for three closeout items:
1. remove committed `__pycache__/*.pyc` + add ignore rules.
2. remove duplicate includes from `LLResidentCharacter.cpp`.
3. verify/fix bright-skin male Peasant exposed arm/hand skin consistency with face/neck.

Reported/implemented:
- Quaternius CC0 humanoids.
- deterministic #65 appearance projection.
- hair / skin / body variation baseline.
- UAL animation assets.
- Peasant outfit integration.
- head-only body derivative to prevent outfit penetration.
- identity binding fixed before appearance construction.
- local PIE and Save/Load appearance continuity reported PASS.

Remaining before DONE:
- push closeout fixes on a new remote HEAD.
- final CI / review / merge / docs sync.

Known limitation:
- Idle-looking movement slide remains until Motion Bootstrap.

## Core ↔ World repair status

Resolved baseline:
- #70 dynamic resident reconciliation.
- #71 UE compile trigger coverage.
- #72 external physical execution pending-ACK Core contract.
- #73 World movement/use → completion ACK.
- #74 runtime GridPos restore.
- #75 environmental exposure/memory/avoidance.
- #76 Core sanitation target consumed by World.
- #77 resident sanitation problem recognition.
- #78 sanitation experiment/personal knowledge.
- #79 persistent authoritative designated sanitation site.
- #80 same-site dug-pit improvement and containment.

Still partial:
- generic facility/resource target authority beyond sanitation.
- facility quality/backing-resource contract.
- target disappearance/path failure/re-resolve semantics.
- child initial Core GridPos at birth.
- dormant `ULLDecisionComponent` competing chooser removal/restriction.
- explicit legacy snapshot fixture regression coverage.

## World / environment canonical loop

`Need`
→ `Intent`
→ `current-world affordance`
→ `movement/arrival`
→ `action result`
→ `environment consequence`
→ `exposure/Memory`
→ `changed behavior`
→ `problem recognition`
→ `experiment/discovery`
→ `better affordance`
→ `culture/civilization`

Sanitation chain now:
- #66 fallback semantics — DONE.
- #68 HumanWaste residue — DONE.
- #75 exposure/memory/avoidance — DONE.
- #76 same visible target/ACK — DONE.
- #77 problem recognition — DONE.
- #78 designated-area discovery — DONE.
- #79 authoritative designated site — DONE.
- #80 DugPit improvement/containment — DONE.
- **HumanWaste visual feedback — READY_NOW.**
- material-backed latrine superstructure and health/pathogen simulation later.

## Shared File Ownership

Jjun default:
- `Source/LifeLensCore/**`
- `Source/LifeLens/AI/**`
- `Source/LifeLens/Simulation/**`
- `Source/LifeLens/World/**`
- Save/Load, Bridge, CI/build

Dagyeom default:
- `Source/LifeLens/UI/**`
- Character appearance/presentation
- `Content/UI/**`
- `Content/Characters/**`
- `Content/Environment/**`
- `Content/Maps/**`
- `Content/WorldPresentation/**`

World Genesis integration crosses Core/World and future Environment content. Coordinate/authority contracts remain Jjun-owned; Dagyeom owns visual environment content/presentation unless explicitly handed off.

## Dagyeom API / design handoff

Relevant read contracts include:
- `GetWorldObservation()`
- `GetResidentObservations()`
- `GetResidentObservation(...)`
- `GetFamilyObservation(...)`
- `GetResidentActionDirective(...)`
- `GetRecentCoreEvents()`
- `OnCoreRuntimeStateChanged`
- `GetResidentCivilizationObservation(...)`
- `GetCivilizationWorldObservation(...)`
- `GetEnvironmentObservation(...)`
- `GetResidentRuntimeGridPosition(...)`
- `GetRecommendedOutdoorReliefGridPosition(...)`
- `GetSanitationUseTarget(...)`
- `ULLAppearanceProfileLibrary::MakeDeterministicAppearanceProfile(...)`

Rules:
- never hard-code runtime population to 4.
- UI/Presentation rebuilds from Bridge/Core state after load.
- Character/UI presentation does not choose competing actions.
- if authoritative data is missing, add an Integration Request rather than duplicating state.
- future environment presentation must not assume the bootstrap plane is the permanent world boundary.

## Merge / execution queue

1. #63 Character Presentation — DONE.
2. #65 Appearance projection contract — DONE.
3. #66 World Affordance Fallback — DONE.
4. #68 Environmental Residue — DONE.
5. #69 Whole-project verification — DONE WITH FINDINGS.
6. #70 runtime residents/dynamic affordances — DONE.
7. #71 UE compile trigger coverage — DONE.
8. #72 external execution Core ACK — DONE.
9. #73 World ACK integration — DONE.
10. #74 runtime position restore — DONE.
11. #75 environmental exposure/perception/avoidance — DONE.
12. #76 World sanitation recommendation integration — DONE.
13. #77 Sanitation Problem Recognition — DONE.
14. #78 Primitive sanitation experimentation progression — DONE.
15. #79 Designated sanitation area authoritative affordance — DONE.
16. #80 Dug sanitation pit progression — DONE.
17. **HumanWaste visual feedback — READY_NOW.**
18. Character Appearance #67 — ACTIVE CLOSEOUT in parallel.
19. Character Motion Bootstrap — after #67.
20. **World Genesis WG-1/WG-2 implementation gate.**
21. World Visual Environment v1 production environment.
22. Character Motion & Context remainder.
23. Observer UX / Mobile Touch / visual feedback UI work.
24. integrated runtime verification.
25. Android smoke APK + device profiling.
26. MetaHuman comparison only after Android/mobile baseline.
27. deeper open-ended invention / health / migration / multi-settlement society.

## Completion rule

검증 + merge + 상태 동기화까지 완료되어야 DONE이다.
