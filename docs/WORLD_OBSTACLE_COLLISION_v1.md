# World Obstacle Collision v1

## Problem

Generated trees and rocks are rendered by `ALLWorldPresentationActor` as HISM dressing with collision disabled. Residents move toward Core-authoritative targets using swept actor movement, so the visible character capsule can pass straight through those solid-looking props.

## Contract

Core/World remains the only authority for actions, targets, resources and outcomes. This milestone does **not** create new Core obstacles or rewrite Core positions.

UE presentation may locally alter only the route used to visually reach an already-authoritative target so the character body does not clip through solid-looking dressing.

## Collision policy

- Tree trunks: blocking.
- Non-trivial rocks/boulders: blocking.
- Small pebbles: non-blocking.
- Shrubs and grass: non-blocking.
- Collision uses hidden, query-only HISM cube proxies rather than changing the render HISM collision setup.
- Proxies block `ECC_Pawn` only and do not affect navigation generation.
- Proxy transforms are derived from the actual generated presentation instances and source-mesh bounds, so they follow the visible props without modifying the presentation source.

## Movement policy

`ALLResidentCharacter` keeps the existing authoritative movement target. On a blocking sweep:

1. project the remaining movement onto the obstacle surface;
2. slide along that surface;
3. for a head-on/degenerate hit, use a resident-stable left/right tangent so the actor does not oscillate each frame;
4. face the direction actually travelled, not the obstructed target direction.

The route may become longer, but target selection and action completion still use the existing Core/physical ACK contract.

## Ownership

- Collision proxy actor: Jjun-owned `Source/LifeLens/World/**`.
- Runtime spawn: Jjun-owned `Source/LifeLens/Core/LLLifeLensGameMode.cpp`.
- Character steering change: temporary assist lock on Dagyeom-owned `Source/LifeLens/Characters/LLResidentCharacter.cpp`.
- `Source/LifeLens/WorldPresentation/**` is intentionally unchanged.

## Validation

Required before merge:

- Preflight PASS.
- Core Tests PASS.
- Unreal Linux Compile PASS, including UHT/UBT for the new collision proxy actor.
- Diff confirms no WorldPresentation render/culling source changes.
- PIE/device visual QA should verify residents route around tree trunks and large rocks without becoming stuck or visibly oscillating.
