#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
layer = (root / "web/src/render/water-layer.ts").read_text(encoding="utf-8")
geometry = (root / "web/src/render/water-geometry.ts").read_text(encoding="utf-8")
types = (root / "web/src/runtime/core-types.ts").read_text(encoding="utf-8")
bridge = (
    root / "Source/LifeLensCore/src/WebClientBridge.cpp"
).read_text(encoding="utf-8")

for token in (
    "flowPotential",
    "drainageAccumulationPotential",
    "hasDownstream",
    "downstreamChunkX",
    "downstreamChunkY",
):
    assert token in types, f"web TerrainChunk missing authoritative hydrology field: {token}"
    assert token in bridge, f"Core terrain JSON missing authoritative hydrology field: {token}"

# The browser must not reconstruct a fake drainage graph from arbitrary adjacent
# water chunks. It consumes the exact downstream relation emitted by Core.
for token in (
    "authoritativeFlowTarget",
    "node.chunk.hasDownstream !== true",
    "node.chunk.downstreamChunkX",
    "node.chunk.downstreamChunkY",
    "buildOpenWaterSurfaceGeometry",
    "buildFlowWaterSurfaceGeometry",
    "appendCurvedRibbon",
):
    assert token in geometry, f"missing authoritative water projection token: {token}"

for forbidden in (
    "preferredFlowTarget",
    "incoming = new Set",
    "outgoing = new Set",
    "undirectedEdges",
    "const directions = connected.length > 0",
    "Math.random(",
):
    assert forbidden not in geometry, (
        f"browser-side hydrology inference/regression returned: {forbidden}"
    )

# Older runtimes may omit the new fields. In that case the browser hides
# uncertain channel segments rather than guessing a direction.
assert "chunk.hasDownstream === true" in geometry
assert "Number(chunk.drainageAccumulationPotential) || 0" in geometry
assert "drainage >= 0.24" in geometry
assert "availability >= 0.5" in geometry

# Width comes from the same Core hydrology presentation formula scale rather
# than arbitrary percentages of the whole chunk.
assert "WORLD_UNITS_PER_GRID_CELL" in geometry
assert "2.75 * availability" in geometry
assert "1.35 * availability" in geometry
assert "0.85 * availability" in geometry

# Open water stays stable and opaque while the channel repair is isolated.
assert "openWaterMesh" in layer
assert "flowWaterMesh" in layer
assert "transparent: false" in layer
assert "depthWrite: true" in layer

print("LifeLens web authoritative hydrology projection: PASS")
