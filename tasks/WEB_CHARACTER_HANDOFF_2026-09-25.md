# Web character repair — 2026-09-25

- Base: `69e0cd2fa1e26d723669e75aa855d3f4bff8792f`.
- Branch: `work/web-character-repair-20260925`.
- User requested character corrections and continued execution; standing merge authorization applies.

## Repaired defects

- Appearance hash returned signed values after its final XOR: negative palette indices, invalid hair variants and out-of-range proportions. Normalize unsigned to [0,1).
- Character normalization was one unit high, but only Y received resident height. Scale X/Z by the same height before independent proportion factors.
- Hair was a rigid child of the visual root. Attach the fitted accessory to the actual Head bone preserving the bind-pose transform; skip accessory if the rig has no head.
- Inspected pinned character GLB: SuperHero_Male body, Eyes and Eyebrows all share one Matte jade material. Assign eye/brow colors and separate exposed head/hand skin from garment using actual skeletal weights. Clone per-resident geometry and dispose owned buffers.

## Validation

- Six automated regression cases include 1,000 identity profiles, deterministic reload, head translation/rotation, missing-head behavior, material and geometry isolation, eye/brow colors.
- Actual pinned GLBs parsed through GLTFLoader: 7,281 body color vertices; hair attached to Head; Walk_Loop moves the head; root translation stays zero. Reproduce with `node web/tests/characters/inspect-assets.mjs MODEL_GLB ANIMATION_GLB`.
- Typecheck and production build pass. Existing bundle warning remains.
- No GPU visual acceptance: cloud browser was previously confirmed WebGL-disabled. Real-device gait, hair fit and readability checks remain necessary.

## Boundaries / continuity

- PR #423 action-context lane remains open; its DTO/restMotion changes were not copied or overwritten. resident-world-layer edits are restricted to model proportions and owned-geometry disposal.
- The current web source still uses one male base mesh for both sexes. This repair does not claim separate female mesh or completed character fidelity. A licensed body-variant asset integration remains follow-up.
- Core motion/weather/lifecycle truth unchanged. No new external assets vendored.
- Product merge and exact-head checks recorded in closeout.

## Integration

- PR #449 merged as `9f5fa4653e27f304fe5f8851dfffed9bbde78e53` after all four exact-head GitHub checks passed.
- CI includes 43 checks (six character, seven weather, 30 existing runtime) and typecheck/build.
- Scope released; separate body assets and real-device visual acceptance remain open.
