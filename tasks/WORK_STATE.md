# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Canonical roadmap: `docs/DEVELOPMENT_MILESTONES.md`.
> Long-range civilization direction: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`.
> Ownership / locks / IR: `tasks/TEAM_BOARD.md`.
> Point-in-time audit: `docs/INTEGRATED_AUDIT_2026-09-17.md`.

Last reconciled: **2026-09-17 KST during PR #132 exact-head closeout and open-ended civilization roadmap reconciliation**.

## Current main baseline

Current `main`: `94b27f6a44310712d55a6e95bbcbf89ed6ba41ca` (post-#131 documentation sync; #132 not merged yet at this checkpoint).

Recent completed checkpoints:
- #115 Lifecycle Core Correctness v1 — DONE.
- #116 Action Completion Unification v1 — DONE.
- #117 Social Communication & Localization v1 — DONE.
- #118~#122 Facilities / Tools / Fire / Furnace / Copper — DONE.
- #123 World / Facility / Obstacle Authority Normalization — DONE.
- #124 Legacy Authority Removal v1 — DONE.
- #125 Lifecycle Presentation v1 — DONE.
- #126 Social speech-bubble readability — DONE.
- #127 Integrated audit / canonical roadmap reconciliation — DONE.
- #128 Mac editor `-Wshadow` unblocker — DONE.
- #130 Knowledge Transmission Spatial Authority v1 — DONE.
- #131 post-#130 canonical documentation sync — DONE.

Authority rule:

> Core / World owns simulation truth. UI / Character / WorldPresentation consumes authoritative contracts and must not invent outcomes.

---

## P0 ACTIVE — PR #132 Emotion Runtime Integration v1 closeout

Owner: Jjun Core provider.
PR: #132.
Actual branch: `jjun/emotion-runtime-integration-v1-20260917`.
Current exact head: `0327afb58caed550b8bb12f046b2ec6738157ef5`.

### Delivered
- Need pressure drives bounded emotional pressure.
- real Need resolution drives relief/joy.
- repeated authoritative failure drives anger/anxiety/fear.
- civilization success/discovery/failure feeds emotion.
- existing social/romantic/family/loss/environment emotion inputs remain authoritative.
- physical utility emotion adjustment is bounded to +/-8%, preserving survival priority.
- deterministic/persistence coverage exists.

### Closeout gap found and fixed

Initial #132 head passed Core/Preflight/Unreal CI, but code review found one production-path omission:

- standalone Core `Use/EmergencyUse` applied Need-resolution emotion;
- production `completeExternalPhysicalAction()` reduced Needs but did not apply equivalent relief.

The branch now:
- snapshots Needs before external physical completion;
- applies `applyNeedResolutionEmotion()` after authoritative external outcome;
- extends `test_external_physical_execution.cpp` to assert relief increases after production external completion.

### Failure/timeout semantics

- authoritative lived simulation failure may change emotion;
- stale/wrong token, invalid/far ACK, restore-time pending drop and presentation/control-plane timeout do **not** create emotional failure by themselves;
- renderer/bridge latency must never fabricate character emotion;
- a future Core rule may explicitly model an unreachable/abandoned attempt as a lived failure and then emit it authoritatively.

### Current exact-head validation
- Preflight — PASS.
- Core Tests — PASS.
- Unreal Linux Compile — IN PROGRESS at this checkpoint.

Merge rule:
> Do not mark #132 DONE until all exact-head gates on `0327afb...` pass.

After merge, reconcile this section to DONE and record the squash merge SHA.

---

## Dagyeom active/stale work

### PR #100 — World Readability Envelope — ACTIVE / CURRENT-MAIN REFRESH REQUIRED

Branch: `dagyeom/world-visual-readability-envelope`.
Last known head: `62b191a6afb2c0c5ae32ddbb1e0ae7876ae00556`.

Live repository check now reports the PR as **not mergeable against current main**.
Therefore the older work-state text saying only “Mac PIE then merge” is obsolete.

Required sequence:
1. Dagyeom owner refreshes/reconstructs the still-needed WorldPresentation changes on current main.
2. resolve conflicts without importing stale Core/authority code.
3. fresh exact-head Preflight + Unreal Linux Compile.
4. Mac PIE visual confirmation.
5. merge only if acceptable.

Do not merge just to clear the queue.

### PR #98 — STALE / SELECTIVE SALVAGE ONLY

Do **not** merge this old branch wholesale.
Still-useful ideas may be reimplemented on latest main:
- Observer panel readability tuning;
- QA-only resident framing helpers;
- true Observer Detail scrolling remains a current gap.

---

## Administrative state

- Current Assist Lock count — 0 at this checkpoint.
- Open Integration Requests — 0 at this checkpoint.
- Android Gate B — **PAUSED BY USER**; keep it in the roadmap but do not run Android builds until explicitly resumed.
- old PR #129 remains superseded; do not merge independently.
- historical `INTEGRATED_AUDIT_2026-09-17.md` remains point-in-time evidence and is not rewritten to impersonate current state.

---

# Unified priority order

The roadmap is one integrated plan with parallel ownership lanes. Dagyeom presentation work does not need to block unrelated Jjun Core provider work unless an actual IR/blocker appears.

## P0 — Close current work safely

1. **#132 Emotion Runtime Integration v1 exact-head closeout — Jjun**
   - final Unreal Linux Compile;
   - ready/squash merge only after full exact-head green;
   - post-merge canonical state reconciliation.

2. **#100 World Readability refresh/closeout — Dagyeom**
   - current-main refresh;
   - fresh CI;
   - Mac PIE;
   - merge decision.

## P1 — Make existing simulation truth readable and believable

3. **Character Context Motion v2 — Dagyeom**
   - PickUp/Carry, Dig/Strike, Craft/Build/Fire/Smelt, Parenting, sanitation, teaching/social;
   - sitting/lying only with real compatible facilities;
   - authoritative target facing/gaze.

4. **Observer Readability + Real Scrolling — Dagyeom**
   - real vertical scrolling;
   - current-main readability work;
   - selective #98 salvage only.

5. **Lifecycle Event Presentation v2 — Dagyeom**
   - pregnancy/birth/growth/death/history visibility;
   - deceased/history inspection from Core truth.

## P2 — Build the missing settlement substrate

6. **Settlement & Subsistence Foundation — Jjun -> Dagyeom**
   - SleepingPlace;
   - Shelter;
   - WorkSurface;
   - actual sleep/comfort/work effects;
   - cultivation/agriculture foundation;
   - renewable food production/storage constraints;
   - Tin/Bronze when prerequisites exist;
   - no free infrastructure or era timer.

Why before more advanced technology:
> Current civilization can reach Copper Smelting but still lacks the durable food/housing basis required for centuries of population growth.

## P3 — Make centuries/millennia technically possible

7. **Long-Run Scale & Cleanup — Jjun**
   - catch-up frame budget;
   - population CPU/memory stability;
   - residue/HISM profiling;
   - snapshot cost/legacy cleanup;
   - multi-generation / multi-century deterministic regression.

Why moved up:
> Future civilization features are wasted if long simulation collapses under its own runtime cost.

## P4 — Remove the hardcoded civilization ceiling

8. **Open-Ended Civilization Framework v1 — Jjun**
   - Knowledge / Capability / Technology / CivilizationTransformation separation;
   - stable/versioned Capability and Technology identity;
   - map existing primitive Techniques into graph;
   - prerequisites/effects;
   - pressure-driven experiment/research;
   - discovery/reproduction/adoption/diffusion;
   - loss/rediscovery;
   - deterministic Save/Load + Observer read model.

Canonical direction: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`.

## P5 — Population resilience and social scaling

9. **Health / Disease / Population Resilience — Jjun -> Dagyeom**
   - infection/pathogens;
   - contaminated water/soil;
   - sanitation health effects;
   - illness/recovery;
   - premature/accident/environment mortality;
   - foundation for later medicine/biotech.

10. **Education / Recording / Specialization / Economy / Institutions — Jjun -> Dagyeom**
    - apprenticeship/education;
    - persistent records/knowledge scaling;
    - roles/professions;
    - production/demand/resource distribution;
    - exchange/trade foundation;
    - generic institutions/coordination;
    - technology adoption/acceptance.

Why split from the old final `economy/society` bucket:
> Industrial, digital and AI civilizations require education, specialization, allocation and institutions before those technologies can emerge naturally.

11. **Migration / Multiple Settlements / Trade Networks — Jjun**
    - carrying-capacity pressure;
    - exploration/migration;
    - settlement founding/abandonment;
    - local knowledge/capability differences;
    - inter-settlement trade/cooperation/conflict foundations.

## P6 — Historical to modern capability content

12. **Advanced Metallurgy / Urban / Scientific Accumulation**
13. **Mechanical / Industrial Civilization**
14. **Electrical / Chemical / Modern Infrastructure**
15. **Digital / Network / Information Civilization**

These are capability/technology content packs on top of the open-ended framework, **not forced era level-ups**.

## P7 — Future civilization

16. **AI / Robotics / Advanced Automation**
17. **Advanced Energy / Materials / Biotechnology**
18. **Planetary / Space / Interplanetary Civilization**
19. **Open Future / Unknown Civilization**
   - compositional innovation;
   - concrete simulation effects;
   - deterministic stable identity;
   - acceleration/stagnation/collapse/rediscovery all possible.

North Star:
> LifeLens should be able to continue beyond present-day humanity without requiring the developer to pre-script one mandatory future.

## PAUSED gate

20. **Android Gate B — PAUSED** until explicit user resume.

---

# Previous list vs new direction

| Previous task | Disposition |
|---|---|
| #100 closeout | KEEP, but live status corrected to current-main refresh required |
| Character Context Motion | KEEP HIGH |
| Emotion Runtime | #132 CLOSEOUT now |
| Observer scrolling/readability | KEEP HIGH |
| Lifecycle Presentation v2 | KEEP HIGH |
| Civilization Phase 2 | EXPAND into Settlement & Subsistence Foundation |
| Cleanup/performance | MOVE UP before open-ended/deep civilization scaling |
| Health/disease | KEEP and formalize before advanced medicine |
| Migration/economy/society | SPLIT; education/economy/institutions move earlier, migration follows |
| Open-Ended Civilization Framework | NEW P4 architecture milestone |
| Historical -> modern capability packs | NEW structured expansion |
| AI/energy/biotech/space | NEW explicit long-range path |
| Unknown future | NEW canonical North Star |

---

## Cross-lane coordination checkpoints

Jjun checks Dagyeom-side open PRs, new comments/review requests and `TEAM_BOARD` Integration Requests:
- before starting a new Jjun work unit;
- after completing or merging a Jjun work unit;
- when entering a long Unreal/CI wait;
- when returning to handle a long Unreal/CI result;
- before a main-changing merge or rebase.

A new Dagyeom blocker is triaged before unrelated follow-up work. This is independent from build polling: never repeatedly query or restart a healthy long build merely to perform coordination checks.

---

## Open audit / implementation gaps

### P1 presentation/integration
- Character Context Motion remains generic for many concrete actions;
- Observer Detail overflow is not true scrolling;
- lifecycle birth/growth/death/history needs richer observer presentation;
- #100 needs refresh against current main.

### P2 settlement/long-run
- no full renewable agriculture/subsistence system yet;
- SleepingPlace/Shelter/WorkSurface effects remain incomplete;
- environmental residue presentation refresh cost needs profiling;
- long catch-up stepping needs per-frame budget;
- population scaling needs long-run profiling;
- `SimulationSnapshotCodecLegacy.cpp` remains dead/uncompiled source;
- structural Preflight still contains compatibility markers for removed legacy surfaces.

### P3 architecture/depth
- current technology representation is adequate for primitive content but not yet an open-ended Capability/Technology graph;
- health/disease/pathogens incomplete;
- education/specialization/economy/institutions incomplete;
- migration/multiple settlements/trade incomplete;
- modern/future/open-future content not yet implemented.

Closed defects:
- remote knowledge witness/teaching telepathy — closed by #130.
- production external physical Need-resolution emotion omission — fixed on #132 closeout head, awaiting final exact-head merge gate.

---

## Validation strategy while Android is paused

Core changes:
- Core Tests;
- deterministic harness;
- Preflight.

Unreal C++ changes:
- Preflight;
- Unreal Linux Compile.

Long-run civilization changes:
- deterministic multi-generation/multi-century headless scenarios in addition to feature-specific tests.

Docs-only changes:
- do not start a heavy Unreal compile solely for documentation.

Exact-head validation remains mandatory before functional merge.
