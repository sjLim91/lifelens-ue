# LifeLens Character Asset Track

Canonical asset direction for Character Appearance v1.

## Decision

**Track B is the default production path for the first human-character milestone.**

- Base character pack: Quaternius **Universal Base Characters** (pack page explicitly marked CC0).
- Animation source: Quaternius **Universal Animation Library** (pack page explicitly marked CC0).
- MetaHuman is **not** the initial Android baseline. It remains an upgrade path after the Android smoke/performance gate succeeds.

## Why Track B first

- lightweight humanoid meshes suitable for Android-first validation
- shared humanoid rig and retarget-friendly workflow
- modular hair / skin / eye variation is enough to exercise `FLLAppearanceProfile`
- avoids making high-cost facial rigs, groom strands, large textures, or MetaHuman-specific content a prerequisite for proving LifeLens simulation + Observer + Character integration
- keeps runtime cost at zero

## License / provenance rule

Do not assume every Quaternius asset is CC0 merely because older packs were CC0.

For repository inclusion, pin the exact source pack and retain license/provenance evidence for the downloaded version. The two baseline packs above are selected because their pack pages explicitly identify them as CC0.

If another Quaternius pack is introduced, verify that pack's own license at the time of import. Newer/general Quaternius licensing may differ and must not be silently treated as CC0.

## Mobile baseline

Character Appearance v1 should prove on Android with the lightweight Track B assets first:

- 4 initial residents visible simultaneously
- male/female base variants
- deterministic skin / eye / hair / body variation
- LOD and material-count limits suitable for mobile
- selection ring and label LOD from PR #63 preserved
- Save/Load retains the same resident appearance
- no presentation-side simulation authority

## MetaHuman upgrade gate

MetaHuman becomes eligible only after the lightweight Android milestone is green.

Gate conditions:

1. Android Development build succeeds.
2. Four human residents + Observer UI + Core simulation run together on-device.
3. frame time, memory, package size, skeletal animation cost and thermal behavior are measured.
4. MetaHuman mobile configuration is tested using mobile-appropriate LOD/hair/material settings rather than desktop/cinematic defaults.
5. If MetaHuman exceeds the target budget, Track B remains the Android runtime and MetaHuman may be reserved for desktop/high-quality tiers.

## Architecture rule

Asset choice must remain behind the appearance/presentation layer.

`FLLAppearanceProfile` and resident identity must not encode vendor-specific asset IDs. This allows Track B, MetaHuman, or another future renderer to consume the same authoritative resident appearance inputs without changing Core simulation state.

## Current order

1. Track B Character Appearance v1
2. Character Motion & Context v1
3. integrated runtime verification
4. Android smoke + profiling
5. MetaHuman comparison / upgrade decision
6. later Appearance Genetics & Lifecycle
