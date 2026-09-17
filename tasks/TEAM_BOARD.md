# LifeLens Team Board

이 파일은 **ownership / active locks / Integration Requests / cross-lane coordination**만 기록한다.

- live execution state: `tasks/WORK_STATE.md`
- canonical roadmap: `docs/DEVELOPMENT_MILESTONES.md`
- integrated audit: `docs/INTEGRATED_AUDIT_2026-09-17.md`

## Ownership

| Lane | Owner | Default scope |
|---|---|---|
| Core / AI / Simulation / World / Save / Bridge | Jjun | `Source/LifeLensCore/**`, `Source/LifeLens/AI/**`, `Source/LifeLens/Simulation/**`, `Source/LifeLens/World/**` |
| Build / CI / Android / Config | Jjun | `.github/workflows/**`, build pipeline, `Config/**`, startup/default map, project integration |
| UI / Observer | Dagyeom | `Source/LifeLens/UI/**`, `Content/UI/**` |
| Character presentation | Dagyeom | `Source/LifeLens/Characters/**`, `Content/Characters/**` |
| Environment / maps / WorldPresentation | Dagyeom | `Source/LifeLens/WorldPresentation/**`, `Content/Environment/**`, `Content/Maps/**`, `Content/WorldPresentation/**` |

Authority rule:

> Core / World owns simulation truth. Presentation consumes authoritative read/action contracts and never invents resources, facilities, outcomes, lifecycle state or technology.

## Collaboration model

LifeLens uses **one integrated roadmap with parallel ownership lanes**.

Rules:
- Jjun defines Core/World truth and provider contracts.
- Dagyeom consumes those contracts for Character/UI/WorldPresentation.
- each owner stays in their lane by default.
- direct cross-owner edits require an Integration Request or explicit scoped Assist Lock.
- Jjun does not push directly to `dagyeom/*` branches.
- stale branches are not merged wholesale; valid missing ideas are reimplemented from latest `main`.

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

## Current owner work references

### Dagyeom — PR #100 World Readability Envelope — ACTIVE / FINAL CLOSEOUT
- branch: `dagyeom/world-visual-readability-envelope`.
- current head: `62b191a6afb2c0c5ae32ddbb1e0ae7876ae00556`.
- exact-head Preflight #707 — PASS.
- exact-head Unreal Linux Compile #200 — PASS.
- PR #128 removed the Mac editor build blocker.
- remaining action: Dagyeom Mac PIE visual confirmation, then merge if acceptable.
- owner lane only: WorldPresentation; Core authority unchanged.

### Dagyeom — PR #98 Observer Readability / QA View — STALE
- **do not merge as-is**.
- selectively reimplement only still-useful ideas on current main.
- Observer true scrolling remains a separate current-main task.

### Jjun — next provider milestone
- **Emotion Runtime Integration v1 — ACTIVE**.
- branch: `jjun/emotion-runtime-integration-v1`.
- no Assist Lock required for Core provider work.
- if new presentation-only fields or motion states are needed, expose/extend a read/action contract first instead of editing Dagyeom UI/Character directly.

## Newly available provider contract from PR #130

`KnowledgeTeaching` is now an authoritative ContextAction:
- Core identifies teacher, learner and technique.
- Bridge exposes target resident + technique + token.
- World moves teacher to learner, plays existing talking presentation and ACKs only after arrival/use duration.
- Core revalidates the real meeting at completion.
- Save/Load does not persist pending teaching.

Dagyeom may consume this contract for richer Context Motion or Observer labeling without changing simulation authority. Open an IR only if the existing target/technique/context fields are insufficient.

## Recently completed coordination items

### PR #130 — Knowledge Transmission Spatial Authority v1 — DONE
- owner: Jjun Core / Simulation / World.
- merged as `fb1842ad60c25f0054eb040f46d757340f65991c`.
- exact-head gates: Preflight #715 PASS, Core Tests #664 PASS, Unreal Linux Compile #203 PASS.
- closed remote witness/teaching telepathy.

### PR #128 — Mac editor build unblocker — DONE
- minimal `-Wshadow` variable rename only.
- merged as `7aaac203af35ab806ee8dcebf49bacc13a22b08a`.
- unblocked Dagyeom Mac editor rebuild and visual QA.

### ASSIST_LOCK-LIFECYCLE-PRESENTATION-1 — RELEASED
- PR #125 merged as `e6e005ba1180a4152476ab9b7a19ad9953c07287`.
- Character ownership returned to Dagyeom.

Other older released Assist Locks remain historical and need no active coordination action.

## Open Integration Requests

**None.**

Open a new Integration Request only when one owner actually needs another owner to change a file/API/config outside the requester's lane and the existing contract is insufficient.

A request must contain:
- requester / needed owner;
- exact file/API/config needed;
- why the existing contract is insufficient;
- target branch/PR or asset path;
- whether it blocks the current milestone.

## Recently resolved Integration Requests

- IR-D typed Context Action consumer — RESOLVED through #103/#111/#116/#117 and extended by #130 teaching contract.
- IR-B character facing — RESOLVED by #102.
- IR-A WorldPresentation owner path — RESOLVED.
- IR-C production map + observer framing — RESOLVED by #96.
