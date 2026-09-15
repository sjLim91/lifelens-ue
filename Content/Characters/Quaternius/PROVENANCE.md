# Content/Characters/Quaternius — Asset Provenance

Canonical rule: `docs/CHARACTER_ASSET_TRACK.md` (Track B, Quaternius CC0). Each pack is pinned to the exact downloaded version; do not assume other Quaternius packs share this license.

## Pack 1 — Universal Base Characters [Standard]

- Source page: https://quaternius.itch.io/universal-base-characters (pack page: https://quaternius.com/packs/universalbasecharacters.html)
- Downloaded: 2026-09-14 by 다겸 (STILLofficial)
- File: `Universal Base Characters[Standard].zip`, 128,968,391 bytes (itch.io lists 122 MB)
- Pack folder timestamp: 2025-12-03
- License: CC0 1.0 Universal — `License_Standard.txt` inside the zip states "CC0 1.0 Universal (CC0 1.0) Public Domain Dedication https://creativecommons.org/publicdomain/zero/1.0/". Pack page screenshot kept in staging as `license_screenshot.png`.
- Tier: Standard (free). Contains the Superhero male/female full bodies only; Regular/Teen bodies are Source-tier and are not in this repository.
- Files used for import (from the Unreal-specific folders, per `Base Characters/Unreal-Engine-README.txt` which recommends glTF for Unreal because of an FBX unit-scale bug):
  - `Base Characters/Godot - UE/Superhero_Male_FullBody.gltf` (+ `.bin`, textures) → `/Game/Characters/Quaternius/UBC/Male`
  - `Base Characters/Godot - UE/Superhero_Female_FullBody.gltf` (+ `.bin`, textures) → `/Game/Characters/Quaternius/UBC/Female`
  - `Base Characters/Textures/T_Superhero_Male_Ligh.png`, `T_Superhero_Female_Light_BaseColor.png` (light skin variants not referenced by the glTF) → `/Game/Characters/Quaternius/UBC/Textures`
  - `Hairstyles/Origin at 0/FBX (Unreal Engine)/*.fbx` (Hair_Buns, Hair_SimpleParted, Hair_Long, Hair_BuzzedFemale, Hair_Buzzed, Hair_Beard, Eyebrows_Regular, Eyebrows_Female) → `/Game/Characters/Quaternius/UBC/Hair`
- Skeleton: 65 joints, UE-mannequin-style names (`pelvis`, `spine_01`…, `clavicle_l/r`, `hand_l/r`, `thigh_l/r`, `calf_l/r`, `foot_l/r`, `ball_l/r`, `Head`). Identical joint set to the Universal Animation Library (verified by comparing glTF skin joints).
- Material slots per body: 3 (body skin, eyes, eyebrows/hair).

## Pack 2 — Universal Animation Library [Standard]

- Source page: https://quaternius.com/packs/universalanimationlibrary.html
- Downloaded: 2026-09-14 by 다겸 (STILLofficial)
- File: `Universal Animation Library[Standard].zip`, 15,904,933 bytes
- Pack folder timestamp: 2025-06-17
- License: CC0 1.0 Universal — `License.txt` inside the zip states "CC0 1.0 Universal (CC0 1.0) Public Domain Dedication https://creativecommons.org/publicdomain/zero/1.0/". Pack page screenshot kept in staging as `license_screenshot.png`.
- Tier: Standard (free). 43 animations: A_TPose, Crouch_Fwd_Loop, Crouch_Idle_Loop, Dance_Loop, Death01, Driving_Loop, Fixing_Kneeling, Hit_Chest, Hit_Head, Idle_Loop, Idle_Talking_Loop, Idle_Torch_Loop, Interact, Jog_Fwd_Loop, Jump_Land, Jump_Loop, Jump_Start, PickUp_Table, Pistol_*, Punch_Cross, Punch_Jab, Push_Loop, Roll, Sitting_Enter, Sitting_Exit, Sitting_Idle_Loop, Sitting_Talking_Loop, Spell_Simple_*, Sprint_Loop, Swim_Fwd_Loop, Swim_Idle_Loop, Sword_Attack, Sword_Idle, Walk_Formal_Loop, Walk_Loop.
- Files used for import (Unreal folder, per `Unreal_Setup.png`: skeleton None on first import, Import Animations on, 30 Hz bake, snap to closest frame):
  - `Unreal-Godot/UAL1_Standard.glb` (root motion disabled) → `/Game/Characters/Quaternius/UAL` on the UBC male skeleton
  - `Unreal-Godot/UAL1_Standard_RM.glb` (root motion baked) — not imported in v1
- Skeleton: same 65-joint set as Universal Base Characters.

## Pack 3 — Modular Character Outfits - Fantasy [Standard]

- Source page: https://quaternius.itch.io/modular-character-outfits-fantasy (pack page: https://quaternius.com/packs/modularcharacteroutfitsfantasy.html)
- Downloaded: 2026-09-15 by 다겸 (STILLofficial)
- File: `Modular Character Outfits - Fantasy[Standard].zip`, 294,347,394 bytes (itch.io lists 280 MB)
- Pack version: 2.1 (itch.io devlog 2026-05-07); pack folder timestamp 2026-01-29
- License: CC0 1.0 Universal — `License_Standard.txt` inside the zip states "CC0 1.0 Universal (CC0 1.0) Public Domain Dedication https://creativecommons.org/publicdomain/zero/1.0/". Pack page states "Free to use in personal, educational and commercial projects. (CC0 License)". Pack page screenshot kept in staging as `license_screenshot.png`.
- Tier: Standard (free). Contains the Peasant and Ranger outfits only (male/female, combined `Outfits/*.gltf` and `Modular Parts/*` Arms/Body/Legs/Feet, Ranger also Head_Hood / pauldrons); the other 10 outfits are Source-tier and are not in this repository. `Readme.txt`: outfits are made for the Universal Base Character kit, "only the head of the model is required. Using the full body will result in clipping."
- Files used for import (`Exports/glTF (Godot-Unreal)/Outfits`, skinned to the UBC skeleton, imported with Skeleton = UBC shared skeleton; script `Import/import_quaternius_outfits.py`):
  - `Outfits/Male_Peasant.gltf` (+ `.bin`, `T_Peasant_BaseColor/Normal/ORM.png`, `T_Regular_Male_Dark_BaseColor/Normal/Roughness.png` for the exposed forearm skin) → `/Game/Characters/Quaternius/MCO/Peasant/Male`
  - `Outfits/Female_Peasant.gltf` (+ `.bin`) → `/Game/Characters/Quaternius/MCO/Peasant/Female`; its `T_Peasant_*` textures and `MI_Peasant` are consolidated onto the male copies (redirectors remain)
  - `Textures/Peasant/T_Peasant_2_BaseColor.png` (second colour variation) → `/Game/Characters/Quaternius/MCO/Peasant/Textures`
  - imported textures are clamped to `MaxTextureSize = 2048` (pack ships 4K)
- Not imported in v1: Ranger outfits, `Modular Parts/*`, FBX (Unity) exports, `T_Ranger_*`, `Textures/Base/*` (duplicates of the Ranger "Base Chars" textures).
- Skeleton: same 65-joint set as Universal Base Characters (verified by comparing glTF skin joints, identical order).
- Runtime use: the outfit is a child skeletal mesh following the UBC body pose (leader pose). The Standard tier has no separate UBC head, so the Superhero full body stays under the clothing.

## Not included

- Source-tier files, `.blend` sources, Unity/Godot folders, the `Rigged to Head Bone` hair variants (Unity-oriented).
- Any other Quaternius pack (including Ultimate Modular Men/Women). Verify its own license before adding.

## Staging

Original zips, extracted folders and license screenshots are kept outside the repository at `<workspace>/assets_staging/Quaternius/` (not committed). Only imported `.uasset` files and this document live under `Content/Characters/Quaternius/`.
