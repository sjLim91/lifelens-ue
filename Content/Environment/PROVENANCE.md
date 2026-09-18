# Content/Environment — Asset Provenance

World Visual Milestone A. Every pack is pinned to the exact downloaded version; licences are verified per pack and never assumed from another pack by the same author.

## Pack 1 — Quaternius "Stylized Nature MegaKit" [Standard]

- Source page: https://quaternius.itch.io/stylized-nature-megakit (pack page: https://quaternius.com/packs/stylizednaturemegakit.html)
- Downloaded: 2026-09-15 by 다겸 (STILLofficial)
- File: `Stylized Nature MegaKit[Standard].zip`, 104,088,529 bytes
- Pack folder timestamp: 2024-07-30
- Licence: CC0 1.0 Universal — `License_Standard.txt` inside the zip states "CC0 1.0 Universal (CC0 1.0) Public Domain Dedication https://creativecommons.org/publicdomain/zero/1.0/". Pack page screenshot kept in staging as `license_screenshot.png`.
- Tier: Standard (free). 68 of the pack's 116 models; the rest are Pro/Source tier and are not in this repository.
- Imported: all 68 `glTF/*.gltf` models → `/Game/Environment/Quaternius/StylizedNature`
  - trees 20 (CommonTree 1-5, DeadTree 1-5, Pine 1-5, TwistedTree 1-5)
  - plants and grass 16 (Grass_Common/Wispy short and tall, Fern, Clover 1-2, Plant_1/_1_Big/_7/_7_Big, Petal 1-5)
  - flowers and mushrooms 6, bushes 2
  - rocks and pebbles 24 (Rock_Medium 1-3, Pebble_Round 1-5, Pebble_Square 1-6, RockPath 10)
- Not imported: `FBX`, `FBX (Unity)`, `OBJ` duplicates of the same models.
- Triangle budget measured from the source glTF: trees 5,648-6,265, bushes ~900, clover 379-615.
- Import notes: the pack shares one texture set across many models and Interchange imports a private copy per model, so the import script consolidates duplicates by name. 93 duplicate textures and 76 duplicate material instances were merged, which took the imported folder from 264 MB to 49 MB.

## Pack 2 — ambientCG ground materials (1K PNG)

- Source: https://ambientcg.com
- Downloaded: 2026-09-15 by 다겸 (STILLofficial)
- Licence: CC0 1.0. ambientCG states "All assets are released under the Creative Commons CC0 license, making them free to use without attribution - even in commercial circumstances." Site screenshot kept in staging as `license_screenshot.png`.
- Sets and file sizes:
  - `Grass004_1K-PNG.zip`, 19,597,533 bytes — https://ambientcg.com/get?file=Grass004_1K-PNG.zip
  - `Ground037_1K-PNG.zip`, 19,321,308 bytes — https://ambientcg.com/get?file=Ground037_1K-PNG.zip
  - `Ground054_1K-PNG.zip`, 17,930,839 bytes — https://ambientcg.com/get?file=Ground054_1K-PNG.zip
- Imported maps per set → `/Game/Environment/ambientCG`: `_Color`, `_NormalGL`, `_Roughness`, `_AmbientOcclusion`. 12 textures total.
- Not imported: `_Displacement`, `.blend`, `.usdc`, `.mtlx`, `.tres`, preview sphere renders, and the DirectX-convention `_NormalDX` maps.

## Android budget

- Every imported texture carries `MaxTextureSize = 1024`, so the cooked size stays at 1K regardless of the source resolution.
- All nature assets are static meshes so the presentation can draw them through instanced components.
- Meshes are untouched engine imports; no runtime-authored geometry.

## Import

Reproducible headlessly with the editor closed:

```
"/Users/Shared/Epic Games/UE_5.6/Engine/Binaries/Mac/UnrealEditor-Cmd" \
  "<repo>/LifeLens.uproject" -run=pythonscript \
  -script="<repo>/Content/Environment/Quaternius/Import/import_nature.py" \
  -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash -stdout -FullStdOutLogOutput
```

Original zips, extracted folders and licence screenshots live outside the repository under `<workspace>/assets_staging/` and are not committed.


## Pack 3 — Poly Haven photoreal CC0 wave

- Source: https://polyhaven.com
- API: https://api.polyhaven.com
- Licence: CC0 1.0 for assets.
- Acquisition: `Tools/acquire_photoreal_environment_assets.py`.
- Live API requests use the required project-specific User-Agent.
- Original model/texture files remain outside Git under `LL_ASSET_STAGING/PolyHaven`.
- Unreal import: `Content/Environment/Photoreal/Import/import_photoreal_nature.py`.
- Unreal destination: `/Game/Environment/Photoreal/PolyHaven`.

Curated photoreal pool:
- `pine_tree_01`
- `fir_sapling`
- `tree_small_02`
- `boulder_01`
- `rock_07`
- `rock_09`
- `tree_stump_01`
- `dead_tree_trunk`
- `forest_floor`
- `forrest_ground_01`
- `mossy_rock`
- `nature_reserve_forest`

Initial automated import wave intentionally starts smaller:
- `pine_tree_01`
- `tree_small_02`
- `boulder_01`
- `tree_stump_01`

The first automated wave acquires 1K source textures to prove the end-to-end zero-cost
download/import/commit path without repository bloat. The desktop target remains 2K+
for later validated waves; Android quality is reduced through LOD/streaming rather than
making the stylized fallback the production art direction.
