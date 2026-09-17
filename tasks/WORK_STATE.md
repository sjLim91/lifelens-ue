# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Long-term order: `docs/DEVELOPMENT_MILESTONES.md`.
> Durable design decisions: `docs/DECISION_LOG.md`.

Last reconciled: 2026-09-16 KST during Action Completion Unification v1.

## Current main baseline

Latest functional checkpoints:
- PR #108 — Android build pipeline split: **MERGED** as `0d3da3d7f69c2e3c2ad54b2b4f43d19822ee8760`.
- PR #109 — Observer Camera Control v1 refresh: **MERGED** as `47f75d2cb7fd93b26b8b2082bf9f30cfafe67bed`.
- PR #110 — Observer mobile UI polish refresh: **MERGED** as `e6443d693076791e3981047d827ff5a64e4858f2`.
- PR #111 — Civilization resource/storage spatial targets: **MERGED** as `dda35bac7cef4a88433f4f6a362b89c1b3ea2eee`.
- PR #112 — Observer Resident Detail Data v1 + authoritative Traits/Preferences: **MERGED** as `27ba0aa147fc38ad05cf388e9390dd2dcaccdf30`.
- PR #114 — World Obstacle Collision v1: **MERGED** as `3a544fc36d743d9afa1b3ec3100f4577db13ba75`.
- PR #115 — Lifecycle Core Correctness v1: **MERGED** as `0a8014482a8eae994e3469f3264760b9d05e99cc`.

Authority rule remains unchanged: Core/World owns simulation truth. UI/Character/Environment/WorldPresentation presents that truth and must not create a second authority.

## Active branch checkpoint

### Action Completion Unification v1 — ACTIVE

Canonical contract: `docs/ACTION_COMPLETION_UNIFICATION_v1.md`.

Branch: `jjun/action-completion-unification-v1`.

Purpose:
- stop Social / Civilization / Parenting outcomes from executing at decision time;
- use one runtime-only pending action contract with stale-token protection;
- route movement / interaction through Unreal presentation;
- apply relationship/resource/care/environment consequences only after authoritative Core ACK;
- preserve the existing physical Eat/Drink/Sleep/Toilet/Hygiene ACK path rather than replacing it.

Implementation in progress:
- `ContextAction.h`: pending Context kind/token/duration, timeout, Core target resolution and read contract.
- `ContextActionRuntime.cpp`: Social/Civilization/Parenting ACK validation and authoritative outcome application.
- `Simulation.cpp`: targeted queue/wait integration only; headless Core still auto-completes through the same completion implementation.
- `LLCoreContextActionBridge.cpp`: pending directive projection + context ACK bridge without expanding the legacy action bridge file.
- `LLWorldDirectorContextActions.cpp`: movement / interaction timing / ACK routing in an isolated World module.
- `LLWorldDirector.cpp`: only the pending-first dispatch hook is added; existing physical/social implementation remains otherwise intact.
- `test_context_action_completion.cpp`: ACK-before-outcome, stale token, spatial target, Save/Load pending-drop and Parenting residue coverage.

Key contract details:
- pending actions are runtime-only and are not serialized;
- stale/wrong token cannot mutate Core;
- Gather/Store use Core ResourceNode/StorageSite positions;
- a blocking resource visual is approached through a truthful interaction radius; Presentation never moves or fabricates the Core coordinate;
- sanitation creation remains bound to the exact Core target selected before movement;
- Parenting ToiletAssist deposits residue only after the caregiver physically completes the interaction.

Expected validation gate before merge:
- all Core tests PASS including `test_context_action_completion`;
- deterministic harness PASS;
- Preflight PASS;
- Unreal Linux UHT/UBT PASS;
- Android build is not required solely for this authority milestone.

## Recently closed product checkpoints

### Lifecycle Core Correctness v1 — DONE

PR #115 merged as `0a8014482a8eae994e3469f3264760b9d05e99cc`.

Final validation:
- Core Tests #534: **PASS**.
- Preflight #642: **PASS**.
- Unreal Linux Compile #148: **PASS**.

Delivered:
- dead residents no longer run Needs/actions/social/civilization work;
- Baby/Toddler ordinary adult autonomy is blocked;
- production Parenting loop is connected and real Feed resources are required;
- close genetic kin are excluded from autonomous romance progression;
- newborn runtime position inherits the gestational parent/partner position instead of `{0,0}`;
- deterministic age-related mortality is wired to pregnancy/household/romance/runtime cleanup;
- no unexplained natural death below age 50 in the v1 mortality model.

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

### Context Action Contract v1 — PROVIDER BASE DONE / COMPLETION FOLLOW-UP ACTIVE
- PR #103 typed Context Action contract and PR #111 spatial targets are merged.
- Character Presentation consumer remains Dagyeom-owned.
- `Action Completion Unification v1` is the active provider follow-up that turns typed intent into an ACK-before-outcome runtime contract.

## Current active / next work

### Jjun lane

Status: `ACTION COMPLETION UNIFICATION V1 ACTIVE`

Current ordered scope:
1. runtime-only pending Context action contract;
2. Social decision -> movement -> ACK -> relationship/emotion result;
3. Civilization decision -> Core target -> movement/interact -> ACK -> resource/knowledge/craft result;
4. Parenting decision -> caregiver approach -> ACK -> care result;
5. stale token / timeout / Save-restore cancellation;
6. Bridge + WorldDirector minimal integration;
7. regression tests + docs + Core/Preflight/Linux validation.

Next canonical priority after this milestone:
- **World / Facility / Obstacle Authority Normalization** — remove Presentation dressing and free-standing ActivityAnchor as sources of physical simulation truth; use Core/World-owned obstacle/facility identities and positions.

After World Authority normalization:
- legacy AI/projection mutator removal;
- child growth presentation/capsule integration;
- Social Communication & Localization;
- Emotion Runtime Integration.

### Dagyeom lane

Status: `IR-D CONTEXT MOTION CONSUMER WORK`

Current facts:
- provider base contracts are merged through #103/#111;
- completion-ACK provider extension is active in Jjun branch;
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

- **Lifecycle runtime — DONE in #115.**
- **Action completion authority — ACTIVE.** Social/Civilization/Parenting are being converted to pending movement/interaction + authoritative ACK.
- **World obstacle/facility authority — NEXT.** Current obstacle collision mirrors WorldPresentation dressing; persistent ActivityAnchor can provide facility behavior without a stable Core facility ID. Final model must be Core/World-owned.

### P1 — after P0 correctness

- WorldDirector Context Motion presentation must consume the completion-aware directive without inventing outcomes; IR-D remains open for Dagyeom presentation polish.
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
- Preflight still asserts existence of some legacy compatibility APIs, which must be updated when those APIs are deliberately removed.

## Current validation risk

`OPEN PIE/APK VISUAL + DEVICE QA RISK — NOT A CORE AUTHORITY BLOCKER`

Next real PIE/APK pass should confirm:
- production map boots correctly;
- four founders and generated world remain readable;
- tree/meaningful-rock clipping is blocked without stuck/jitter/speed burst;
- explicit pebbles/shrubs/grass remain traversable;
- Gather approaches the actual Core target using a truthful interaction radius and mutates inventory only after ACK;
- Social/Parenting approach targets before their Core outcome is applied;
- Sleep/Hygiene fallbacks complete through UE physical ACK;
- Observer Level 2 data remains readable;
- camera/touch/safe area behavior remains correct.

## Long compile rule

When a long UE compile/package is started, record HEAD + Run ID and continue safe independent work. The user reports compile/package completion; **do not repeatedly poll a healthy long-running compile**. Do not restart a healthy run merely because the large Unreal image pull is slow. A successful compile is integration evidence; visual/input quality still requires PIE/APK validation.