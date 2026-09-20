# Character Appearance Data Contract v1

## Purpose

Character Appearance v1 needs a stable, vendor-neutral visual profile without creating a second simulation authority or a new SaveGame cache.

`ULLAppearanceProfileLibrary::MakeDeterministicAppearanceProfile(...)` projects a resident's stable Core-backed identity into `FLLAppearanceProfile`.

## Stability contract

`ULLCoreBridgeSubsystem` already derives `ResidentId` from `(WorldSeed, Core CharacterId)` and rebuilds that same identity after Core snapshot restore.

Appearance now derives its visual profile from:

- stable `ResidentId`
- authoritative Core `Sex`
- authoritative Core `LifeStage`
- authoritative persisted Core `GeneticsProfile` phenotype axes

The profile does not need separate persistence. The same restored resident produces the same look, while children inherit visible skin/eye/hair/height/build phenotype from their parents through Core genetics. Hair-style/outfit catalogue choices remain deterministic presentation-only variation.

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

## Genetics integration

LifeLensCore persists `GeneticsProfile` traits including face shape, eye/hair pigment, skin tone, height potential and build potential. The Unreal bridge now exposes those traits through `FLLCoreGeneticsSnapshot`, and `MakeGeneticAppearanceProfile(...)` uses them for inheritable phenotype axes.

Current asset limitation: the shipped human body set does not expose a production face-morph catalogue, so `FaceShape` is carried through the contract but is not yet expressed as a true facial morph. Skin, eye, hair pigment, height and build are actively visible. A later facial-morph asset pass can consume the already-authoritative `FaceAxis` without changing Save authority.

## Dagyeom usage

Character presentation code may call:

`ULLAppearanceProfileLibrary::MakeDeterministicAppearanceProfile(ResidentId, Sex, LifeStage)`

using the resident identity/sex/life-stage already supplied by the Core bridge/observer binding.

The resulting profile should choose modular mesh/material/hair/clothing variants, while selection ring and label behavior remain owned by the existing Character Presentation component.
