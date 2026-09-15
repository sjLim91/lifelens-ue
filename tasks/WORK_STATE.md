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
- PR #82 `[WORLD] Add HumanWaste environmental visual feedback v1`
- merge SHA: `831ba22ce17ca5fef8a92f2288e18a0495248a7a`

Validated PR #82 final head `8c54ca100c28b12a77375ad48626c4d513087b04`:
- Structural Preflight run `34931331778`: PASS
- Environmental visual feedback structural validator: PASS
- Unreal Linux Compile run `34931331779`: PASS
- UE 5.6 image verification / UHT / UBT / link: PASS
- PR comments/reviews/unresolved threads at merge checkpoint: 0

Delivered by #82:
- authoritative Core `HumanWaste` residue is now projected into the Unreal world through a read-only presentation component.
- one HISM component represents many residue records instead of one heavy Actor/Niagara per record.
- Core Grid XY maps to World location and a WorldStatic trace resolves the visible surface Z.
- amount/intensity/radius/age drive footprint/custom presentation data.
- open designated-area residue (`0.42`, radius `3`) remains visibly broader/stronger than DugPit-contained residue (`0.16`, radius `1`) without duplicating site authority.
- visual state is rebuilt from `GetEnvironmentObservation(...)`; presentation never deposits, contains, cleans, or mutates sanitation state.
- Save/Load restoration naturally rebuilds from authoritative residue state.
- Android baseline includes HISM, max-instance cap, culling, and signature-based rebuild suppression.

Latest Core/civilization baseline remains PR #80 `[CORE] Add dug sanitation pit progression v1`.

Validated PR #80 head `fc918693bcd265f3c021f0bd826f6ba5a7113678`:
- Structural Preflight run `34926841905`: PASS
- Core Tests run `34926841895`: PASS, **45/45**
- deterministic harness smoke: PASS
- Unreal Linux Compile run `34926841907`: PASS
- UE 5.6 image verification / UHT / UBT / link: PASS

Delivered by #80:
- personal `DugSanitationPit` technique and `DigSanitationPit` experiment added after `DesignatedSanitationArea`.
- an actual designated sanitation site plus meaningful evidence/use is required before the dug-pit experiment can become relevant.
- discovery remains personal; existing witness/imitation/teaching provenance paths can spread knowledge without global unlock.
- knowledge alone does not instantly create a pit.
- repeated Civilization Craft work accumulates excavation progress.
- the existing authoritative sanitation site keeps the **same site id and GridPos** and upgrades from `DesignatedArea` to `DugPit`; no duplicate facility authority is created.
- current no-Dig-tool state uses slower manual/primitive excavation; future Dig-capable tools can accelerate the same work contract.
- pit completion contains existing HumanWaste by reducing exposure intensity/radius without deleting waste amount.
- future DugPit use still produces HumanWaste, but at a lower exposure profile than an open designated area.
- physical use still requires the exact authoritative site id + GridPos through the World → Core ACK path.
- outer snapshot binary remains v5; primitive sanitation sub-extension advances to v2 and remains compatible with v1 sanitation data.
- site kind/work progress/improver/improvement minute survive Save/Load with reference/time validation.
- Unreal civilization read DTOs now expose both `DesignatedSanitationArea` and `DugSanitationPit`.
- no global `LatrineUnlocked` or automatic modern toilet/plumbing was introduced.

Documentation-only commits may advance `main` beyond the product-code baseline above.

## Current dispatch

### Jjun lane — World Genesis WG-1 — READY_NOW

Owner: 쭌 / 쭌 AI
Dependency: canonical `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`; PR #82 DONE.
Handoff safety: SAFE; HumanWaste visual slice merged and state-synced.

Goal:
- establish the deterministic logical world coordinate/chunk identity contract before production World Visual commits to a fixed map.
- separate natural-world identity from initial population randomness.

WG-1 scope:
- `WorldSeed` and explicit `GenerationVersion` contract.
- stable `ChunkCoord` / chunk key representation.
- deterministic untouched-chunk baseline derived only from world seed + generation version + chunk coordinate.
- exploration/generation order must not affect untouched geography.
- define world-grid/chunk conversion boundaries without making Unreal streaming or PCG the simulation authority.
- preserve current bootstrap/test map compatibility while the scalable logical world foundation is introduced.

Acceptance:
- same inputs produce identical untouched chunk identity/baseline regardless of request order.
- different seeds can produce different natural baselines.
- PopulationSeed/random resident generation is not coupled to terrain generation.
- no production-sized fixed arena assumption is introduced.
- relevant Core/Preflight/UE compile checks pass.

### World Genesis / Chunk / Migration — DESIGN FIXED / IMPLEMENTATION GATE

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
- small bootstrap/test maps remain allowed for current Core/Bridge/animation validation.
- before `World Visual Environment v1` becomes a production-sized permanent map, **WG-1 deterministic world/chunk coordinate contract and WG-2 macro world/start-site boundaries must be implemented or explicitly integrated into that work**.
- do not lock the project into a hand-authored small arena that later requires a world rewrite.

### Dagyeom lane — Motion Bootstrap — READY_NOW

Owner: 다겸 / 다겸 AI (Claude)
Dependency: PR #67 DONE.
Branch / PR: not yet observed on remote at this reconciliation checkpoint.

Character Appearance v1 closeout:
- PR #67 final head `982d930a53f199b33ebf7ca3d4f5b72f72f8a91b`.
- helper PR #81 merged into the Dagyeom branch to sync latest main and close the final three review items.
- tracked `__pycache__/*.pyc` removed and ignore rules added.
- duplicate appearance/presentation includes removed.
- male Peasant exposed forearm/hand skin now selects the same light/dark skin BaseColor axis and tint as face/head.
- prior `CHANGES_REQUESTED` review dismissed after verification; final review approved.
- Preflight `34929703738`: PASS.
- Unreal Linux Compile `34929703712`: PASS including UE 5.6 image verify / UHT / UBT / link.
- helper integration Core Tests `34929611584`: PASS, 45/45 + deterministic harness.
- squash merge to main: `915906357d9752a5654b3dfeb85795419d885b59`.

Motion Bootstrap scope:
- Idle / Walk / Jog.
- velocity/movement-state-driven presentation switching.
- basic orientation smoothing.
- remove Idle-looking slide.
- Character animation remains presentation only; Core/World remains movement/action authority.

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
4. Motion Bootstrap — **READY_NOW** (Dagyeom lane).
5. **World Genesis WG-1/WG-2 architectural gate** — before production-sized permanent World Visual implementation.
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
6. **HumanWaste Environmental Visual Feedback — READY_NOW.**
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

Actual visual implementation still pending:
- **HumanWaste decal/material/VFX — READY_NOW.**
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

### World Genesis runtime — DESIGN FIXED / NOT IMPLEMENTED
- WG-1 deterministic world/chunk coordinates.
- WG-2 macro world/start-site selection.
- WG-3 lazy natural chunks.
- WG-4 persistent chunk deltas.
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
17. **HumanWaste visual feedback — READY_NOW.**
18. Character Appearance #67 — ACTIVE in parallel.
19. Motion Bootstrap — after #67.
20. **World Genesis WG-1/WG-2 architecture implementation gate.**
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
