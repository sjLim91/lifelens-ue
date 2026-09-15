# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

> **LIVE LOCK STATUS (main): active assist locks = 0.**
> 실제 `main` / branch / PR / Actions가 문서보다 우선한다.
> 전체 진행 요약: `docs/PROJECT_PROGRESS_2026-09-15.md`.

## 최우선 규칙

- 기능 작업 전 `WORK_STATE.md` → 이 보드 → `HANDOFF_LOG.md`를 맞춘다.
- ACTIVE_LOCK 파일은 lock owner 외 수정 금지.
- 실제 즉시 착수 가능 상태는 `READY_NOW` 또는 `PARALLEL_SAFE_NOW`뿐이다.
- `NEXT` / `AFTER` / `HIGH PRIORITY` / `READY_AFTER_*`는 즉시 착수 허가가 아니다.
- 쭌이 다겸 작업을 도울 때는 `docs/INTEGRATION_SPRINT.md`를 따른다.
- 기본 지원은 REVIEW_ONLY이며 `dagyeom/*` direct push 금지.
- 검증 전용 PR은 제품 코드로 병합하지 않는다.
- compile PASS만으로 DONE 처리하지 않는다. merge + state sync까지 완료되어야 DONE이다.

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/primitive-latrine-progression-v1` | Dug pit / primitive latrine progression | Core/Civilization/World contract | **IN_PROGRESS** |
| 다겸 + 다겸 AI | PR #67 `dagyeom/character-appearance-v1` | Character Appearance v1 | Character appearance + `Content/Characters/**` | **ACTIVE / CLOSEOUT** |
| 다겸 + 다겸 AI | after #67 | Character Motion Bootstrap | Character locomotion presentation | READY_AFTER_#67 |
| 다겸 + 다겸 AI | after Motion Bootstrap | World Visual Environment v1 | Environment/Maps/WorldPresentation | HIGH PRIORITY / READY_AFTER_MOTION_BOOTSTRAP |
| 다겸 + 쭌 Bridge support as needed | after World Visual v1 | Character Motion & Context remainder | Character presentation/animation | AFTER WORLD VISUAL v1 |
| 쭌 + 다겸 lanes | after primitive sanitation improvement | HumanWaste visual feedback | Core read contract + Presentation | NEXT |
| 쭌 + 쭌 AI | PR #2 | old Android validation path | Bridge/build | FROZEN |

## Current Assist Locks

**0개.** Previous appearance/presentation assist locks are released.

## Current Jjun lane — Dug pit / primitive latrine progression — IN_PROGRESS

Branch:
- `jjun/primitive-latrine-progression-v1`

Dependency:
- PR #79 DONE.

Goal:
- improve the authoritative designated sanitation area into a physically meaningful primitive sanitation facility without jumping directly to modern plumbing.
- preserve the causal progression from recognized problem and discovered knowledge into actual work/material-backed infrastructure.
- keep facility identity/location/state Core-owned while World executes movement/use and reports completion facts.

Planned minimal boundary:
- inspect #79 `PrimitiveSanitationSite`, Civilization recipe/tool/material representation, environmental residue deposition and snapshot extension first.
- prefer a dug-pit step before a more developed latrine unless current material/tool contracts make a different primitive step more coherent.
- require actual knowledge/resources/tools/work rather than a global unlock or silent upgrade.
- decide explicitly whether improvement upgrades the existing site or creates a successor identity; never leave two competing authorities for the same facility.
- encode a measurable sanitation benefit such as tighter containment/lower exposure while preserving realistic waste residue.
- persist/restore the improvement and keep stale/invalid target handling fail-closed.

Acceptance:
- no prerequisite knowledge/material/work → no pit/latrine appears.
- the #79 designated area remains usable until a real improvement completes.
- improved facility keeps one authoritative Core identity/GridPos for preference, World movement and completion ACK.
- physical use yields a measurable sanitation improvement without magical waste deletion.
- Save/Load continuity.
- Core tests + Structural Preflight + Unreal compile for any Bridge/World contract changes.

## Latest Jjun product checkpoint — PR #79 DONE

PR #79 `[CORE/WORLD] Add authoritative designated sanitation area affordance v1` merged as:
- merge SHA: `90f2b4f9cdc480e99886632e09323b2feab9625c`
- validated head: `49c76863b1957df09b7d814f7117826e92707021`

Validation:
- Structural Preflight run `34923460056`: PASS.
- Core Tests run `34923459974`: PASS, 44/44.
- deterministic harness smoke: PASS.
- Unreal Linux Compile run `34923459949`: PASS.
- actual UE 5.6 image verification / UHT / UBT / link: PASS.

Delivered:
- personal reproducible `DesignatedSanitationArea` knowledge can materialize only through actual Civilization Craft execution.
- Core owns persistent `PrimitiveSanitationSite` identity, exact GridPos, establisher/minute, active state and use count.
- technique knowledge alone does not create a world site.
- `GetSanitationUseTarget(...)` prefers the active Core designated site before emergency outdoor relief.
- Bridge propagates designated-site flag + site id + exact Core GridPos to World.
- World keeps authored Preferred/Primitive anchors ahead of the Core site, then Core designated Primitive site, then Natural, then Emergency.
- exact site id + GridPos is required for designated completion; stale/wrong identity or position fails closed.
- successful designated use increments site use count and deposits HumanWaste at the acknowledged site cell.
- snapshot binary format v5 persists designated sanitation sites and older v1-v4 snapshots decode with no site state.
- no global `LatrineUnlocked` or automatic modern Toilet/Latrine SmartObject.

## Previous Jjun product checkpoint — PR #78 DONE

PR #78 `[CORE] Add primitive sanitation experimentation progression v1` merged as:
- merge SHA: `3a9739682034ad7c5009da5f75a11b0f07fed55b`
- validated head: `e78f6d211589b74008ff0fdf73e9be1048e2d807`

Validation:
- Structural Preflight run `34921390473`: PASS.
- Core Tests run `34921390510`: PASS, 43/43.
- Unreal Linux Compile run `34921390491`: PASS.
- actual UE 5.6 image verification / UHT / UBT / link: PASS.

Delivered:
- recognized sanitation problem + sufficiently clean candidate site gate sanitation experimentation.
- personal `DesignatedSanitationArea` technique added to existing civilization Experiment/Knowledge machinery.
- deterministic failure becomes Hypothesized; deterministic success becomes Reproducible.
- existing witness/teaching provenance and civilization read models cover the new technique without global unlock.
- existing civilization snapshot extension persists the technique.
- discovery creates no Toilet/Latrine SmartObject and no `LatrineUnlocked` flag.

## Previous Jjun product checkpoint — PR #77 DONE

PR #77 `[CORE] Add sanitation problem recognition v1` merged as:
- merge SHA: `3ea6048d9e7ac6b31e75faa4f5ca55b5c46f9016`
- validated head: `26a0af034364f7ae12b636cfc5a991d57fe729ab`

Validation:
- Structural Preflight run `34919266787`: PASS.
- Core Tests run `34919266571`: PASS, 42/42.
- Unreal Linux Compile run `34919266720`: PASS.
- actual UE 5.6 UHT / UBT / link: PASS.

Delivered:
- direct sanitation Memory evidence can become a durable resident-level sanitation-problem Belief.
- weak single exposure does not instantly create recognized civilization knowledge.
- repeated qualifying direct evidence, or one exceptionally salient direct event, can establish recognition.
- recognition reuses Core Memory → Belief authority.
- evidence re-evaluation is idempotent and cannot inflate support on every tick.
- environmental exposure reports recognized/newly-recognized/confidence.
- snapshot encode/decode/restore preserves the Belief.
- no pit/latrine/global tech unlock is created by #77.

## Previous Jjun product checkpoint — PR #76 DONE

PR #76 `[WORLD] Consume Core sanitation recommendation for emergency toilet movement` merged as:
- merge SHA: `3bb50056311b3a9a75c6ce2bb2317b10163e69c5`
- validated head: `e089857710f6ef5430f57940167362ff39d08d64`

Validation:
- Structural Preflight run `34918024951`: PASS.
- Unreal Linux Compile run `34918024929`: PASS.
- actual UE 5.6 UHT / UBT / link: PASS.

Delivered:
- WorldDirector emergency Toilet consumes Core `GetRecommendedOutdoorReliefGridPosition(...)`.
- exact Core GridPos is projected to World using `CoreGridCellSizeUU`.
- previous independent 650uu sanitation target removed.
- recommendation failure fails closed rather than creating a competing World authority.
- emergency Toilet arrival radius stays inside the authoritative Core cell before ACK.
- existing movement → use-duration → actual completion GridPos ACK remains intact.
- residue/avoidance feedback can now agree with the visible World location.
- Preflight regression guard added.

## Previous Jjun product checkpoint — PR #75 DONE

PR #75 `[CORE] Add environmental exposure perception and avoidance v1` merged as:
- `157ce9937e53d5868d7e558b4149a4fa56c4c454`

Validated head `c36a31d0f0e1caa069f5389735b3828289d252e9`:
- Structural Preflight: PASS
- Core Tests: PASS, 41/41
- Unreal Linux Compile: PASS
- actual UE 5.6 UHT / UBT / link: PASS

Delivered:
- environmental contamination perception.
- hygiene burden and emotional discomfort baseline.
- location-specific sanitation memory.
- remembered + physical contamination avoidance scoring.
- deterministic low-exposure outdoor sanitation recommendation.
- Save/Load continuity.
- Bridge API `GetRecommendedOutdoorReliefGridPosition(...)`.

Disease/pathogen health modelling is not part of #75.

## Core ↔ World repair status

The old `Core ↔ World Execution Sync v1 — READY_NOW` entry is obsolete. Most verification findings were repaired by #70–#79.

### Resolved / baseline closed

- #70: dynamic resident actor reconciliation + ActivityAnchor refresh.
- #71: UE compile trigger coverage expanded to all LifeLens/LifeLensCore C++ paths.
- #72: external physical execution ACK Core contract.
- #73: World arrival/use → Core completion ACK integration, actual resolved position, 100 uu/tile baseline.
- #74: resident runtime position restore from authoritative Core GridPos.
- #75: environmental perception/memory/avoidance recommendation.
- #76: World consumes the authoritative sanitation recommendation and ACKs the actual visible location.
- #77: repeated/salient sanitation evidence can become explicit resident problem recognition.
- #78: recognized concern can drive deterministic sanitation experiment/discovery and personal knowledge.
- #79: discovered sanitation knowledge can become a persistent Core-owned primitive site with exact identity/GridPos carried through World movement and completion ACK.

### Still partial / follow-up

- one exact shared facility/resource target authority across all authored and civilization-created facilities.
- facility identity / tier / quality / backing-resource facts in the execution contract beyond the sanitation-specific #79 path.
- target disappearance/path failure/re-resolve semantics across generic facilities.
- child initial Core GridPos at birth.
- dormant `ULLDecisionComponent` competing chooser removal/restriction.
- explicit legacy snapshot migration fixtures.

## World Affordance / Environment — canonical

- Simulation/environment consequence: `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`
- Environmental visual feedback: `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`
- World visual presentation: `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`

Rules:
- initial world must not silently spawn modern civilization infrastructure.
- fallback order: `Preferred → Primitive → Natural → Emergency → Unavailable`.
- Core is simulation/intent authority; physical World reports execution facts.
- Eat/Drink never create resources implicitly.
- environmental consequence belongs to Core authority and Save/Load.
- visual presentation must not become a second world authority.
- meaningful environmental consequence must have a visual representation path.

### Completed environmental slices

- #66 World Affordance Fallback — DONE.
- #68 Environmental Residue — DONE.
- #75 Environmental Exposure / Perception / Avoidance — DONE.
- #76 World sanitation recommendation integration — DONE.
- #77 Sanitation Problem Recognition v1 — DONE.
- #78 Primitive sanitation experimentation progression v1 — DONE.
- #79 Designated sanitation area authoritative affordance v1 — DONE.

### Immediate environmental loop

`Core low-contamination recommendation`
→ `World actual target/movement`
→ `completion ACK at same GridPos`
→ `HumanWaste residue`
→ `resident exposure/Memory`
→ `next-location avoidance`
→ `problem recognition`
→ `primitive sanitation experiment/discovery`
→ `authoritative designated sanitation area` **DONE #79**
→ **`dug pit / primitive latrine improvement` IN_PROGRESS**
→ `visual feedback`

## Character Appearance v1 — ACTIVE / PR #67 CLOSEOUT

Owner: 다겸 / 다겸 AI
Canonical acceptance: `docs/CHARACTER_APPEARANCE_ROADMAP.md`
Canonical asset track: `docs/CHARACTER_ASSET_TRACK.md`

Latest checked head:
- `e034fe785ca46dd5cb39fd7d7e8710d677994a38`

Latest actual workflow lookup for that head:
- Preflight `34919060096`: PASS.
- Unreal Linux Compile `34919060115`: PASS.

Current review status:
- Jjun review remains `CHANGES_REQUESTED`.
- requested closeout: remove tracked `__pycache__/*.pyc` + add ignore rules; remove duplicate includes; verify/fix bright-skin male Peasant exposed-skin tone consistency.
- latest PR comment asks Dagyeom to push the closeout commit and report new HEAD + CI.
- no newer remote HEAD / Dagyeom reply was present at the latest reconciliation checkpoint.

Previously validated/reported in PR #67:
- Core Tests PASS on an earlier final code head.
- local PIE and Save/Load appearance continuity checks reported PASS.

Reported/implemented:
- Quaternius CC0 humanoids.
- deterministic #65 appearance projection.
- hair / skin / body variation baseline.
- UAL animation assets.
- Peasant outfit integration.
- head-only body derivative to prevent outfit penetration.
- appearance construction after resident identity binding, fixing invalid-id identical appearances.
- humanoid residents visible in PIE.

Remaining before DONE:
- close three review items on a new remote HEAD.
- final CI / review / merge / docs sync.

Known limitation:
- Idle-looking movement slide remains until Motion Bootstrap.

## Character visual direction

After #67:
1. Motion Bootstrap: Idle / Walk / Jog + basic orientation.
2. World Visual Environment v1.
3. remaining Motion/Context: sit / stand / lie / wake / gaze / IK / interaction transitions.

World Visual v1 must follow authoritative environmental consequence rules from `WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`.

## Open-ended invention direction

Canonical: `docs/OPEN_ENDED_INVENTION_v1.md`.

Status:
- design/product rule: fixed.
- generic arbitrary artifact runtime engine: not yet implemented.

Rules:
- do not collapse civilization into a fixed global Tech Tree.
- reality-compatible novel artifacts may emerge from materials/components/connections/use results.
- failed experiments remain learning evidence.
- no global auto-unlock.

## Dagyeom API / design handoff

Current blockers from Jjun for #67: **review closeout 3건 only**.

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

## Integration Requests

Current open blockers from Jjun for Dagyeom #67: **3 review closeout items**.

### Motion promotion

**ACCEPTED WITH GATE.**
- Motion Bootstrap becomes READY_NOW only after #67 is validated, merged and docs synchronized.
- bootstrap limited to Idle/Walk/Jog + basic orientation.
- World Visual v1 takes priority after that checkpoint.

### Clothing / skin variety

**PARTIALLY ACCEPTED / SPLIT.**
- minimum default clothing belongs to #67 acceptance.
- extra clothing/skin detail is follow-up enhancement.
- each external asset pack requires exact source/version/license provenance.

## Merge / execution queue

1. #63 Character Presentation — DONE.
2. #65 Appearance projection contract — DONE.
3. #66 World Affordance Fallback — DONE.
4. #68 Environmental Residue — DONE.
5. #69 Whole-project verification — DONE WITH FINDINGS / closed verify-only.
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
16. **Dug pit / primitive latrine progression — IN_PROGRESS (`jjun/primitive-latrine-progression-v1`).**
17. Character Appearance #67 — ACTIVE CLOSEOUT in parallel.
18. HumanWaste visual feedback.
19. Character Motion Bootstrap — after #67.
20. World Visual Environment v1.
21. Character Motion & Context remainder.
22. Observer UX / Mobile Touch / Visual Feedback UI work.
23. integrated runtime verification.
24. Android smoke APK + device profiling.
25. MetaHuman comparison only after Android/mobile baseline.
26. deeper open-ended invention / health / civilization production chains.

## Completion rule

검증 + merge + 상태 동기화까지 완료되어야 DONE이다.
