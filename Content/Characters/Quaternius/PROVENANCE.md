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

## Not included

- Source-tier files, `.blend` sources, Unity/Godot folders, the `Rigged to Head Bone` hair variants (Unity-oriented).
- Any other Quaternius pack. Verify its own license before adding.

## Staging

Original zips, extracted folders and license screenshots are kept outside the repository at `<workspace>/assets_staging/Quaternius/` (not committed). Only imported `.uasset` files and this document live under `Content/Characters/Quaternius/`.
