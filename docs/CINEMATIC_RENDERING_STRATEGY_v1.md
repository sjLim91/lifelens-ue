# LifeLens Cinematic Rendering Strategy v1

Status: canonical rendering companion for Master Spec §6 / §20.

## 1. Product goal

LifeLens keeps one authoritative simulation and one world identity, but does **not** force the Windows visual ceiling down to the Android renderer budget.

```
LifeLens Core / World / Save
        |
        +-----------------------------+
        |                             |
Windows PC Presentation        Android Presentation
cinematic real-time tier       mobile-optimized tier
```

The same resident, facility, resource, terrain state, weather and event must remain semantically identical across platforms. Rendering quality may differ; simulation truth may not.

## 2. Windows PC cinematic baseline

The Windows target uses Unreal Engine 5.6's native high-fidelity real-time stack:

- DirectX 12 + Shader Model 6.
- Lumen Global Illumination.
- Lumen Reflections.
- Virtual Shadow Maps.
- Temporal Super Resolution (TSR).
- Nanite project support for eligible assets.
- Mesh Distance Fields for Lumen software tracing.
- Sky Atmosphere / volumetric environment presentation where appropriate.
- PBR photoreal source assets, material layering, weather/wetness response and high-detail local-view presentation.

Baseline Lumen uses **software tracing**, not mandatory hardware ray tracing. Hardware RT is a future optional PC Ultra tier only after minimum-spec profiling.

## 3. Android baseline

Android stays the first delivery/device target, but it is a separate renderer tier:

- no Lumen baseline.
- no Virtual Shadow Maps baseline.
- no Nanite baseline.
- TAA/mobile anti-aliasing.
- HISM/instancing, conventional LOD/HLOD/fallback meshes.
- reduced texture residency, foliage density, shadow distance and view distance through Device Profiles.
- the same world/resource/facility/character identity as PC.

Android performance constraints may simplify a visual representation; they must not delete or alter authoritative simulation state.

## 4. Nanite policy

Nanite support being enabled for Windows does **not** automatically make every mesh a Nanite mesh.

Eligible PC assets may opt in when they benefit from Nanite:
- rocks/cliffs/high-detail static environment.
- photogrammetry-like static props.
- dense architecture or other high-detail static geometry.

Requirements:
- Android-compatible fallback mesh/LOD remains available.
- collision/navigation authority is not derived from Nanite visibility.
- foliage Nanite is not assumed safe by default; it must be profiled as a separate presentation task.
- an extremely large source asset is not accepted simply because Nanite can render it. Import/storage/build budgets still apply.

## 5. Lumen / lighting policy

PC local-view presentation should use dynamic GI/reflection rather than fake baked lighting as the default target.

Lumen is responsible for:
- diffuse bounce lighting.
- dynamic sky/light response.
- emissive contribution where appropriate.
- reflection response consistent with dynamic time/weather.

Presentation tuning must consume authoritative time/weather/environment state; it must not invent a second weather or time truth.

## 6. Visual asset quality rule

Production local-view should prefer approved photoreal assets/materials.

Do not silently replace a missing production asset with:
- Engine Cube/Cone.
- visibly prototype geometry.
- unrelated low-poly hero assets.

If the visual is decorative and no approved replacement exists, omission is preferable to a visibly broken placeholder. Core state still exists.

Quaternius/other lightweight CC0 assets may remain useful for:
- Android LOD/fallback.
- distant presentation.
- bootstrap/developer modes.
- places where the asset is genuinely stylistically acceptable after review.

## 7. Platform configuration

Canonical platform overrides:

- `Config/Windows/WindowsEngine.ini`
  - DX12 + SM6.
  - Lumen GI/reflections.
  - VSM.
  - TSR.
  - Nanite project support.
  - Mesh Distance Fields.
- `Config/Android/AndroidEngine.ini`
  - mobile-safe GI/reflection/shadow/AA path.
  - Lumen/Nanite/VSM disabled.
- `Config/DefaultDeviceProfiles.ini`
  - residency, LOD, view-distance, foliage/shadow density scaling.

The global `Config/DefaultEngine.ini` remains a universal Android-safe baseline instead of forcing desktop-only features onto every platform.

## 8. Presentation follow-up

Config alone cannot produce cinematic imagery. The presentation lane must also supply:

1. eligible photoreal meshes/materials and Android fallback LODs.
2. explicit Nanite opt-in for approved Windows assets.
3. Sky Atmosphere / volumetric fog/cloud / exposure tuning.
4. dynamic sun/moon/weather lighting driven from authoritative environment state.
5. terrain material layering and slope/height/wetness response.
6. water material/presentation driven by authoritative Hydrology.
7. quality-scaled foliage/ground cover.
8. runtime visual QA on representative PC and Android devices.

## 9. Validation

A renderer feature is not considered delivered merely because an INI value exists.

Windows acceptance:
- packaged/editor Windows path reports DX12 + SM6.
- Lumen GI/reflections visibly active.
- VSM active.
- TSR active.
- approved Nanite assets show Nanite rendering while fallback meshes remain valid.
- GPU/frame-time profiling captured for representative local-view scenes.

Android acceptance:
- APK/device launch succeeds.
- no unsupported Lumen/Nanite/VSM dependency.
- fallback assets render correctly.
- resident/world semantics match the PC world.
- device GPU/frame-time and memory are profiled.

## 10. Non-goals

- Movie Render Queue / Path Tracer is not the normal gameplay renderer.
- RTX-class hardware is not a mandatory baseline requirement.
- visual quality never becomes simulation authority.
- Android-first delivery does not mean PC graphics are permanently capped at mobile quality.


## Platform package payload boundary

The source repository is shared, but final platform packages carry only the presentation payload they need.

Canonical packaging/content boundary:
- `docs/PLATFORM_CONTENT_COOK_POLICY_v1.md`.

In particular, Android must not inherit desktop-only photoreal nature or desktop-authored PCG dependencies merely because those assets exist in the repository. Windows/macOS likewise exclude future mobile-only content.
