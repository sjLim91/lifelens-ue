#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
weather = (
    root / "web/src/render/weather-layer.ts"
).read_text(encoding="utf-8")
scene = (
    root / "web/src/render/world-scene.ts"
).read_text(encoding="utf-8")
surface = (
    root / "web/src/render/environment-surface-presentation.ts"
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

# Snow must be a soft screen-space flake, never the default square PointsMaterial.
assert "private readonly snowMaterial = new THREE.ShaderMaterial" in weather
assert "length(centered)" in weather
assert "smoothstep(0.28, 0.5, radius)" in weather
assert "snow.onBeforeRender" in weather
assert "new THREE.PointsMaterial" not in weather

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
    "applyGroundWetness(material, this.surfaceWetness01)",
    "snowPresentationCoverage",
    "updateSurfaceSnow",
):
    assert token in scene, f"missing wet terrain presentation token: {token}"

# Wet-surface material math was extracted from WorldScene so terrain, rocks and
# facilities can share one presentation contract. Validate the helper itself
# instead of requiring those implementation literals to stay in world-scene.ts.
for token in (
    "material.color.setScalar(1 - wet * config.wetGroundDarkening)",
    "material.roughness",
    "config.dryGroundRoughness",
    "config.wetGroundRoughness",
    "SurfaceSnowModifier",
):
    assert token in surface, f"missing shared surface weather token: {token}"

for token in (
    "reportedPrecipitation",
    "storm ? 0.72 : rain || snow ? 0.34 : 0",
    "storm ? 0.9 : rain || snow ? 0.72 : 0",
    "rain ? 0x526369",
):
    assert token in atmosphere, f"missing weather atmosphere visibility token: {token}"

print("LifeLens web visible weather recovery: PASS")
