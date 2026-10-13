#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

world_scene = (
    root / "web/src/render/world-scene.ts"
).read_text(encoding="utf-8")
terrain = (
    root / "web/src/render/terrain-geometry.ts"
).read_text(encoding="utf-8")
ground = (
    root / "web/src/render/ground-detail-layer.ts"
).read_text(encoding="utf-8")
vegetation = (
    root / "web/src/render/vegetation-layer.ts"
).read_text(encoding="utf-8")
tree_asset = (
    root / "web/src/render/tree-asset-layer.ts"
).read_text(encoding="utf-8")
profile = (
    root / "web/src/render/vegetation-profile.ts"
).read_text(encoding="utf-8")
readme = (
    root / "web/README.md"
).read_text(encoding="utf-8")

# Presentation v2 must enrich the authoritative surface without inventing a
# second terrain simulation. Elevation remains sourced only from the shared
# Core elevation sampler.
assert "createTerrainElevationSampler" in terrain
assert "sampleElevation(" in terrain
assert "positions.push(px, elevation, z)" in terrain
assert "synthetic" not in terrain.lower()
assert "chunk.elevationSamples?.length === 289 ? 16 : 10" in terrain
assert "elevationSamples" in (root / "Source/LifeLensCore/src/WebClientBridge.cpp").read_text(encoding="utf-8")
assert "geometry.setAttribute(\n      'color'" in terrain
assert "vertexColors: true" in world_scene

# Ground detail is driven by Core ecology coverage and is presentation-only.
for token in (
    "grassCoverage01",
    "shrubCoverage01",
    "rockCoverage01",
    "wetlandCoverage01",
    "forestCoverage01",
    "createTerrainElevationSampler",
    "InstancedMesh",
    "setWetness",
):
    assert token in ground, f"missing ground detail token: {token}"

for forbidden in (
    "waterAvailability +=",
    "forestCoverage01 =",
    "grassCoverage01 =",
    "chunk.elevation01 =",
):
    assert forbidden not in ground, (
        f"presentation must not mutate Core truth: {forbidden}"
    )

assert "this.groundDetailLayer.setTerrain(window," in world_scene
assert "this.resourceProjectionCache.get(this.civilization, window)" in world_scene
assert "this.groundDetailLayer.setWetness" in world_scene
assert "this.groundDetailLayer.dispose()" in world_scene

# Legacy provenance remains for comparison; production uses local procedural trees.
for token in (
    "tree1.glb",
    "tree2.glb",
    "tree3.glb",
    "CC0-1.0",
    "TREE_ASSET_VARIANTS",
):
    assert token in profile or token in readme, (
        f"missing tree variant/provenance token: {token}"
    )

assert "PROCEDURAL_TREE_VARIANTS.map" in vegetation
assert "variantCounts" in vegetation
assert "tree-asset-layer" not in vegetation
assert "modelUrl" not in vegetation
procedural = (root / "web/src/render/procedural-tree-layer.ts").read_text(encoding="utf-8")
generator = (root / "web/src/render/procedural-tree-generator.ts").read_text(encoding="utf-8")
for token in ("InstancedMesh", "acquireTreeGeometry", "llWind", "llLeafDensity", "setEnvironment"):
    assert token in procedural
for species in ("oak", "birch", "pine"):
    assert f"id: '{species}'" in generator

assert "treeVariantFoliageTints" in vegetation
assert "this.treeVariantFoliageTints[index]" in vegetation
for warm_color in ("0xc19a46", "0xb84f58", "0xc96b78"):
    assert warm_color in profile, (
        f"missing visible warm foliage palette color: {warm_color}"
    )

print("LifeLens web presentation v2 environment foundation: PASS")


