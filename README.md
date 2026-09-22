# LifeLens

LifeLens is an autonomous life, society, and world observation simulation built around one deterministic simulation authority and multiple observer clients.

## Architecture

**LifeLensCore (C++)** owns simulation truth: WorldSeed, simulation time, residents, identity, needs, relationships, life events, and environment data.

**Unreal client** remains the high-end native path for Unreal-specific rendering, animation, and Android/Windows/macOS experiments.

**Web Observer** is a first-class fast-iteration and cross-device observer client using React/TypeScript, Three.js, and the same Core compiled to WASM. It must never maintain a separate fake simulation.

## Repository structure

`web/` is the canonical development source for LifeLens Web Observer.

`docs/web/LIFELENS_WEB_MASTER_SPEC.md` defines the browser architecture, rendering direction, reliability rules, asset policy, performance targets, and migration roadmap.

`docs/web/LIFELENS_WEB_WORKING_STATE.md` tracks current production state, un-deployed work, known debt, and next implementation order.

## Simulation direction

A new world begins from a deterministic seed contract with two male and two female adult founders. Stable identity and persistent life state are Core concerns rather than renderer concerns.

The design expands through needs, emotion, personality, memory/belief, relationships, goals/actions, conversations/conflicts, dating/marriage/family, pregnancy/birth, aging/death, generations, settlements, society/factions, and an increasingly Earth-like physical environment.

## Development policy

GitHub is the source of truth.

Web code may be developed, refactored, tested, and documented without production deployment. AppDeploy deployment is a separate explicit action and is performed only when the user directly requests deployment.

See [LifeLens Web Observer Master Spec](docs/web/LIFELENS_WEB_MASTER_SPEC.md).
