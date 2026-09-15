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
| 쭌 + 쭌 AI | new milestone branch after state sync | World Generation Milestone A | detailed natural chunks + start-region materialization + spawn/persistence boundary | **READY_NOW** |
| 다겸 + 다겸 AI (Claude) | `dagyeom/character-motion-v1` / #84 | Character Motion Bootstrap | Character locomotion presentation | **ACTIVE — CI PASS / PIE+review pending** |
| 쭌 + 다겸 lanes | before production World Visual | World generation integration gate | WG-1/WG-2 authority + materialized chunk boundary | WG-1/WG-2 DONE / MILESTONE A NEXT |
| 다겸 + 다겸 AI | after Motion + world-genesis gate | World Visual Environment v1 | Environment/Maps/WorldPresentation | HIGH PRIORITY |
| 다겸 + 쭌 Bridge support as needed | after World Visual v1 | Character Motion & Context remainder | Character presentation/animation | AFTER WORLD VISUAL v1 |
| 쭌 + 쭌 AI | PR #2 | old Android validation path | Bridge/build | FROZEN |

## Current Assist Locks

**0개.** Previous appearance/presentation assist locks are released.

## Latest Jjun product checkpoint — PR #85 DONE

PR #85 `[CORE] Add World Genesis WG-2 macro world and viable start region`
- merge SHA: `ed4bf34d4b5bd0eb917a8bfb7fc5da16f52a4907`
- validated head: `eac5a50b83416e9f55e160d1e6ad0d022b3f926a`

Validation:
- Structural Preflight `34935360828`: PASS, including WG-2 validator.
- Core Tests `34935360834`: PASS, **47/47**.
- deterministic harness smoke: PASS.
- Unreal Linux Compile `34935360862`: PASS.
- UE 5.6 image verification / UHT / UBT / link: PASS.
- merge checkpoint PR comments/reviews/unresolved threads: 0.

Delivered:
- coherent deterministic macro terrain/climate fields and biome classification.
- broad water/fertility/natural-resource/traversal/hazard potential.
- PopulationSeed-independent geography and start-region ranking.
- deterministic viable start-region selection over 625 candidates.
- no starting civilization infrastructure and no forced relocation outside the current bootstrap surface before chunk materialization exists.

## Jjun next lane — World Generation Milestone A — READY_NOW

Purpose:
- reduce PR/CI/document churn by grouping the next tightly coupled world-generation work into one milestone-sized delivery.

Scope:
- WG-3 deterministic detailed natural chunk baseline.
- macro/biome-driven local nature/resource facts.
- generated-chunk registry / no-reroll identity.
- WG-2 selected start-region materialization boundary.
- initial founder spawn integration into that region.
- minimum persistence boundary required for unload/load and Save/Load continuity.
- minimal Bridge read path required by later World Visual presentation.

Boundary:
- still no house/toilet/farm/storage/road/tool auto-spawn.
- Unreal streaming/PCG remains presentation/implementation, not simulation authority.
- one heavy UE compile gate at milestone close unless an earlier interface change specifically requires it.

## World Genesis / Chunk / Migration — WG-1 DONE / WG-2 DONE / MILESTONE A READY_NOW

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
- WG-1 deterministic world coordinates/chunk keys are **DONE #83**.
- WG-2 macro world + viable start-site selection is **DONE #85**.
- production World Visual must consume these contracts; World Generation Milestone A materializes the first detailed natural/start-region slice.
- do not hard-lock Environment/Maps work to a small hand-authored arena.

## Latest Dagyeom product checkpoint — PR #67 DONE

PR #67 `[UI] Character Appearance v1 — Quaternius CC0 human body, deterministic look (Track B)`
- final head: `982d930a53f199b33ebf7ca3d4f5b72f72f8a91b`
- main squash merge: `915906357d9752a5654b3dfeb85795419d885b59`
- helper closeout: PR #81 merged into `dagyeom/character-appearance-v1`

Final closeout delivered:
- committed Python cache artifact removed; `.gitignore` now covers `__pycache__/` and `*.pyc`.
- duplicate character includes removed.
- bright-skin male Peasant exposed arms/hands use the same deterministic light/dark skin BaseColor choice and tint as the resident body/head.
- latest main was integrated without moving simulation authority into Character presentation.

Validation:
- #67 final-head Preflight `34929703738`: PASS.
- #67 final-head Unreal Linux Compile `34929703712`: PASS including UE 5.6 image verify / UHT / UBT / link.
- helper integration Core Tests `34929611584`: PASS, **45/45** + deterministic harness smoke.
- previous blocking review dismissed after verification; final review approved.

Next Dagyeom lane:
- **Character Motion Bootstrap — READY_NOW.**
- scope: Idle / Walk / Jog + velocity-driven switching + orientation smoothing.
- Core/World continues to own movement/action authority; animation only reflects runtime movement state.

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
- #82 authoritative HumanWaste residue → Android-safe Unreal visual projection.
- #83 deterministic World Genesis WG-1 coordinate/seed/runtime contract.

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
- **HumanWaste visual feedback — DONE #82.**
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
17. HumanWaste visual feedback #82 — DONE.
18. Character Appearance #67 — DONE.
19. **Character Motion Bootstrap — ACTIVE #84; CI PASS, PIE/review/merge pending** (Dagyeom lane).
20. **World Genesis WG-1 — DONE #83; WG-2 — DONE #85; World Generation Milestone A — READY_NOW.**
21. World Visual Environment v1 production environment.
22. Character Motion & Context remainder.
23. Observer UX / Mobile Touch / visual feedback UI work.
24. integrated runtime verification.
25. Android smoke APK + device profiling.
26. MetaHuman comparison only after Android/mobile baseline.
27. deeper open-ended invention / health / migration / multi-settlement society.

## Completion rule

검증 + merge + 상태 동기화까지 완료되어야 DONE이다.
