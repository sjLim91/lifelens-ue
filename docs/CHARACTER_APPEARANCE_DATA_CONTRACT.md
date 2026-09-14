# Character Appearance Data Contract v1

## Purpose

Character Appearance v1 needs a stable, vendor-neutral visual profile without creating a second simulation authority or a new SaveGame cache.

`ULLAppearanceProfileLibrary::MakeDeterministicAppearanceProfile(...)` projects a resident's stable Core-backed identity into `FLLAppearanceProfile`.

## Stability contract

`ULLCoreBridgeSubsystem` already derives `ResidentId` from `(WorldSeed, Core CharacterId)` and rebuilds that same identity after Core snapshot restore.

Appearance v1 therefore derives its visual profile from:

- stable `ResidentId`
- authoritative Core `Sex`
- authoritative Core `LifeStage`

The profile does not need separate persistence. The same restored resident produces the same base face/skin/eye/hair/body/outfit axes and variant indices.

## Profile fields

- `VisualSeed`
- `FaceAxis`
- `SkinToneAxis`
- `EyeColorAxis`
- `HairColorAxis`
- `HeightAxis`
- `BuildAxis`
- `FaceVariant`
- `HairStyleVariant`
- `OutfitVariant`
- current `Sex`
- current `LifeStage`

Continuous axes are normalized to `[0,1]`. Variant counts are presentation defaults, not asset IDs or Core truth. Character presentation may remap them to whatever licensed asset set is selected.

## Authority boundary

- Core remains authority for resident identity, sex, life stage, genetics, lifecycle and simulation state.
- Appearance profile is a deterministic presentation projection only.
- No UI/Character-side SaveGame authority or mutable appearance cache is introduced.
- Asset/vendor identifiers stay out of LifeLensCore.

## Genetics follow-up

LifeLensCore already persists `GeneticsProfile` traits including face shape, eye/hair pigment, skin tone, height potential and build potential. Phase F (Appearance Genetics & Lifecycle) can replace or bias the v1 visual axes with those authoritative inherited traits while keeping the same presentation-facing profile concept.

## Dagyeom usage

Character presentation code may call:

`ULLAppearanceProfileLibrary::MakeDeterministicAppearanceProfile(ResidentId, Sex, LifeStage)`

using the resident identity/sex/life-stage already supplied by the Core bridge/observer binding.

The resulting profile should choose modular mesh/material/hair/clothing variants, while selection ring and label behavior remain owned by the existing Character Presentation component.
