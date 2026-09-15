# LifeLens Canonical Work State

> 실제 GitHub `main` / target branch / PR / Actions가 항상 최우선 진실이다.
>
> 현재 전체 진행 요약: `docs/PROJECT_PROGRESS_2026-09-15.md`

Canonical product references:
- Master: `docs/LIFELENS_SPEC_v1.1.md`
- Civilization: `docs/CIVILIZATION_PROGRESSION_v1.md`
- Open-ended invention: `docs/OPEN_ENDED_INVENTION_v1.md`
- World affordance/environment: `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`
- **World genesis / chunks / migration: `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`**
- World visual environment: `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`
- Environmental visual feedback: `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`
- Character appearance: `docs/CHARACTER_APPEARANCE_ROADMAP.md`
- State/collaboration: `docs/STATE_MANAGEMENT.md`, `docs/INTEGRATION_SPRINT.md`
- Whole-project verification baseline: `docs/WHOLE_PROJECT_VERIFICATION_2026-09-14.md`

Last reconciled: 2026-09-15 KST

## Mandatory sync gate

1. 작업 시작 전 actual `main`, target branch/PR, Actions 확인.
2. `WORK_STATE.md` → `TEAM_BOARD.md` → `HANDOFF_LOG.md` 대조.
3. stale 문서가 있으면 코드 작업보다 먼저 갱신.
4. ACTIVE_LOCK 파일은 lock owner 외 수정 금지.
5. 실제 즉시 착수 가능 상태는 `READY_NOW` 또는 `PARALLEL_SAFE_NOW`뿐이다.
6. 검증 전용 PR은 제품 PR로 병합하지 않는다.
7. Compile PASS와 product DONE은 동일하지 않다. merge + state sync까지 완료되어야 DONE이다.

## Current product-code baseline

Latest product merge:
- PR #85 `[CORE] Add World Genesis WG-2 macro world and viable start region`
- merge SHA: `ed4bf34d4b5bd0eb917a8bfb7fc5da16f52a4907`

Validated PR #85 final head `eac5a50b83416e9f55e160d1e6ad0d022b3f926a`:
- Structural Preflight run `34935360828`: PASS, including WG-2 validator
- Core Tests run `34935360834`: PASS, **47/47**
- deterministic harness smoke: PASS
- Unreal Linux Compile run `34935360862`: PASS
- UE 5.6 image verification / UHT / UBT / link: PASS
- PR comments/reviews/unresolved threads at merge checkpoint: 0

Delivered by #85:
- deterministic coherent macro elevation / moisture / temperature fields.
- broad water / fertility / wood / stone / food / traversal / hazard potentials.
- deterministic biome classification from WorldSeed + GenerationVersion, independent of PopulationSeed.
- deterministic viable initial start-region scoring over 625 candidates.
- selected start is survivable-but-unsolved; no house/toilet/farm/road/tool is created.
- current bootstrap presentation is not forcibly relocated before detailed chunk materialization exists.

Previous product checkpoints:
- PR #83 World Genesis WG-1 — DONE, merge `f5c8cbab3aa41c6a37c3bae06838eeb583749771`.
- PR #82 HumanWaste Environmental Visual Feedback — DONE, merge `831ba22ce17ca5fef8a92f2288e18a0495248a7a`.
- PR #80 Dug sanitation pit progression — DONE, merge `291926cf78d12c1c61284eb9c59a50e7c70e54e7`.

Documentation-only commits may advance `main` beyond the product-code baseline above.

## Current dispatch

### Jjun lane — World Generation Milestone A — READY_NOW

Owner: 쭌 / 쭌 AI
Dependency: WG-1 DONE #83 + WG-2 DONE #85; canonical `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`.
Handoff safety: SAFE.

Development-unit rule:
- from this checkpoint forward, same-purpose / same-layer / same-validation work is grouped into a milestone-sized PR instead of one PR per small contract.
- intermediate commits may be small, but canonical state sync and heavy UE validation happen at meaningful milestone checkpoints.

Milestone A goal:
- turn the WG-1/WG-2 logical world contracts into the first actually materializable natural-world slice without locking LifeLens to a fixed arena.

Milestone A scope:
- deterministic detailed natural chunk baseline (WG-3 core).
- biome/macro-fact-driven local natural resources/environment facts.
- generated-chunk registry and no-reroll identity.
- selected initial start-region materialization boundary.
- initial founder spawn integration into the materialized start region, with zero civilization infrastructure.
- minimum persistence boundary needed so generated/visited natural state cannot silently reroll across unload/load.
- minimal Core/Bridge read contracts needed by later World Visual presentation.

Acceptance:
- generation remains independent of exploration order.
- same WorldSeed + GenerationVersion reproduces untouched detail.
- PopulationSeed does not alter natural world detail.
- start region chosen by WG-2 is the region materialized for production NEW GAME.
- no prebuilt house/toilet/farm/storage/road/tool appears.
- unload/load or Save/Load cannot silently reroll generated/modified regions.
- relevant Core / Preflight / UE compile checks pass once at milestone merge gate.

### World Genesis / Chunk / Migration — WG-1 DONE / WG-2 DONE / MILESTONE A READY_NOW

Canonical: `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`.

Product decision:
- LifeLens is **not** a permanently small fixed arena.
- canonical world chain is:
  `WorldSeed → Macro World → deterministic lazy chunks → persistent human/environmental change → carrying-capacity pressure → migration → additional settlements`.
- same `WorldSeed + GenerationVersion + ChunkCoord` must produce the same untouched natural chunk independent of exploration order.
- initial resident randomization uses a separate `PopulationSeed`, preserving fresh names/traits while allowing same-world replay.
- NEW GAME starts with natural environment + 2 male + 2 female residents + zero civilization infrastructure.
- generated/modified chunks never silently reroll after unload/load.
- distant regions/populations need not render every Actor at full fidelity.

Implementation timing:
- **WG-1 is implemented and merged via PR #83.**
- **WG-2 macro world/start-site selection is implemented and merged via PR #85.**
- small bootstrap/test maps remain allowed for current Core/Bridge/animation validation.
- production World Visual must now consume the merged WG-1/WG-2 contracts; the next Jjun unit is World Generation Milestone A.
- do not lock the project into a hand-authored small arena that later requires a world rewrite.

### Dagyeom lane — Character Motion Bootstrap — ACTIVE / FINAL VISUAL CHECK

Owner: 다겸 / 다겸 AI (Claude)
Branch: `dagyeom/character-motion-v1`
PR: #84
Latest observed head: `70c6544abda45f547f900242753abe523a29e045`
Handoff safety: CONDITIONAL — CI green; PIE visual confirmation / final review / merge remain.

Delivered on branch:
- in-place Idle / Walk / Jog / Sprint locomotion BlendSpace.
- actual Actor movement measured over a 0.2s presentation window.
- body-mesh-only orientation smoothing; Actor/Core movement authority unchanged.
- teleport/load-like large steps are excluded from locomotion measurement.
- no Character-side action chooser and no root-motion authority.

Validation observed:
- Preflight `34935329473`: PASS.
- Unreal Linux Compile `34935329453`: PASS.

Exact next action:
- PIE visual confirmation → review/comments check → merge → final canonical state sync.

## Completed foundation / repair slices

### #66 World Affordance Fallback — DONE
- `Preferred → Primitive → Natural → Emergency → Unavailable`.
- no implicit modern facility spawn.

### #68 Environmental Residue — DONE
- production NEW GAME modern Core SmartObjects removed.
- Core-owned HumanWaste residue.
- accumulation / decay / exposure query.
- snapshot persistence and Unreal read DTO.

### #69 Whole Project Verification — DONE WITH FINDINGS / VERIFY-ONLY CLOSED
- verification report: `docs/WHOLE_PROJECT_VERIFICATION_2026-09-14.md`.

### #70 Runtime resident / dynamic affordance reconciliation — DONE
- births/newly projected residents reconcile into World presentation.
- ActivityAnchors refresh beyond BeginPlay.

### #71 Unreal compile trigger coverage — DONE
- all `Source/LifeLens/**` and `Source/LifeLensCore/**` C++ paths trigger actual UE compile validation.

### #72 External physical execution ACK Core contract — DONE
- Core physical intent can remain pending for World execution.

### #73 World physical completion ACK integration — DONE
- actual movement/arrival/use timing drives completion ACK.
- actual completion position returns to Core.

### #74 Runtime position restore — DONE
- authoritative Core runtime GridPos survives snapshot restore and actor reconstruction.

### #75 Environmental Exposure / Perception / Avoidance — DONE
- contamination perception, hygiene/emotion burden, Memory, dedupe, avoidance and low-exposure recommendation.

### #76 World sanitation recommendation integration — DONE
- Core recommendation drives visible World emergency sanitation target and same-cell ACK/residue.

### #77 Sanitation Problem Recognition v1 — DONE
- direct Memory evidence can promote to durable sanitation-problem Belief.
- Core 42/42 + Preflight + UE compile PASS.
- merge SHA `3ea6048d9e7ac6b31e75faa4f5ca55b5c46f9016`.

### #78 Primitive sanitation experimentation progression v1 — DONE
- recognized concern + clean candidate site gates `DesignatedSanitationArea` experimentation.
- personal knowledge, provenance, persistence; no world auto-unlock.
- Core 43/43 + Preflight + UE compile PASS.
- merge SHA `3a9739682034ad7c5009da5f75a11b0f07fed55b`.

### #79 Designated sanitation area authoritative affordance v1 — DONE
- reproducible sanitation knowledge materializes only through actual Craft execution.
- Core-owned persistent site identity/GridPos/useCount.
- exact site id + GridPos drives World movement and completion ACK.
- snapshot binary v5 persists site state.
- Core 44/44 + deterministic harness + Preflight + UE compile PASS.
- merge SHA `90f2b4f9cdc480e99886632e09323b2feab9625c`.

### #80 Dug sanitation pit progression v1 — DONE
- personal `DugSanitationPit` discovery/provenance.
- repeated excavation work; no instant upgrade.
- same site id/GridPos upgrades to `DugPit`.
- containment reduces exposure without deleting HumanWaste.
- primitive sanitation extension v2 with backward-readable v1 data inside outer snapshot v5.
- Core **45/45** + deterministic harness + Preflight + UE 5.6 UHT/UBT/link PASS.
- merge SHA `291926cf78d12c1c61284eb9c59a50e7c70e54e7`.

## Character / presentation sequence

1. Character Presentation v1 — DONE via #63.
2. Appearance projection contract — DONE via #65.
3. Character Appearance v1 — DONE via #67.
4. Motion Bootstrap — **ACTIVE #84; CI PASS, PIE/review/merge pending** (Dagyeom lane).
5. **World Genesis WG-1 DONE #83 / WG-2 DONE #85; World Generation Milestone A — READY_NOW.**
6. World Visual Environment v1 — presentation prototype may proceed after Motion, but production map must respect World Genesis/Chunk contract.
7. Character Motion & Context remainder.
8. Observer UX Polish / Mobile Touch / presentation feedback afterward.

Motion Bootstrap scope:
- Idle / Walk / Jog or Run.
- basic orientation smoothing.
- remove Idle-sliding.
- Character presentation must not become action authority.

## Environment / civilization causal chain

Canonical causal chain:

`Need → Intent → current-world affordance → movement/arrival → action result → environment consequence → exposure/Memory → changed behavior → problem recognition → experiment/discovery → better affordance → culture/civilization`

Current sanitation sequence:
1. WorldDirector sanitation recommendation integration — DONE #76.
2. Sanitation Problem Recognition v1 — DONE #77.
3. Primitive sanitation experiment / DesignatedSanitationArea discovery — DONE #78.
4. Designated sanitation area authoritative affordance — DONE #79.
5. Dug sanitation pit progression — DONE #80.
6. **HumanWaste Environmental Visual Feedback — DONE #82.**
7. material-backed latrine superstructure / further sanitation improvement — later causal progression.
8. Health/pathogen and water/soil contamination — later.

## World scaling / multi-generation rule

Canonical: `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`.

Rules:
- bootstrap gray plane is not production world architecture.
- logical world is larger than the currently rendered region.
- natural world comes from deterministic seed-driven generation.
- detailed chunks are generated lazily and order-independently.
- history persists as chunk/world deltas; unloading never rerolls history.
- carrying capacity is dynamic and depends on environment + infrastructure + knowledge.
- population pressure can produce exploration, household/group migration and additional settlements rather than hard crowding in one fixed map.
- distant residents/regions can remain logically simulated without all 3D Actors being active.
- Unreal streaming/PCG/World Partition choices are presentation/implementation tools, not simulation authority.
- Android-first profiling decides chunk/render detail, not the existence of the larger logical world.

## Open-ended invention status

Canonical: `docs/OPEN_ENDED_INVENTION_v1.md`.

Status:
- product rule/design: DONE.
- fully generic runtime artifact engine: NOT YET IMPLEMENTED.

Required eventual behavior:
- non-historical / developer-unpredicted artifact forms may emerge.
- material + component + connection + physical/use outcome determine usefulness.
- no global auto-unlock.
- failure contributes to learning.
- physical impossibilities do not become magic inventions.

## Environmental visual feedback status

Canonical: `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`.

Rules already fixed:
- simulation/environment state is authoritative.
- visual state follows creation / intensity / decay / removal / SaveLoad restore.
- meaningful physical consequence has a visual representation path.
- Android uses pooling / instancing / LOD / culling / clustering.

Actual visual implementation:
- **HumanWaste baseline ground feedback — DONE #82.**
- material/mesh polish may continue later without changing simulation authority.
- resource depletion/regrowth.
- fire/smoke/scorch.
- foot traffic trails.
- construction/damage state visuals.

## Remaining structural work / known risks

### Unified facility/resource authority — PARTIAL
#79/#80 establish one authoritative sanitation facility identity/GridPos path, but all World facilities/resources do not yet share one generic registry.

Follow-up:
- facility identity / tier / quality / backing-resource contract.
- civilization-created and authored facilities use one authority path.
- target disappearance / path failure / re-resolve semantics.

### World Genesis runtime — WG-1 + WG-2 IMPLEMENTED / MILESTONE A READY_NOW
- WG-1 deterministic world/chunk coordinates — **DONE #83**.
- WG-2 macro world/start-site selection — **DONE #85**.
- **World Generation Milestone A — READY_NOW:** WG-3 detailed natural chunks + start-region materialization + initial spawn integration + minimum no-reroll persistence boundary.
- deeper WG-4 persistent world deltas continue inside/after Milestone A as scope proves safe.
- WG-5 Unreal streaming presentation.
- WG-6 carrying capacity/migration.
- WG-7 multi-settlement society.

Production-sized World Visual work must not assume a permanently fixed small arena.

### Birth initial physical position — NEEDS FIX
New child runtime position may default to `{0,0}`.

### Legacy `ULLDecisionComponent` — REVIEW / REMOVE
Dormant Blueprint-callable competing chooser remains.

### Snapshot legacy migration fixtures — NEEDS TEST
Binary migration support exists, but representative old-format fixture regression coverage remains incomplete.

### Health/pathogen environment feedback — LATER
Current sanitation chain stops at contamination/hygiene/Memory/avoidance/primitive containment.

### Death presentation policy — UNDECIDED
Core death exists; Unreal body/actor/observer presentation requires an explicit policy.

### Android product validation — NOT DONE
Still required:
- smoke package path.
- Android Cook / Package.
- APK artifact.
- real-device run.
- performance / thermal / memory profiling.

Do not revive the old multi-hour PR #2 path as the default loop.

## Canonical execution order

1. #63 Character Presentation — DONE.
2. #65 Appearance contract — DONE.
3. #66 World Affordance Fallback — DONE.
4. #68 Environmental Residue — DONE.
5. #69 Verification — DONE WITH FINDINGS.
6. #70 runtime residents/dynamic affordances — DONE.
7. #71 compile trigger coverage — DONE.
8. #72 external execution Core ACK — DONE.
9. #73 World ACK integration — DONE.
10. #74 runtime position restore — DONE.
11. #75 environment exposure/perception/avoidance — DONE.
12. #76 World sanitation recommendation integration — DONE.
13. #77 Sanitation Problem Recognition — DONE.
14. #78 Primitive sanitation experimentation progression — DONE.
15. #79 Designated sanitation area authoritative affordance — DONE.
16. #80 Dug sanitation pit progression — DONE.
17. HumanWaste visual feedback #82 — DONE.
18. Character Appearance #67 — DONE.
19. **Motion Bootstrap — READY_NOW** (Dagyeom lane).
20. **World Genesis WG-1 — DONE #83; WG-2 — DONE #85; World Generation Milestone A — READY_NOW.**
21. World Visual Environment v1 production world work.
22. Character Motion & Context remainder.
23. Integrated runtime verification.
24. Android smoke APK + device profiling.
25. MetaHuman comparison only after mobile baseline validation.
26. deeper open-ended invention / production / health / multi-settlement society simulation.

## Frozen legacy

- PR #2 / `task/03-fast-test`: FROZEN.
- old Run `34739283266` failed before Cook/Package/APK.
- do not revive the old multi-hour path as normal iteration.

## Recovery rule

If interrupted:
1. fetch actual GitHub main / open PRs / Actions.
2. read `docs/PROJECT_PROGRESS_2026-09-15.md`.
3. reconcile this file + `TEAM_BOARD.md` + `HANDOFF_LOG.md`.
4. inspect ACTIVE_LOCK.
5. choose only READY_NOW / PARALLEL_SAFE_NOW work.
6. resume from the last verified checkpoint, not from stale conversation assumptions.
