#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
layer = (
    root / "web/src/render/water-layer.ts"
).read_text(encoding="utf-8")
geometry = (
    root / "web/src/render/water-geometry.ts"
).read_text(encoding="utf-8")
material = (
    root / "web/src/render/water-surface-material.ts"
).read_text(encoding="utf-8")
scene = (
    root / "web/src/render/world-scene.ts"
).read_text(encoding="utf-8")

# Water remains Core-topology-driven. Presentation may shade and slightly
# overlap adjacent source chunks, but must not invent new water kinds/chunks.
for token in (
    "createWaterGeometryBuilder",
    "chunk.waterKind",
    "chunkMap",
    "WORLD_GRID_CONTRACT.worldUnitsPerChunk",
):
    assert token in layer or token in geometry, (
        f"missing Core-driven water token: {token}"
    )

for forbidden in (
    "waterAvailability +=",
    "chunks.push(",
    "Math.random(",
):
    assert forbidden not in layer
    assert forbidden not in geometry

# Avoid false-positive matching of equality checks such as `===`.
assert "chunk.waterKind = '" not in layer
assert 'chunk.waterKind = "' not in layer
assert "chunk.waterKind = '" not in geometry
assert 'chunk.waterKind = "' not in geometry

# Open-water seams are hidden only by a tiny presentation overlap.
assert "chunkWorldSize * 1.02" in geometry
assert "chunkWorldSize * 1.2" not in geometry

# Mobile-friendly procedural shading: no texture/network dependency, animated
# with a tiny fixed-cost fragment function and a flat source surface.
for token in (
    "ShaderMaterial",
    "uTime",
    "uDeepColor",
    "uShallowColor",
    "uSkyColor",
    "uWaveScale",
    "uFlowSpeed",
    "uFlow",
    "uWind",
    "uRain",
    "uDaylight",
    "waveField",
    "fresnel",
    "specular",
):
    assert token in material, f"missing water shader token: {token}"

for forbidden in (
    "TextureLoader",
    "loadAsync(",
    "sampler2D",
):
    assert forbidden not in material, (
        f"web water shader must stay texture-free/mobile-cheap: {forbidden}"
    )

# Flow direction is projected from connected authoritative neighbors and
# elevation; weather/time are presentation inputs only.
for token in (
    "configureFlow",
    "candidate.chunk",
    "elevation01",
    "setEnvironment",
    "setSimulationMinute",
    "update(deltaSeconds",
):
    assert token in layer, f"missing dynamic water token: {token}"

for token in (
    "this.waterLayer.setSimulationMinute(minute)",
    "this.waterLayer.setEnvironment(environment)",
    "this.waterLayer.update(deltaSeconds)",
):
    assert token in scene, f"world scene does not drive water: {token}"

print("LifeLens web animated water surface: PASS")
