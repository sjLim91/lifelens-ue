# LifeLens

Unreal-native autonomous life and society observation simulation.

## M1 resident simulation core

This package establishes the first stable runtime core:

- Android-first Unreal C++ project skeleton.
- Deterministic `WorldSeed` simulation.
- Every **New Game** creates exactly **2 male + 2 female adults**.
- Names, GUIDs, age, personality, traits, skills, preferences, and background are generated only at New Game.
- Save/load persists the same four residents and their IDs.
- Relationship records are initialized independently from resident generation so dating/partner/marriage/family logic can evolve without replacing the population model.
- Pregnancy/parent/child fields are already present in the persistent resident schema.
- Observation state is isolated from the simulation so the main view can remain clean; detailed panels can subscribe only when a resident is selected.
- Basic need decay and simulation minutes are present as the first testable time loop.

## Intentionally not copied from LOCAL OBSERVER

No HTML/JS/Vercel runtime, web game state, or legacy renderer is included. LOCAL OBSERVER remains reference material only.

## Next implementation slice

1. Execute Utility-AI intents through navigation/StateTree tasks.
2. Spawn and bind the four residents into a smoke-test world.
3. Social encounters and relationship progression.
4. Dating -> partner -> engagement -> marriage -> pregnancy -> birth -> child aging.
5. Minimal observer HUD: world summary by default, resident detail panel on selection.
6. Android smoke map, then Unreal toolchain-backed APK packaging after the fast preflight stays green.

## Build note

The source structure targets Unreal Engine 5.6. An Unreal Engine toolchain is required to run UHT/UBT, cook, and package APKs; this bootstrap package itself does not claim a successful APK build.
