# LifeLens Project Status

Date: 2026-09-30  
Active architecture: **LifeLensCore -> WASM -> Web Observer**  
Current validated main: `df839012177bc8f4a0bfd59e30cbf8e5d29d4129` (#536)

## Current autonomy follow-through — resident strategy differentiation

- `746a488` / D-026 completed the first development-choice split: cultivation/construction/crafting utility reacts to resident disposition and lived technique experience instead of collapsing identical affordances into identical choices.
- `9aef4a1` / #534 / D-027 completed non-critical Eat autonomy. The same moderate Hunger and same carried PlantFood can produce different Eat timing from resident disposition, while actual freshness raises consumption value as spoilage approaches.
- `df83901` / #536 / D-028 completed shared-provision allocation: the final locally accessible PlantFood now has an explicit retention-vs-cultivation opportunity cost, nearby real Storage removes that false last-unit scarcity, and Store remains a real-surplus reversible path.
- Identical Hunger/food/tool/knowledge/plot affordances can therefore deterministically split into Eat-oriented retention versus Plant investment without fixed jobs or random role assignment.
- Urgent/critical Hunger keeps authoritative survival priority. Eat/Plant/Store all operate only on real Core inventory/storage authority; SmartObjects/Web never fabricate provisions.
- #536 post-merge Core Tests, deterministic harness, Preflight, Web WASM, Runtime Release, Pages Preview and External Preview Probe all passed.
- Next autonomy target: authoritative decision rationale observation. Core should expose the factual inputs/reason for the selected resident action so Web can explain differences without inferring motives or fabricating a rationale.

## Active development unit — DU-01 Living Settlement

- Canonical unit: `docs/DU01_LIVING_SETTLEMENT.md`.
- Baseline #521 connects shared settlement storage to physical construction/restoration logistics.
- DU-01 now integrates facility reuse/repair/build choice, settlement-local material causality, truthful Web observation, and regression protection as one milestone-sized PR.
- Development uses targeted tests while iterating; full Core/WASM/Web/Preflight runs once at the merge gate unless a boundary-changing failure requires earlier full validation.
- Completion is runtime/causality based: accelerated observation must show real pressure -> logistics -> work -> use without duplicate facilities, fake water, teleport work, or browser-authored outcomes.

## Runtime decision

The Unreal client has been retired from active development and is being removed from `main`.

Reasons:
- visual/product verification now happens through the Web Observer;
- Unreal UHT/UBT and platform pipelines added maintenance/CI cost without being used for product review;
- LifeLensCore is already platform-neutral and the Web client consumes the same authority through WASM.

The final Unreal state is preserved at `archive/unreal-final-20260929`.

PR #497, which only repaired the Unreal external physical sleep path, was closed without merge. Timed sleep for the authoritative headless/Web path already landed through #494.

## Current authority

- LifeLensCore C++ is simulation truth.
- Web uses Core via WASM.
- React/TypeScript/Three.js is the primary observer/presentation client.
- Browser presentation must not fabricate resources, actions, facilities, geography, relationships, or future state.

## Recent P0/P1 baseline

- #491 closer mobile zoom / readable resident labels.
- #492 ruined facility restoration before wasteful rebuilding.
- #493 provisions required before consumption.
- #494 real bedding travel, timed sleep, paid washing.
- #495 canonical GitHub Pages preview probe.
- #496 spatial causality guard for gather/store/retrieve/facility work.
- #503 canonical observation speed rebased to the former 4x pace, with real
  portable/natural-water authority for washing.
- #504 Web hydrology visibility aligned with Core direct-water authority;
  drinking/washing interaction waits for visible arrival and carried-water
  actions are labeled separately from natural-water use.
- #505 resident locomotion/action animation was unified with the global
  observer speed authority instead of an independent visual cap.
- #506 timed Core sleep is presented as a truthful lying/resting posture only
  after authoritative arrival; movement, collision and Need authority stay in
  Core.
- #507 Observer feed now exposes factual Need -> travel -> interaction
  milestones for Eat/Drink/Sleep/UseToilet/Wash without replay/spam.
- #509 high-speed observer hot path split from heavyweight snapshots: resident
  movement/Needs/presentation use a compact runtime DTO while relationships,
  memories, beliefs, family and civilization detail refresh at a lower cadence;
  deterministic terrain is cached until the observer window changes.
- #510 dynamic human-trace refresh avoids rescanning static terrain while still
  rebuilding grass/rocks/trees once when authoritative facility footprints
  actually change.
- #511 social observation now exposes factual approach -> interaction phases for
  Approach/Comfort/Repair/Avoid plus KnowledgeTeaching and Parenting, with exact
  Core social outcomes taking priority over duplicate inferred activity rows.
- #513 retires 16x observation because it can monopolize the browser main
  thread; selectable speeds are pause/1x/4x, with stale higher requests capped
  at 4x. Sleeping residents now freeze skeletal animation after authoritative
  arrival, center the feet-pivoted model around the sleep point, clear the
  terrain/sleeping-place surface, and sample sloped outdoor ground to prevent
  repeated bobbing or burial.
- #515 exact social outcomes can show the actual directional relationship
  change produced by Core (recipient -> actor), such as trust/bond/conflict
  percentage-point deltas. If multiple same-direction events are batched in one
  heavy refresh window, per-event deltas are omitted instead of fabricated.
- #517 observation events can carry factual world focus coordinates. Clicking a
  feed event explicitly moves the camera to the event/target location, selects
  the relevant resident when one exists, and keeps a visible "현장 관찰" banner
  without auto-moving the camera when events merely arrive. Facility/resource/
  sanitation events use their authoritative grid positions; physical actions
  use the Core presentation target grid where available.
- #519 live social behavior is differentiated directly in the world using only
  authoritative presentation state. Approach/Comfort/Repair/Avoid, teaching and
  parenting now use natural Korean action cues with real target/action context;
  Approach/Comfort/Repair, teaching and parenting connectors have distinct
  presentation-contract styling. No new social outcome or animation is inferred.

## Validation target after Unreal removal

Required:
- LifeLens Core Tests
- deterministic harness
- LifeLens Web WASM
- Web Typecheck/build
- Web Runtime Release
- GitHub Pages Preview
- External Preview Probe
- Web/Core structural Preflight

No Unreal compile or Android Unreal APK gate remains on active `main`.

## Canonical documents

- Runtime architecture: `docs/RUNTIME_ARCHITECTURE_WEB_CORE_v1.md`
- Product/domain master spec: `docs/LIFELENS_SPEC_v1.1.md` (engine-specific implementation sections are historical where they conflict with the runtime architecture document)
- Web architecture: `docs/WEB_CLIENT_ARCHITECTURE_v1.md`
- Web working state: `docs/web/LIFELENS_WEB_WORKING_STATE.md`
- World architecture: `docs/WORLD_ARCHITECTURE_v2.md`
- Open-ended civilization: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`
