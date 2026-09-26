#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
layer = (root / "web/src/render/water-layer.ts").read_text(encoding="utf-8")
geometry = (root / "web/src/render/water-geometry.ts").read_text(encoding="utf-8")
scene = (root / "web/src/render/world-scene.ts").read_text(encoding="utf-8")
vegetation = (root / "web/src/render/vegetation-layer.ts").read_text(encoding="utf-8")
ground = (root / "web/src/render/ground-detail-layer.ts").read_text(encoding="utf-8")

for token in (
    "buildOpenWaterSurfaceGeometry",
    "MARCHING_POLYGONS",
    "buildOpenWaterNodes",
    "componentByKey",
    "OPEN_WATER_KINDS",
    "buildFlowWaterSurfaceGeometry",
    "preferredFlowTarget",
    "selectFlowEdges",
    "incoming",
    "outgoing",
    "undirectedEdges",
    "appendCurvedRibbon",
    "vertexColors: true",
):
    assert token in geometry or token in layer, f"missing continuous-water token: {token}"

assert "chunkWorldSize * 0.96" not in geometry
assert "chunkWorldSize * 1.02" not in geometry
assert "openWaterMesh" in layer
assert "openWaterMaterial" in layer
assert "flowWaterMesh" in layer
assert "flowWaterMaterial" in layer
assert "transparent: false" in layer
assert "depthWrite: true" in layer
assert "Wetland" not in geometry[geometry.index("OPEN_WATER_KINDS"):geometry.index("CONNECTED_WATER_KINDS")]

for forbidden in (
    "waterAvailability +=",
    "chunks.push(",
    "Math.random(",
    "const directions = connected.length > 0",
):

    assert forbidden not in layer
    assert forbidden not in geometry

for token in (
    "chunk.waterKind === 'Ocean'",
    "chunk.waterKind === 'Coast'",
    "chunk.waterKind === 'Lake'",
    "chunk.waterKind === 'River'",
    "chunk.waterKind === 'Stream'",
    "chunk.waterKind === 'Spring'",
):
    assert token in vegetation, f"trees may leak into water: {token}"

assert "chunk.waterKind === 'Coast'" in ground
assert "case 'Coast': return 0x66705a" in scene

assert "flowWidth(chunk.waterKind, chunkWorldSize)" in geometry
assert "chunkWorldSize * 0.14" in geometry
assert "chunkWorldSize * 0.065" in geometry
assert "chunkWorldSize * 0.038" in geometry
assert "edges.push(proposal)" in geometry
assert "if (incoming.has(targetKey)) continue" in geometry
assert "if (outgoing.has(proposal.source.key)) continue" in geometry

print("LifeLens web continuous water and natural flow network: PASS")
