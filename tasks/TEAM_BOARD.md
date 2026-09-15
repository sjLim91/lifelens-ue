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
| 쭌 + 쭌 AI | after #77 | Primitive sanitation experimentation / affordance progression | Core/Civilization/World contract | **READY_NOW** |
| 다겸 + 다겸 AI | PR #67 `dagyeom/character-appearance-v1` | Character Appearance v1 | Character appearance + `Content/Characters/**` | **ACTIVE / CLOSEOUT** |
| 다겸 + 다겸 AI | after #67 | Character Motion Bootstrap | Character locomotion presentation | READY_AFTER_#67 |
| 다겸 + 다겸 AI | after Motion Bootstrap | World Visual Environment v1 | Environment/Maps/WorldPresentation | HIGH PRIORITY / READY_AFTER_MOTION_BOOTSTRAP |
| 다겸 + 쭌 Bridge support as needed | after World Visual v1 | Character Motion & Context remainder | Character presentation/animation | AFTER WORLD VISUAL v1 |
| 쭌 + 다겸 lanes | after authoritative sanitation loop | HumanWaste visual feedback | Core read contract + Presentation | NEXT |
| 쭌 + 쭌 AI | PR #2 | old Android validation path | Bridge/build | FROZEN |

## Current Assist Locks

**0개.** Previous appearance/presentation assist locks are released.

## Current Jjun lane — Primitive sanitation experimentation / progression READY_NOW

Dependency:
- PR #77 DONE.

Goal:
- use the resident sanitation-problem Belief created by #77 as a causal input to experimentation/progression.
- recognized concern must precede sanitation invention attempts.
- no fixed global tech unlock and no instant latrine creation.

Planned minimal boundary:
- inspect existing `CivilizationIntent` / experiment machinery first.
- add the smallest sanitation experiment/discovery path that fits the generic civilization loop.
- likely initial responses are designated waste area and/or dug pit attempts before a durable latrine affordance.
- success becomes personal knowledge first; propagation and infrastructure follow later.
- failure remains learning evidence.
- feasibility is constrained by actual resources/material/environment state.

Acceptance:
- no recognized sanitation problem → no spontaneous sanitation experiment.
- recognized concern can deterministically seed/bias an experiment opportunity.
- success/failure is state-driven and testable.
- successful discovery does not create a global magic unlock.
- Core tests + Structural Preflight.

## Latest Jjun product checkpoint — PR #77 DONE

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

The old `Core ↔ World Execution Sync v1 — READY_NOW` entry is obsolete. Most verification findings were repaired by #70–#77.

### Resolved / baseline closed

- #70: dynamic resident actor reconciliation + ActivityAnchor refresh.
- #71: UE compile trigger coverage expanded to all LifeLens/LifeLensCore C++ paths.
- #72: external physical execution ACK Core contract.
- #73: World arrival/use → Core completion ACK integration, actual resolved position, 100 uu/tile baseline.
- #74: resident runtime position restore from authoritative Core GridPos.
- #75: environmental perception/memory/avoidance recommendation.
- #76: World consumes the authoritative sanitation recommendation and ACKs the actual visible location.
- #77: repeated/salient sanitation evidence can become explicit resident problem recognition.

### Still partial / follow-up

- one exact shared facility/resource target authority across all authored and civilization-created facilities.
- facility identity / tier / quality / backing-resource facts in the execution contract.
- target disappearance/path failure/re-resolve semantics.
- child initial Core GridPos at birth.
- dormant `ULLDecisionComponent` competing chooser removal/restriction.
- explicit v1/v2/v3 snapshot migration fixtures.

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

### Immediate environmental loop

`Core low-contamination recommendation`
→ `World actual target/movement`
→ `completion ACK at same GridPos`
→ `HumanWaste residue`
→ `resident exposure/Memory`
→ `next-location avoidance`
→ `problem recognition`
→ **`primitive sanitation experiment/discovery` (READY_NOW)**
→ `better sanitation affordance`
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
14. **Primitive sanitation experimentation / affordance progression — READY_NOW.**
15. Character Appearance #67 — ACTIVE CLOSEOUT in parallel.
16. HumanWaste visual feedback.
17. Character Motion Bootstrap — after #67.
18. World Visual Environment v1.
19. Character Motion & Context remainder.
20. Observer UX / Mobile Touch / Visual Feedback UI work.
21. integrated runtime verification.
22. Android smoke APK + device profiling.
23. MetaHuman comparison only after Android/mobile baseline.
24. deeper open-ended invention / health / civilization production chains.

## Completion rule

검증 + merge + 상태 동기화까지 완료되어야 DONE이다.
