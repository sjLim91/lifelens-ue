# LifeLens Desktop Platform Matrix v1

Status: canonical companion for desktop renderer / editor / QA parity.

## Product rule
LifeLens PC means **Windows + macOS**. Both use the same Core / World / Save truth and one PC roadmap. Renderer switches may differ by platform capability.

## Baseline matrix

| Feature | Windows | macOS broad | Android |
|---|---|---|---|
| Dynamic GI | Lumen software | Lumen software | mobile/non-Lumen |
| Reflections | Lumen | Lumen | mobile path |
| TSR | on | on | off/mobile AA |
| Mesh Distance Fields | on | on | off |
| VSM | on | off | off |
| Nanite | on | off | off |
| Hardware RT | off baseline | off baseline | off |
| Sky/atmosphere | desktop | desktop | mobile-safe |
| Volumetric fog | on | on | off |
| Texture mip bias | 0 | 0 | +1 |

## Source-code rule
Shared desktop behavior uses `PLATFORM_WINDOWS || PLATFORM_MAC`. Windows-only or Mac-only branches require a genuine platform-specific reason.

## QA
Linux compile proves shared C++ integration only. Windows and Mac each require native Editor/runtime diagnostics plus representative scene visual/performance QA before their renderer tier is considered complete.
