# World Obstacle Collision v1

## Problem

Generated trees and rocks are rendered by `ALLWorldPresentationActor` as HISM dressing with collision disabled. Residents move toward Core-authoritative targets using swept actor movement, so the visible character capsule can pass straight through those solid-looking props.

## Contract

Core/World remains the only authority for actions, targets, resources and outcomes. This milestone does **not** create new Core obstacles or rewrite Core positions.

UE presentation may locally alter only the route used to visually reach an already-authoritative target so the character body does not clip through solid-looking dressing.

## Collision policy

- Tree trunks: blocking.
- Non-trivial rocks/boulders: blocking.
- Explicit `Pebble_*` meshes: always non-blocking even when presentation scale makes an individual instance larger.
- Other genuinely tiny rock meshes: non-blocking by footprint threshold.
- Shrubs and grass: non-blocking.
- Collision uses hidden, query-only HISM cube proxies rather than changing the render HISM collision setup.
- Proxies block `ECC_Pawn` only and do not affect navigation generation.
- Proxy transforms are derived from the actual generated presentation instances and source-mesh bounds, so they follow the visible props without modifying the presentation source.

## Movement policy

`ALLResidentCharacter` keeps the existing authoritative movement target and the existing `VInterpConstantTo` constant-speed frame step. On a blocking sweep:

1. move forward only as far as the blocking sweep allows;
2. subtract that actual forward travel from the current frame's movement budget;
3. project the remaining movement onto the obstacle surface;
4. spend only the **unconsumed** frame-distance budget on the slide/side-step;
5. for a head-on/degenerate hit, use a resident-stable left/right tangent so the actor does not oscillate each frame;
6. face the direction actually travelled, not the obstructed target direction.

Obstacle avoidance must never grant extra movement distance. The route may become longer, but target selection and action completion still use the existing Core/physical ACK contract.

## Civilization target interaction

#111 makes Gather/Store positions authoritative Core target loci. A blocking visual proxy may make the exact centre physically unreachable by the character capsule. Therefore Character Presentation must:

- approach the same authoritative target locus;
- stop within a truthful interaction radius outside the blocker when needed;
- face/interact with the same target;
- never move, replace or invent the Core target coordinate just to satisfy presentation movement.

This interaction-radius rule is a consumer requirement for IR-D / Context Motion and does not change Core spatial authority.

## Ownership

- Collision proxy actor: Jjun-owned `Source/LifeLens/World/**`.
- Runtime spawn: Jjun-owned `Source/LifeLens/Core/LLLifeLensGameMode.cpp`.
- Character steering change: temporary assist lock on Dagyeom-owned `Source/LifeLens/Characters/LLResidentCharacter.cpp`.
- `Source/LifeLens/WorldPresentation/**` is intentionally unchanged.

## Validation

Required before merge:

- Preflight PASS on the final PR head.
- Unreal Linux Compile PASS on the final PR head, including UHT/UBT for the new collision proxy actor.
- Core Tests are **not path-triggered** by this milestone because `Source/LifeLensCore/**` is unchanged; Core authority must instead remain unchanged by diff review.
- Diff confirms no `Source/LifeLensCore/**` authority changes and no WorldPresentation render/culling source changes.
- PIE/device visual QA should verify residents route around tree trunks and large rocks without becoming stuck or visibly oscillating.
- PIE/device QA should verify no obstacle-avoidance speed burst and that `Pebble_*`, shrubs and grass remain traversable.
