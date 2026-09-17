# LifeLens Team Board

이 파일은 **ownership / active locks / Integration Requests / cross-lane coordination**만 기록한다.

- live execution state: `tasks/WORK_STATE.md`
- canonical roadmap: `docs/DEVELOPMENT_MILESTONES.md`
- long-range civilization direction: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`
- integrated audit: `docs/INTEGRATED_AUDIT_2026-09-17.md` — historical point-in-time evidence

## Ownership

| Lane | Owner | Default scope |
|---|---|---|
| Core / AI / Simulation / World / Save / Bridge | Jjun | `Source/LifeLensCore/**`, `Source/LifeLens/AI/**`, `Source/LifeLens/Simulation/**`, `Source/LifeLens/World/**` |
| Build / CI / Android / Config | Jjun | `.github/workflows/**`, build pipeline, `Config/**`, startup/default map, project integration |
| UI / Observer | Dagyeom | `Source/LifeLens/UI/**`, `Content/UI/**` |
| Character presentation | Dagyeom | `Source/LifeLens/Characters/**`, `Content/Characters/**` |
| Environment / maps / WorldPresentation | Dagyeom | `Source/LifeLens/WorldPresentation/**`, `Content/Environment/**`, `Content/Maps/**`, `Content/WorldPresentation/**` |

Authority rule:

> Core / World owns simulation truth. Presentation consumes authoritative read/action contracts and never invents resources, facilities, outcomes, lifecycle state, emotion, knowledge or technology.

## Collaboration model

LifeLens uses **one integrated roadmap with parallel ownership lanes**.

Rules:
- Jjun defines Core/World truth and provider contracts.
- Dagyeom consumes those contracts for Character/UI/WorldPresentation.
- each owner stays in their lane by default.
- direct cross-owner edits require an Integration Request or explicit scoped Assist Lock.
- Jjun does not push directly to `dagyeom/*` branches.
- stale branches are not merged wholesale; valid missing ideas are reimplemented from latest `main`.
- new civilization/future systems expose provider contracts before presentation work begins.

### Cross-lane request checkpoint cadence

Jjun must check Dagyeom-side open PRs, new comments/review requests and new Integration Requests:
- immediately before starting a new Jjun functional work unit;
- immediately after completing or merging a Jjun work unit;
- when entering a long CI / Unreal build wait;
- when returning to act on a long CI / Unreal result;
- before a main-changing merge or rebase.

A new blocker is triaged before unrelated follow-up work. This does **not** mean repeatedly polling or restarting a healthy long build.

## Current Assist Locks

**None.**

## Open Integration Requests

**None at this checkpoint.**

Open a new Integration Request only when one owner actually needs another owner to change a file/API/config outside the requester's lane and the existing contract is insufficient.

A request must contain:
- requester / needed owner;
- exact file/API/config needed;
- why the existing contract is insufficient;
- target branch/PR or asset path;
- whether it blocks the current milestone.

---

## Current owner work references

### Jjun — PR #132 Emotion Runtime Integration v1 — CLOSEOUT

PR: #132.
Actual branch: `jjun/emotion-runtime-integration-v1-20260917`.
Current exact head: `0327afb58caed550b8bb12f046b2ec6738157ef5`.

Current state:
- Needs pressure/resolution, authoritative failure and civilization outcome emotion integration implemented;
- physical utility influence bounded to +/-8%;
- closeout review found production `completeExternalPhysicalAction()` was missing Need-resolution emotion relief;
- gap fixed and production external regression assertion added;
- Preflight + Core Tests PASS on the closeout head;
- final exact-head Unreal Linux Compile required before merge.

Semantic ownership:
- renderer/bridge timeout or invalid/stale ACK does not invent emotion;
- only authoritative Core-lived outcomes may create resident emotion.

No Assist Lock required.

### Dagyeom — PR #100 World Readability Envelope — ACTIVE / REFRESH REQUIRED

Branch: `dagyeom/world-visual-readability-envelope`.
Last known head: `62b191a6afb2c0c5ae32ddbb1e0ae7876ae00556`.

Live repository state now reports the PR as **not mergeable against current main**.
Therefore the old “PIE only” closeout sequence is no longer valid.

Owner actions:
1. refresh/reconstruct still-needed WorldPresentation changes on current main;
2. resolve conflicts without importing stale Core authority;
3. run fresh exact-head Preflight + Unreal Linux Compile;
4. run Mac PIE visual confirmation;
5. merge only if acceptable.

Jjun must not repair this by pushing directly to the Dagyeom branch. Open an IR only if Dagyeom discovers a Core/provider blocker.

### Dagyeom — PR #98 Observer Readability / QA View — STALE

- **do not merge as-is**;
- selectively reimplement only still-useful ideas on current main;
- Observer true scrolling remains a separate current-main task.

---

## Provider contracts available to Dagyeom

### ContextAction / KnowledgeTeaching

`KnowledgeTeaching` is authoritative:
- Core identifies teacher, learner and technique;
- Bridge exposes target resident + technique + token;
- World moves teacher to learner and ACKs only after authoritative completion conditions;
- Core revalidates the real meeting at completion;
- Save/Load does not persist stale pending teaching.

Use for richer teaching/social Context Motion and Observer labeling without changing simulation authority.

### Emotion runtime

After #132 merge, Dagyeom may consume the existing authoritative resident emotion projection for presentation/Observer work.

Rules:
- UI/Character must not calculate a second emotional truth;
- animation/facial/UI state may summarize Core emotion but must not mutate it;
- if later presentation requires missing causal history/event metadata, request a provider read-model extension instead of inferring causes from visuals.

### Lifecycle

Existing Core lifecycle/family/history DTOs remain the authority for Lifecycle Event Presentation v2.
Presentation may show birth/growth/death/history but must not infer a death cause or create grave/corpse state unless Core exposes that truth.

---

# Next work by lane

## Jjun Core/World lane

After #132 merge:

1. **Settlement & Subsistence Foundation**
   - SleepingPlace / Shelter / WorkSurface;
   - real facility effects;
   - cultivation/agriculture / renewable food substrate;
   - Tin/Bronze with real prerequisites.

2. **Long-Run Scale & Cleanup**
   - catch-up budget;
   - population CPU/memory stability;
   - residue/HISM profiling;
   - snapshot/legacy cleanup;
   - multi-century deterministic regression.

3. **Open-Ended Civilization Framework v1**
   - Knowledge / Capability / Technology / CivilizationTransformation;
   - stable IDs/definitions;
   - prerequisites/effects;
   - problem-driven research;
   - diffusion/adoption/loss/rediscovery.

4. **Health / Disease / Population Resilience**.
5. **Education / Recording / Specialization / Economy / Institutions**.
6. **Migration / Multiple Settlements / Trade Networks**.
7. **Historical -> industrial -> modern -> digital capability content**.
8. **AI / automation -> advanced energy/materials/biotech -> space -> open future**.

Canonical architecture: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`.

## Dagyeom Presentation lane

1. #100 current-main refresh + CI + Mac PIE closeout.
2. Character Context Motion v2.
3. Observer Readability + Real Scrolling.
4. Lifecycle Event Presentation v2.
5. consume settlement/civilization capability/history provider contracts as they land.

Parallel execution is expected. Dagyeom tasks do not block unrelated Jjun provider work unless an actual IR is opened.

## Android/device lane

**Gate B remains PAUSED BY USER.**
Do not start Android build/seed merely because roadmap documents changed.

---

## Recently completed coordination items

### PR #130 — Knowledge Transmission Spatial Authority v1 — DONE
- owner: Jjun Core / Simulation / World;
- merged as `fb1842ad60c25f0054eb040f46d757340f65991c`;
- exact-head gates PASS;
- closed remote witness/teaching telepathy.

### PR #128 — Mac editor build unblocker — DONE
- minimal `-Wshadow` rename only;
- unblocked Dagyeom Mac editor work at that checkpoint.

### ASSIST_LOCK-LIFECYCLE-PRESENTATION-1 — RELEASED
- Character ownership returned to Dagyeom after #125.

Other older released Assist Locks remain historical and need no active coordination action.

## Recently resolved Integration Requests

- IR-D typed Context Action consumer — RESOLVED through #103/#111/#116/#117 and extended by #130 teaching contract.
- IR-B character facing — RESOLVED by #102.
- IR-A WorldPresentation owner path — RESOLVED.
- IR-C production map + observer framing — RESOLVED by #96.
