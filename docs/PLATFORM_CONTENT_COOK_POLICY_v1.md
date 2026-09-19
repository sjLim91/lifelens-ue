# LifeLens Platform Content / Cook Policy v1

Status: canonical packaging boundary for Android / Windows / macOS.

## Product rule

LifeLens uses **one repository and one authoritative simulation**, but final packages do not carry every platform's presentation payload.

```
shared source repository
├─ Core / World / Save / Bridge              -> all platforms
├─ Shared presentation/content               -> all platforms when referenced
├─ Desktop presentation/content              -> Windows + macOS only
└─ Mobile presentation/content               -> Android only
```

A platform-specific renderer or asset may live in the same Git repository without being cooked/staged into another platform's package.

## Canonical content roots for new work

New platform-specific content should prefer:

- `/Game/Shared/**` — intentionally shared presentation assets.
- `/Game/Desktop/**` — Windows + macOS only.
- `/Game/Mobile/**` — Android only.

Existing historical content is **not mass-moved just to satisfy the folder convention**. Unreal binary moves can create redirectors/reference churn. Existing assets are migrated when touched or replaced.

Platform cook config is authoritative even during that migration.

## Current Android boundary

`Config/Android/AndroidGame.ini` excludes:

- `/Game/Desktop/**`.
- current desktop-authored `/Game/Environment/PCG/**`.
- the large photoreal natural-dressing groups under Poly Haven:
  - boulder_01
  - fir_sapling
  - pine_sapling_small
  - shrub_02
  - shrub_03
  - tree_stump_01
  - weed_plant_02

At the time this policy was introduced those excluded nature source groups account for more than 25 MiB in the repository before platform cooking/compression.

Android `LLWorldPresentationActor` hard references are compile-time separated and use a small Quaternius nature set:
- Pine_1.
- CommonTree_1.
- Bush_Common.
- Grass_Common_Short.
- Grass_Wispy_Short.
- Rock_Medium_1 / 2.

This prevents desktop photoreal nature hard references from pulling their dependency graphs into APK cook.

## Current shared exception

The following compact facility hero props remain shared for now:

- `stone_fire_pit`.
- `wicker_basket_01`.
- `wooden_axe`.

Together they are small compared with the nature catalogue and preserve facility readability on Android.

When dedicated mobile replacements exist, they can move behind the same Desktop/Mobile split. Until then Android must **not** exclude the entire `/Game/Environment/Photoreal` root.

## Desktop boundary

`Config/Windows/WindowsGame.ini` and `Config/Mac/MacGame.ini` exclude:

- `/Game/Mobile/**`.

Desktop packaging may use:
- full photoreal nature.
- desktop-authored PCG graph/assets.
- platform renderer features allowed by Windows/macOS capability policy.

Windows and macOS share the PC product roadmap. Renderer switches may differ; packaging ownership does not.

## Source-code rule

Shared C++ stays shared when reasonable.

Platform-specific asset hard references must be guarded at compile time:

```cpp
#if PLATFORM_ANDROID
    // mobile refs
#else
    // Windows + macOS desktop refs
#endif
```

Use more specific platform macros only when behavior genuinely differs by platform.

The goal is not separate games. The goal is **shared simulation + platform-specific presentation payloads**.

## CI / packaging guard

`Tools/validate_platform_cook_boundaries.py` checks:

- Android never-cook entries.
- Windows/macOS Mobile exclusions.
- mobile vs desktop hard-reference separation.
- compact shared facility exception.
- absence of global `bCookAll=True`.
- that the current Android build workflow invokes this validator.

`LifeLens Preflight` runs the same validator on PRs.

When Android Gate B resumes, the package itself should additionally be inspected as part of APK QA. Structural validation prevents known hard-reference regressions, but only an actual cook/package proves final artifact contents and size.

## Done criteria for this boundary

- desktop nature refs compile out of Android source.
- Android uses explicit lightweight nature refs.
- Android platform packaging config excludes current desktop nature/PCG groups.
- Windows and macOS exclude future mobile-only content.
- CI fails if these rules silently regress.
- future new platform-only content follows `Shared/Desktop/Mobile` convention.
