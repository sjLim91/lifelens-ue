# LifeLens Team Board

이 파일은 **ownership / active locks / Integration Requests**만 기록한다.

- live execution state: `tasks/WORK_STATE.md`
- canonical roadmap: `docs/DEVELOPMENT_MILESTONES.md`
- integrated audit: `docs/INTEGRATED_AUDIT_2026-09-17.md`
- durable decisions: `docs/DECISION_LOG.md`

## Ownership

| Lane | Owner | Default scope |
|---|---|---|
| Core / AI / Simulation / World / Save / Bridge | Jjun | `Source/LifeLensCore/**`, `Source/LifeLens/AI/**`, `Source/LifeLens/Simulation/**`, `Source/LifeLens/World/**` |
| Build / CI / Android / Config integration | Jjun | `.github/workflows/**`, build pipeline, `Config/**`, startup/default map, project plugin integration |
| UI / Observer presentation | Dagyeom | `Source/LifeLens/UI/**`, `Content/UI/**` |
| Character presentation | Dagyeom | `Source/LifeLens/Characters/**`, `Content/Characters/**` |
| Environment / maps / world presentation | Dagyeom | `Source/LifeLens/WorldPresentation/**`, `Content/Environment/**`, `Content/Maps/**`, `Content/WorldPresentation/**` |

Authority rule:

> Core / World owns simulation truth. UI / Character / Environment / WorldPresentation presents it and must not duplicate or invent authority.

## Collaboration model

LifeLens now uses **one integrated roadmap with parallel ownership lanes**.

Rules:
- Jjun establishes Core/World truth and read/interaction contracts.
- Dagyeom consumes those contracts for Character/UI/WorldPresentation.
- each owner works in their own lane by default.
- if one owner must directly modify another owner's lane, first create an Integration Request or explicit scoped Assist Lock.
- Jjun does not push directly to `dagyeom/*` branches.
- old branches are not merged wholesale; missing functionality is reimplemented from current `main` when needed.

## Current Assist Locks

**None.**

Normal ownership is fully restored to each lane after PR #125.

## Current owner work references

These are not locks; they are coordination references only.

### Dagyeom — PR #100 World Readability Envelope
- branch: `dagyeom/world-visual-readability-envelope`.
- owner lane: Environment / WorldPresentation.
- based on current #125 main.
- presentation-only settlement readability envelope.
- Core resource/position/action authority is unchanged.
- merge only after latest exact-head required CI passes.

### Dagyeom — PR #98 Observer Readability / QA View
- branch is stale relative to current main.
- **do not merge as-is**.
- useful ideas may be selectively reimplemented on latest main under normal Dagyeom UI ownership.

### Jjun — next functional provider milestone
- Knowledge Transmission Spatial Authority v1.
- no cross-owner lock required unless the implementation later needs Character/UI/WorldPresentation changes beyond existing contracts.

## Recently released Assist Locks

### ASSIST_LOCK-LIFECYCLE-PRESENTATION-1 — RELEASED
- requester/implementer: Jjun under explicit user continue approval.
- owner lane temporarily touched: Dagyeom Character presentation.
- implementation branch: `jjun/lifecycle-presentation-v1`.
- PR #125 merged to `main` as `e6e005ba1180a4152476ab9b7a19ad9953c07287`.
- delivered:
  - living-only physical compatibility projection;
  - deceased actor/runtime cleanup through WorldDirector reconciliation;
  - LifeStage-driven body scale synchronization;
  - capsule scale synchronization while preserving ground contact;
  - no identity/genetics reroll.
- authority source remains `FLLCoreResidentObservation.bAlive` / `LifeStage`.
- release condition satisfied after Preflight #701 PASS + Unreal Linux Compile #197 PASS + merge.
- Character ownership returns to Dagyeom.

### ASSIST_LOCK-FACILITY-TECH-1 — RELEASED
- requester/implementer: Jjun, user-approved Core + World + Character + UI + WorldPresentation vertical slice.
- branch history: `jjun/facilities-tools-technology-v1` -> `jjun/tool-effectiveness-v1` -> `jjun/dig-strike-tools-v1` -> `jjun/fire-heat-firepit-v1` -> `jjun/furnace-smelting-v1`.
- delivered PRs: #118~#122.
- final PR #122 merged as `72e90d5e94085b82806e0b0071f2b81b39cd6f16`.
- delivered authority chain: need/problem -> experiment/discovery -> reproducibility -> actual craft/build -> real efficiency/world effect -> Bridge/Character/UI/WorldPresentation -> Save/Load.
- New Game starts with zero fabricated facilities.
- ownership returned to normal lanes after merge.

### ASSIST_LOCK-SOCIAL-COMM-1 — RELEASED
- requester/implementer: Jjun.
- PR #117 merged as `250b3c2ed37ebb7b49e78f5305d429fa3781b7ba`.
- delivered: ACK-backed social event feed, Korean Observer labels/bubbles, recent interaction detail, target facing, `Idle_Talking_Loop` context motion.
- UI/Character ownership returned to Dagyeom.

### ASSIST_LOCK-CHARACTER-OBSTACLE-1 — RELEASED
- requester/implementer: Jjun.
- owner lane temporarily touched: Dagyeom Character presentation.
- purpose: immediate obstacle clipping mitigation before final Core obstacle authority normalization.
- later #123 replaced presentation-derived obstacle truth with Core/World-owned obstacle read models.
- Character ownership returned to Dagyeom.

### ASSIST_LOCK-UI-OBSERVER-DATA-1 — RELEASED
- requester/implementer: Jjun.
- PR #112 merged as `27ba0aa147fc38ad05cf388e9390dd2dcaccdf30`.
- delivered authoritative Needs / Personality / Traits / Preferences / Skills / Relationships / Family / Knowledge/Gear detail data.
- Observer ownership returned to Dagyeom.

### ASSIST_LOCK-UI-CAMERA-1 — RELEASED
- requester/implementer: Jjun.
- PR #109 merged as `47f75d2cb7fd93b26b8b2082bf9f30cfafe67bed`.
- delivered PC and Android Observer camera input routing.
- UI ownership returned to Dagyeom.

## Open Integration Requests

**None.**

Open a new Integration Request only when one owner actually needs another owner to change a file/API/config outside the requester's lane and no active scoped Assist Lock covers it.

## Recently resolved Integration Requests

### IR-D — typed Context Action consumer — RESOLVED
- provider #103, spatial follow-up #111, completion unification #116 merged.
- social presentation follow-up completed in #117.
- Character Presentation consumes Core action/target truth and cannot bypass ACK.

### IR-B — character facing / backwards-walk issue — RESOLVED
- PR #102 corrected Quaternius visual forward axis and merged as `d3c87996499b353c742ce97fd90978633b9b7ca0`.

### IR-A — WorldPresentation owner path — RESOLVED
- `Source/LifeLens/WorldPresentation/**` established as Dagyeom-owned presentation path.
- presentation consumes authoritative read contracts only.

### IR-C — production map + observer framing — RESOLVED
- production map `/Game/Maps/LifeLensWorld` supplied by Dagyeom lane.
- default/startup map and observer camera integration completed in PR #96.

## Integration Request rule

A new request must contain:
- requester / needed owner;
- exact file/API/config needed;
- why the existing contract is insufficient;
- target branch/PR or asset path;
- whether it blocks the current milestone.

Shared/config change alone does not mechanically require cross-owner review. Request review when the change materially touches another owner's behavior, a cross-owner contract, or genuinely needs visual/content validation.
