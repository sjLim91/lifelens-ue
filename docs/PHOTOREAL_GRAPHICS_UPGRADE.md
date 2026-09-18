# LifeLens Photoreal Graphics Upgrade — Free Asset Policy

Status: active graphics direction.

## Goal

Move the current visible world away from the stylized/bootstrap look while
preserving Android viability and Core simulation authority.

## Asset policy

Priority order:

1. CC0 / public-domain assets with reproducible source URLs.
2. Free Fab assets only after the exact listing license is verified and the
   asset is acquired through an authorized Epic/Fab account.
3. Existing Quaternius stylized assets are bootstrap/LOD-only; they are not an acceptable hero/local-view production fallback.
4. If a production-quality asset is missing, omit the decorative visual instead of spawning an obvious primitive/placeholder.
5. No paid pack or subscription is required for runtime correctness.

Original downloaded archives/models stay outside Git under
`LL_ASSET_STAGING`. The repository stores import scripts, provenance and
curated imported Unreal assets only.

## First photoreal wave — Poly Haven CC0

Models:
- fir_sapling
- pine_sapling_small
- boulder_01
- tree_stump_01
- shrub_02
- shrub_03
- weed_plant_02
- dead_tree_trunk

Surface references:
- forest_floor
- forrest_ground_01
- mossy_rock

Lighting reference:
- nature_reserve_forest HDRI

The source assets are CC0. The Poly Haven live API is used only by the
acquisition tool and must send the LifeLens-specific User-Agent.

## Quality tiers

### Desktop / PC
- retain up to 2K imported photoreal textures in this first wave.
- baseline local-view meshes must remain within explicit per-asset payload budgets; ultra-heavy hero scans are not silently accepted.
- use higher-detail static meshes and authored PBR materials.
- future PC-only tier may use Nanite/high-detail variants after the universal
  mobile fallback is proven.

### Android
- same semantic world and same asset identity.
- texture streaming mip bias / view-distance / foliage density can be reduced
  by Device Profile.
- static-mesh LOD/HLOD must exist; Nanite is not the only rendering path.
- Core resources and collisions are never removed because a visual LOD drops.

## Rendering priorities

1. Replace stylized hero trees and boulders.
2. Replace cube-composed primitive facilities with realistic wood/stone/fiber
   modular pieces.
3. Multi-layer forest-floor material: grass/soil/mud/leaf litter/rock.
4. Terrain height + slope blending.
5. Real water materials after authoritative Hydrology read models.
6. Better skylight/HDRI calibration, fog and exposure.
7. PC/Android separated render scalability.

## Fallback rule

Production/local-view presentation prefers:
`/Game/Environment/Photoreal/**`

Rules:
- Do not replace a missing photoreal hero asset with Engine Cube/Cone or another obvious placeholder.
- Quaternius may remain for bootstrap, distant LOD, mobile budget tiers, or explicit developer/test modes.
- In normal local-view production presentation, a missing approved asset means the decorative visual is omitted until an approved replacement exists.
- Core resources, facility state and collision authority continue to exist even when a presentation mesh is omitted.

This prevents the scene from silently regressing to prototype-looking geometry while the asset library is upgraded in waves.


## Wave 1 validation evidence

The compact Poly Haven wave completed the zero-cost GitHub Actions import path successfully.

Validated outputs committed by the import workflow:
- `SM_LL_fir_sapling.uasset`
- `SM_LL_pine_sapling_small.uasset`
- `SM_LL_boulder_01.uasset`
- `SM_LL_tree_stump_01.uasset`
- `SM_LL_shrub_02.uasset`
- `SM_LL_shrub_03.uasset`
- `SM_LL_weed_plant_02.uasset`

Validation sequence completed before the generated asset commit:
1. curated source acquisition with per-asset payload guards.
2. Unreal 5.6 LifeLensEditor compile.
3. headless Unreal import.
4. repository file-size guard.
5. generated Unreal assets committed by GitHub Actions.

The generated asset commit intentionally uses `[skip ci]` because it is the output of the already-successful headless import workflow. Source/config changes remain subject to normal Preflight and Unreal compile gates.
