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


## C6 extension — settlement network

DU-01의 `settlement-local authority`는 2026-10-02 C6에서 다중 생활권으로 확장되었다.

- 자원 희소성과 실제 이동 비용이 주민별 migration pressure를 만든다.
- 먼 곳의 생활권은 기존 마을의 저장고·화덕·화로 존재 여부에 종속되지 않고 자체 인프라를 구축할 수 있다.
- 시설과 저장소는 서비스 반경 기반의 공간 군집으로 관찰되며, 실제 runtime 주민 위치가 정착지 소속을 결정한다.
- 정착지 간 물류는 remote transfer를 허용하지 않는다.
- 원거리 교역은 주민이 상대 정착지로 실제 이동한 뒤에만 Inventory 교환을 수행하고, 이후 출발 생활권으로 실제 귀환한다.
- 반복된 원거리 교역 사실은 영속적인 교역로 관찰 근거가 된다.

따라서 DU-01의 인과 사슬은 다중 정착지에서도 동일하다.

`local pressure -> local decision -> real movement/logistics -> spatial interaction -> authoritative result -> observer consequence`

한 정착지의 존재나 시설이 월드 전체의 singleton 권위가 되어 다른 생활권의 자율 발전을 막아서는 안 된다.
