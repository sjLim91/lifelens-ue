# LifeLens · Field Visual Rebuild V3 — Original Art & Renderer Architecture

Date: 2026-11-23. Supersedes F2-b–F2-f patch-by-patch illustration fixes. This changes **field presentation only**, not the independent simulated world.

## Core visual contract (fixed)

- User reference screenshots are **only visual-quality and ecological-layout references**. Do not introduce the reference game's RPG content, monsters, dungeons, quests, spider regions, characters, or named locations.
- Preserve original deterministic Core resident/animal identities, locations, interaction intent, facility kind/position, time/weather, forest coverage, observed water and elevation.
- Do not claim that merely passing GitHub CI means screenshot-level aesthetic approval.

## Actual replacement scope

1. **All 20 local field SVG assets replaced**: pine, cedar, oak, bush, rock, cliff, flowers, hare, deer, both citizen fallbacks, hut, fire, sleeping, workbench, store, furnace, farm, site and settlement flag. Original richly layered local vector illustrations with unified earthy forest palette, no external asset dependencies.
2. **Terrain renderer split**: `field-terrain-compositor.ts` blends authoritative neighboring Core chunk palettes with seeded continuous pigment into reusable 80×80 texture atlases; paint single contiguous ground surfaces with Core-elevation-derived relief shading and authentic shoreline highlights. No single-color rectangular tiles and no invented Core water.
3. **Occupied land painter**: `field-settlement-illustration.ts` overlays soft, transparent ground wear *only at real facility locations*. It is atmosphere, not new infrastructure or roads. Actual facilities are not moved to arbitrarily prettified spots.
4. **Resident presentation rebuilt**: layered clothing, softer proportions and distinct directional/action poses; sleeping, walking, gathering, crafting, tending and needs interactions remain backed by the existing Core presentation directive. Footpoint/picking remains authoritative.
5. **Existing wildlife, forest density, season/weather, Core persistence remain intact**. The field sprite atlas still loads exactly 20 local assets and retains bounded color variant caching.
6. **Robust Canvas fallback**: if DOM/offscreen texture API is unavailable, retain the validated Core-height ground mesh. Resource-count, picking and resident behavior still follow the same Core snapshot.

## Performance constraints

- Cache at most 112 lightweight 80×80 RGBA ground canvases, invalidated by observed neighborhood signature.
- Terrain in 4×4 elevation-aware bitmap slices, zero new WebGL contexts.
- Existing 384-entry seeded foliage cache, bounded read-only sprite size and watercolor weather layer retained.
- No paid API, model or runtime service.

## What is not falsely marked complete

- This first integrated source/art replacement has automated structural, causality and type/build validation, not an independent artist review.
- Real browser screenshots at wide and close zoom must be checked against the user's actual images. Before marking reference-quality **accepted**, verify Core-grounded biome/shorelines, character/bed interaction and mobile frame time with visual evidence.
- Physical device QA remains excluded at user request.
