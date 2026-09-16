# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Long-term order: `docs/DEVELOPMENT_MILESTONES.md`.
> Durable design decisions: `docs/DECISION_LOG.md`.

Last reconciled: 2026-09-16 KST during Lifecycle Core Correctness v1.

## Current main baseline

Latest functional checkpoints:
- PR #108 — Android build pipeline split: **MERGED** as `0d3da3d7f69c2e3c2ad54b2b4f43d19822ee8760`.
- PR #109 — Observer Camera Control v1 refresh: **MERGED** as `47f75d2cb7fd93b26b8b2082bf9f30cfafe67bed`.
- PR #110 — Observer mobile UI polish refresh: **MERGED** as `e6443d693076791e3981047d827ff5a64e4858f2`.
- PR #111 — Civilization resource/storage spatial targets: **MERGED** as `dda35bac7cef4a88433f4f6a362b89c1b3ea2eee`.
- PR #112 — Observer Resident Detail Data v1 + authoritative Traits/Preferences: **MERGED** as `27ba0aa147fc38ad05cf388e9390dd2dcaccdf30`.
- PR #114 — World Obstacle Collision v1: **MERGED** as `3a544fc36d743d9afa1b3ec3100f4577db13ba75`.

Authority rule remains unchanged: Core/World owns simulation truth. UI/Character/Environment/WorldPresentation presents that truth and must not create a second authority.

## Active branch checkpoint

### Lifecycle Core Correctness v1 — ACTIVE

Canonical contract: `docs/LIFECYCLE_CORE_CORRECTNESS_v1.md`.

Branch: `jjun/lifecycle-core-correctness-v1`.

Purpose:
- fix lifecycle correctness gaps found during the full-source audit before deeper society/civilization expansion;
- make dead residents truly inactive;
- stop Baby/Toddler from running adult autonomous AI;
- connect authoritative parent care without synthesizing food/water;
- prevent autonomous close-kin romance;
- give newborns a real parent-relative runtime position;
- connect deterministic natural death to pregnancy/household/romance cleanup.

Implementation in progress:
- `Genealogy.h`: prohibited genetic romance kinship rule.
- `Pregnancy.h`: lifecycle termination API.
- `Death.h`: deterministic daily natural mortality policy; death memory provenance corrected to the survivor who owns the memory.
- `Parenting.h`: `requiresDirectCare`, resource-aware Feed, `ToiletAssist`, living-parent validation.
- `Simulation.h/.cpp`: production lifecycle wiring.
- `test_lifecycle_runtime_correctness.cpp`: dead-runtime / direct-care / incest / newborn-position / death cleanup integration coverage.

Expected validation gate before merge:
- all Core tests PASS including `test_lifecycle_runtime_correctness`;
- deterministic harness PASS;
- Preflight PASS;
- Unreal Linux UHT/UBT PASS;
- Android build is not required solely for this Core milestone.

## Recently closed product checkpoints

### World Obstacle Collision v1 — DONE / DEVICE FEEL QA REMAINS

PR #114 merged as `3a544fc36d743d9afa1b3ec3100f4577db13ba75`.

Delivered:
- query-only tree/meaningful-rock collision proxies;
- explicit pebble traversal;
- constant-speed swept resident movement;
- remaining-frame-distance slide/side-step budget;
- stable alternate tangent when the first local bypass is blocked;
- Gather/Store interaction-radius rule documented without moving Core coordinates.

Ownership closeout:
- `ASSIST_LOCK-CHARACTER-OBSTACLE-1` is **RELEASED**.
- normal Character ownership returns to Dagyeom.

Important audit follow-up: the current obstacle proxy source is still WorldPresentation dressing. That is a known authority inversion and is tracked below for the later World Authority normalization milestone; #114 remains a visual-clipping mitigation, not the final authoritative obstacle model.

### Observer Resident Detail Data v1 — DONE

PR #112 delivered direct-Core Needs, all 14 Personality dimensions, explicit 8-axis Traits, explicit 8-axis Preferences, civilization Skills, directional Relationships, Family, Knowledge/Gear, and bounded Trait/Preference behavior integration.

Validation on final PR head:
- Core Tests #515: **PASS, 52/52**.
- deterministic harness smoke: **PASS**.
- Preflight #627: **PASS**.
- Unreal Linux Compile #137: **PASS**.

### Civilization resource/storage spatial authority — DONE
- PR #111 merged as `dda35bac7cef4a88433f4f6a362b89c1b3ea2eee`.
- ResourceNode and StorageSite own authoritative `GridPos`.
- generated ResourceNodes preserve exact `NaturalResourcePatch.pos`.

### Observer Camera Control / mobile UI polish — DONE / DEVICE QA REMAINS
- Camera PR #109 and mobile UI PR #110 are merged.
- PC and Android observer gestures exist; device feel remains a QA item.

### Early Survival Provisioning — DONE
- PR #106 merged as `4b8c938629cca18fafad00de0abcfb8cf41b36c9`.
- urgent Hunger/Thirst promotes only real matching ResourceNode Gather; no synthetic food/water.

### Context Action Contract v1 — PROVIDER DONE / IR-D CONSUMER OPEN
- PR #103 typed Context Action contract and PR #111 spatial targets are merged.
- Character Presentation consumer remains Dagyeom-owned.

## Current active / next work

### Jjun lane

Status: `LIFECYCLE CORE CORRECTNESS V1 ACTIVE`

Current ordered scope:
1. dead-resident runtime guard;
2. dependent-child stage gates + Parenting runtime;
3. close-kin autonomous romance prohibition;
4. newborn runtime position and growth-rate correctness;
5. deterministic death + pregnancy/household/romance lifecycle cleanup;
6. regression tests + docs + Core/Preflight/Linux validation.

Next canonical priority after this milestone:
- **Action Completion Unification** — Social / Civilization / Parenting must move from "decision executes immediately" to `decision -> movement/interaction -> authoritative Core completion/ACK`, matching the already-correct physical action pattern.

After Action Completion Unification:
- World/Facility/Obstacle authority normalization;
- legacy AI/projection mutator removal;
- child growth presentation/capsule integration;
- Social Communication & Localization;
- Emotion Runtime Integration.

### Dagyeom lane

Status: `IR-D CONTEXT MOTION CONSUMER WORK`

Current facts:
- provider contracts are merged through #103/#111;
- Character Presentation owns Context Motion Router/action-to-animation consumption;
- there is currently **no Jjun assist lock** on Character files;
- Observer normal presentation maintenance remains Dagyeom-owned.

### Android / device lane

Status: `GATE B PENDING — NO CANONICAL SUCCESSFUL SEED CACHE RECORDED HERE`

Execution rule:
1. Run `mode=seed` once when a fresh engine cache is required.
2. Successful `seed-engine` chains into Android build/APK.
3. After a genuinely successful Android engine cache exists, use `mode=fast` for normal APK checks.
4. Use `full` only as evidence-driven fallback.

Do not run repeated expensive seed/full jobs blindly.

## Full-source audit findings — ordered backlog

### P0 — current / next correctness work

- **Lifecycle runtime** — ACTIVE in current branch.
- **Action completion authority** — Social/Civilization currently apply outcomes before physical arrival; Parenting runtime introduced by lifecycle work is also Core-immediate until the next unification milestone.
- **World obstacle/facility authority** — current obstacle collision mirrors WorldPresentation dressing; persistent ActivityAnchor can provide facility behavior without a stable Core facility ID. Final model must be Core/World-owned.

### P1 — after P0 correctness

- Gather/Store spatial target should ultimately be authoritative in the Core activity observation itself, not only resolved in the Unreal bridge.
- WorldDirector still does not fully consume Civilization directives as physical movement/context work.
- legacy `ULLDecisionComponent` remains a second callable AI surface even though WorldDirector no longer uses it.
- legacy `ApplyActionOutcome` / `ApplySocialInteraction` projection mutators remain public compatibility APIs.
- death state is absent from legacy `FLLResidentData`; World/Observer actor cleanup must be completed when death presentation is integrated.
- Baby/Toddler visual mesh scale and ACharacter capsule scale are not lifecycle-synchronized.
- Character appearance is built once; LifeStage growth does not yet rebuild/update presentation scale.
- production New Game starts with zero StorageSites and currently has no storage-construction progression, so Store is not naturally reachable.
- civilization knowledge witness/teaching currently lacks physical distance/meeting requirements.
- Observer Detail overflow is indicated but not truly scrollable.

### P2 — cleanup / performance

- environmental residue visualizer can rebuild HISM/ground traces frequently as residue state changes; profile on Android during long-run testing.
- long frame/catch-up simulation stepping needs an explicit per-frame budget.
- `Source/LifeLensCore/src/SimulationSnapshotCodecLegacy.cpp` is uncompiled dead source under the current-only pre-release save policy.
- `LLCoreActionTypes.h` has stale comments predating resource/storage spatial authority.
- Preflight still asserts existence of some legacy compatibility APIs, which must be updated when those APIs are deliberately removed.

## Current validation risk

`OPEN PIE/APK VISUAL + DEVICE QA RISK — NOT A CORE AUTHORITY BLOCKER`

Next real PIE/APK pass should confirm:
- production map boots correctly;
- four founders and generated world remain readable;
- tree/meaningful-rock clipping is blocked without stuck/jitter/speed burst;
- explicit pebbles/shrubs/grass remain traversable;
- Gather approaches the actual Core target using a truthful interaction radius;
- Sleep/Hygiene fallbacks complete through UE physical ACK;
- Observer Level 2 data remains readable;
- camera/touch/safe area behavior remains correct.

## Long compile rule

When a long UE compile/package is started, record HEAD + Run ID and continue safe independent work. The user reports compile/package completion; **do not repeatedly poll a healthy long-running compile**. Do not restart a healthy run merely because the large Unreal image pull is slow. A successful compile is integration evidence; visual/input quality still requires PIE/APK validation.
