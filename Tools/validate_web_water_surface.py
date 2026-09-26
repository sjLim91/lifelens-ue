#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
layer = (
    root / "web/src/render/water-layer.ts"
).read_text(encoding="utf-8")
geometry = (
    root / "web/src/render/water-geometry.ts"
).read_text(encoding="utf-8")
scene = (
    root / "web/src/render/world-scene.ts"
).read_text(encoding="utf-8")

# Water remains projected only from authoritative Core topology.
for token in (
    "createWaterGeometryBuilder",
    "chunk.waterKind",
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

# Wetland is ecological ground, not a full-chunk open-water plane.
water_kind_block = layer[
    layer.index("const WATER_KINDS"):
    layer.index("type VisibleWaterKind")
]
assert "'Wetland'" not in water_kind_block
assert "| 'Wetland'" not in layer

# The failed v2 experiment used overlapping transparent/specular slabs.
# Recovery keeps each source water tile bounded and opaque/stable.
assert "chunkWorldSize * 0.96" in geometry
assert "chunkWorldSize * 1.02" not in geometry
assert "MeshStandardMaterial" in layer
assert "transparent: false" in layer
assert "opacity: 1" in layer
assert "depthWrite: true" in layer
assert "ShaderMaterial" not in layer
assert "water-surface-material" not in layer

# Weather/daylight can tint roughness/brightness, but cannot animate separate
# per-chunk reflection fields or change hydrology.
for token in (
    "setEnvironment",
    "setSimulationMinute",
    "daylight01",
    "wind01",
    "rain01",
    "material.roughness",
):
    assert token in layer, f"missing stable water response token: {token}"

for token in (
    "this.waterLayer.setSimulationMinute(minute)",
    "this.waterLayer.setEnvironment(environment)",
    "this.waterLayer.update(deltaSeconds)",
):
    assert token in scene, f"world scene does not drive water: {token}"

print("LifeLens web stable water regression recovery: PASS")
