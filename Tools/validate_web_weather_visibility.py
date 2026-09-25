#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
weather = (
    root / "web/src/render/weather-layer.ts"
).read_text(encoding="utf-8")
scene = (
    root / "web/src/render/world-scene.ts"
).read_text(encoding="utf-8")
atmosphere = (
    root / "web/src/render/atmosphere-layer.ts"
).read_text(encoding="utf-8")

# Weather summary and visual presentation must never disagree by silently
# hiding low-intensity authoritative precipitation.
for token in (
    "summary === 'Rain'",
    "summary === 'Storm'",
    "visibleFloor",
    "Math.max(reportedIntensity, visibleFloor)",
    "ShaderMaterial",
    "gl_PointCoord",
    "gl_PointSize",
    "MAX_RAIN_DROPS",
    "setFocus(worldX: number, worldZ: number)",
):
    assert token in weather, f"missing visible web precipitation token: {token}"

# Precipitation follows the observer focus so panning or close inspection does
# not leave the rain volume behind at the original world origin.
assert "this.weatherLayer.setFocus(panX, panZ)" in scene

# Rain/snow must visibly affect the world surface and atmosphere, not only the
# HUD badge.
for token in (
    "surfaceWetness01",
    "summaryFloor",
    "updateTerrainWeather",
    "applyTerrainWeather",
    "material.roughness",
    "multiplyScalar(1 - wetness * 0.3)",
):
    assert token in scene, f"missing wet terrain presentation token: {token}"

for token in (
    "reportedPrecipitation",
    "storm ? 0.72 : rain || snow ? 0.34 : 0",
    "storm ? 0.9 : rain || snow ? 0.72 : 0",
    "rain ? 0x526369",
):
    assert token in atmosphere, f"missing weather atmosphere visibility token: {token}"

print("LifeLens web visible weather recovery: PASS")
