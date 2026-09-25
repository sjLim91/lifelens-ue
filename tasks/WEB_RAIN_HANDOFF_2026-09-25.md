# Web precipitation repair — 2026-09-25

- Base: `69dec0a7171b910442b9da784b7ac1f972379086`.
- Branch: `work/web-rain-20260925`.
- User requested the rain be corrected, with prior authorization to merge validated changes.
- Scope: weather-layer.ts, weather regression tests, existing web CI; no Core/actor/camera authority changes.

## Findings and repair

The old salted generator produced near-identical x/y/z values: rain occupied a diagonal sheet rather than a volume. Independent avalanche hashing distributes rain and snow across the observation volume. Rain wraps with its vertical overshoot preserved instead of snapping back to fixed seeded launch phases. Screen streaks follow the camera-projected particle velocity; CSS-pixel width/length is preserved at DPR 1/2 without world-space rods.

Explicit Core None/zero precipitation and unavailable observations stop particles; summary fallback is restricted to missing type fields. Positive weak precipitation retains the presentation visibility floor. Weather generation and consequences remain exclusively Core-owned.

## Validation

- Seven executable regression checks: light-rain ground coverage, long-run vertical/spatial coverage, dry/unavailable transitions, legacy summary compatibility, renderer DPR/aspect updates, drift/velocity agreement.
- Typecheck and production build pass; existing large bundle warning remains.
- Browser reached the live GitHub Pages app, but WebGL context creation failed in this cloud browser. No runtime visual acceptance is claimed. Real-device checks still needed: rain/storm at near/far zoom, camera rotation, mobile DPR, wet-to-dry transition.
- Product PR/merge and deployment result will be recorded at closeout.

## Integration

- PR #448 merged as `e11119280ae122879c38a0e13d496124679b9064` after all four exact-head GitHub checks passed (runtime-resilience, structural-preflight, two typecheck runs).
- Scope released. Cloud WebGL was disabled; real-device visual acceptance remains open.
