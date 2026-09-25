# Character scalp regression repair — 2026-09-25

- User screenshot shows exposed scalp with large dark collar-like hair geometry after #449.
- Base: dc293c3d039e22da67744f4543fa462c8dff3cfd.
- Branch: work/web-scalp-fix-20260925.

## Cause and correction

The procedural hair spheres used a fixed normalized height and radius. Head attachment preserved that guessed fit rather than fitting the actual skinned head. Bone-follow tests passed but could not establish silhouette correctness.

Remove the spherical cap/bun/back proxies entirely. Apply close-cropped hair color only to the upper portion of the actual Head-weighted body vertices. Head bounds come from the pinned model in its bind pose; face/neck vertices remain skin. Hair now uses the same skinning as the head with no independent object or transform. Darken the untextured eye surfaces to avoid the previous glaring white appearance.

## Evidence / limits

- Five character tests pass: 1,000 valid profiles, identity stability, shared-geometry isolation, actual scalp selection/no extra geometry, eye/brow colors.
- Actual GLTFLoader test: 145 full-color scalp vertices on the sampled profile, no rigid hair proxy, 7,281 body color vertices; Walk_Loop still animates the head.
- CPU triangle-rendered close-up of the actual posed GLB inspected: scalp covered, face exposed, collar-shaped hair geometry absent. This diagnostic is not a WebGL/device screenshot.
- Typecheck and production build pass. Cloud WebGL remains unavailable; final device appearance still needs confirmation.
- Existing single male body model limitation remains separate; no claim of complete character fidelity.

## 2026-09-26 integration

- PR #451 merged as `d8fcd616a344833a0da13707c80be18d8c8638c4` after all three final-head checks passed: structural-preflight, typecheck, runtime-resilience (49 regressions plus build).
- Initial preflight failed because the old structural validator required the removed addResidentHairVariant proxy function. Updated it to require skinned scalp coverage, geometry isolation and absence of spherical hair proxies; gait assertions retained.
- Scope released. Device visual acceptance remains pending.
