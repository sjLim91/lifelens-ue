# DU-01 — Living Settlement

Status: **ACTIVE**  
Started: **2026-09-30**  
Baseline: `main@3135e7bb8a45dc4d3ecc785d89273e74b68f4015` (#521)

## Goal

Turn the existing survival/facility/economy pieces into one observable settlement loop:

`lived pressure -> reuse/repair/build decision -> real material logistics -> spatial work -> operational use -> observable consequence`

The world must feel like residents are living and adapting, not spawning facilities from isolated need triggers.

## Delivery model

DU-01 is one milestone-sized PR. Internal commits may be small.

Three lanes may proceed in parallel when their files do not overlap:

- **Lane A — Core / Simulation:** need pressure, facility reuse, repair/build priority, shared logistics, settlement-local authority.
- **Lane B — Web / Observation:** truthful in-world carrying/building/repair/use presentation and cause -> action -> result observer narration.
- **Lane C — QA / Contracts:** targeted regressions during development; full Core/WASM/Web/Preflight once before merge.

Do not create documentation-only PRs for intermediate DU-01 status. Documentation lands in the same development unit.

## Core decision contract

Before creating a new facility, residents must evaluate real alternatives:

1. usable existing facility;
2. damaged facility that can be restored;
3. shared/local stock that can support the required action;
4. only then, a new project when lived local demand remains unmet.

Construction priority must be derived from authoritative facts such as:

- survival urgency;
- number of locally affected residents;
- usable capacity already available;
- facility condition;
- travel cost;
- available/shared materials;
- required labor;
- viable fallback alternatives.

No browser-side priority or completion authority is allowed.

## Resource causality

- Drink/Wash require carried water or a Core-usable real freshwater source.
- Construction/repair require material physically owned by residents or shared local storage.
- Material delivery remains spatial; no remote withdrawal, delivery or completion.
- Existing resources/facilities must be reused before wasteful duplication when they satisfy the need.
- Settlement logistics must stay anchored to lived resident activity, not initial spawn coordinates.

## Observer contract

Web displays Core truth and does not invent activity.

For relevant settlement actions the observer should be able to read a sequence such as:

`pressure -> travel -> retrieve/gather -> carry -> arrive -> work -> completed/use`

Examples:

- sleep capacity shortage -> material logistics -> SleepingPlace completion -> resident actually sleeps there;
- ruined facility -> restoration demand -> material retrieval/delivery -> repair -> facility returns to use;
- hygiene/thirst -> real water acquisition -> visible travel/arrival -> actual use.

## Regression guard

DU-01 must preserve and explicitly protect:

- no washing without real water;
- no drinking from invisible/non-Core water;
- no repeated SleepingPlace construction merely because sleep Need rises;
- no remote/teleport gathering, storage, construction or restoration;
- no Web-fabricated facility/action/resource outcomes;
- sleep recovery remains time-based;
- resident movement and action motion remain synchronized with the global simulation speed;
- deterministic same-seed/snapshot continuation.

## Validation strategy

During implementation, run only tests targeted at the changed contract.

Before merge run the full DU gate once:

1. Core configure/build + CTest;
2. deterministic harness;
3. LifeLens Web WASM;
4. Web typecheck/build, including `node tests/observation-feed/run.mjs` as the Observer-feed regression gate;
5. Web/Core structural Preflight.

After merge verify:

1. Web Runtime Release;
2. GitHub Pages Preview;
3. External Preview Probe;
4. `gh-pages/runtime/RUNTIME_COMMIT.txt` equals the merged `main` SHA.

## 2026-09-30 checkpoint — P0 survival causality recovery

- #527 (`d1e73894efbceb54cf4f6031e6061f1316ebdcea`) is merged to `main`.
- Known live freshwater remains a direct Physical Drink/Wash affordance; Civilization Explore is used for Water only when no known live natural source exists.
- Critical hunger/thirst can continue beyond an exhausted ordinary six-chunk exploration envelope without synthesizing provisions.
- Active provision acquisition is kept stable instead of re-running the expensive frontier search every simulated minute.
- The regression that inflated `test_snapshot_codec` from ~2 s to 40.40 s and `test_autonomous_civilization_loop` from 4.42 s to 84.66 s was removed; main #1615 completed all 80 tests in 29.93 s.
- Web Runtime Release, Web Preview, GitHub Pages and External Preview Probe all passed for the merged runtime; `gh-pages/runtime/RUNTIME_COMMIT.txt` matches #527.
- This checkpoint does **not** close DU-01. Long accelerated observation must still demonstrate autonomous provision choice, real acquisition/use travel, rational facility reuse/repair/build and truthful Observer causality before status may become COMPLETE.
## Done means

DU-01 is complete only when a long accelerated observation demonstrates all of the following without fabricated state:

- residents use viable existing facilities before unnecessary duplicates;
- ruined facilities are restored when restoration is the rational/local option;
- storage stock actually feeds construction and restoration;
- local unmet demand can still create new facilities;
- real water causality remains intact;
- settlement activity follows lived resident positions and repeated use;
- Observer feed and world cues expose why the visible change happened;
- no regression in movement, sleep, resource or facility spatial causality.

Graphics-wide polish is intentionally outside DU-01 unless a visual defect prevents truthful observation of the above loop.
