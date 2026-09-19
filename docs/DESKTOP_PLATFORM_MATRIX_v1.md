# LifeLens Desktop Platform Matrix v1

Status: canonical companion for desktop renderer / editor / QA parity.

## Product rule

LifeLens PC means **Windows + macOS**.

Both desktop platforms:
- run the same Core / World / Save truth.
- advance on the same gameplay and presentation roadmap.
- consume the same authoritative time/weather/hydrology/resource/facility contracts.
- may use different renderer feature switches when Unreal or hardware support differs.

Android remains a separate mobile renderer tier, not a separate game.

## Unreal Engine baseline

Project engine: **Unreal Engine 5.6**.

Epic's UE 5.6 macOS requirements:
- minimum runtime OS: macOS Sonoma 14.0.
- minimum Xcode: 15.2.
- recommended Xcode: 15.4 or newer.
- Lumen software GI/reflections: supported on Intel/AMD Mac GPU and Apple Silicon M1+.
- TSR: supported on Intel/AMD Mac GPU and Apple Silicon M1+.
- Nanite + Virtual Shadow Maps: Beta on Apple Silicon M2+.
- Lumen hardware RT: Experimental on Apple Silicon M2+.

Official reference:
https://dev.epicgames.com/documentation/en-us/unreal-engine/macos-development-requirements-for-unreal-engine?application_version=5.6

Desktop rendering-path reference:
https://dev.epicgames.com/documentation/unreal-engine/supported-features-by-rendering-path-for-desktop-with-unreal-engine

## Renderer matrix

| Feature | Windows baseline | macOS broad baseline | Android baseline |
|---|---|---|---|
| Dynamic GI | Lumen software | Lumen software | mobile/non-Lumen |
| Reflections | Lumen | Lumen | mobile path |
| TSR | on | on | off / mobile AA |
| Mesh Distance Fields | on | on | off |
| Virtual Shadow Maps | on | off | off |
| Nanite project path | on | off | off |
| Hardware RT | off baseline | off baseline | off |
| SkyAtmosphere | desktop high quality | desktop high quality | mobile-safe |
| Volumetric atmosphere | desktop path | desktop path | cheaper fallback |
| Texture mip bias | 0 | 0 | +1 |
| Foliage/grass density | full desktop | desktop, profiled slightly lower initially | reduced |

## Mac quality tiers

### Mac Broad
Default compatibility tier.

Purpose:
- Dagyeom can develop and visually inspect the same PC product on Mac.
- avoid making M2+ Beta renderer features a mandatory project dependency.
- retain Lumen/TSR/PBR/atmosphere quality.

Config:
- `Config/Mac/MacEngine.ini`.
- `[Mac DeviceProfile]` in `Config/DefaultDeviceProfiles.ini`.

### Mac M2+ High — future profiled option
Not a baseline requirement yet.

Candidate features after actual-device validation:
- Nanite.
- Virtual Shadow Maps.
- optional experimental hardware ray tracing only if it proves worthwhile.

Promotion requires:
- actual Mac hardware identity recorded.
- runtime renderer diagnostics.
- representative LifeLens scene visual QA.
- frame-time / memory measurement.
- no breakage of broad Mac compatibility.

## Source-code platform rule

Presentation code should prefer:
- `PLATFORM_WINDOWS || PLATFORM_MAC` for shared desktop behavior.
- `PLATFORM_ANDROID` for mobile-specific reductions.

Use Windows-only conditionals only for genuinely Windows/DX12-specific behavior.

Likewise, macOS-only code should exist only where Metal/macOS behavior truly differs.

## QA gates

Compile/test evidence and visual evidence are separate.

Windows:
- normal CI compile/preflight.
- runtime renderer diagnostics.
- representative local-view screenshot/video + frame timing.

macOS:
- normal shared CI compile/preflight where platform-independent.
- native Mac Editor/project open + compile on representative Mac.
- runtime renderer diagnostics.
- representative local-view screenshot/video + frame timing.

Android:
- Gate B remains paused until explicitly resumed.
- when resumed, APK/device launch and GPU/memory profiling remain required.

## Current limitation

A Linux Unreal compile proves shared C++ integration but **does not prove the native Windows or macOS renderer actually booted with the intended feature tier**.

Runtime diagnostics and native-platform visual QA remain mandatory before declaring either desktop renderer complete.
