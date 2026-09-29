#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (root / path).read_text(encoding="utf-8")

natural = read("Source/LifeLensCore/include/lifelens/NaturalWorldChunk.h")
spatial = read("Source/LifeLensCore/include/lifelens/CivilizationSpatial.h")
observer = read("Source/LifeLensCore/include/lifelens/CivilizationObserverReadModel.h")
bridge = read("Source/LifeLensCore/src/WebClientBridge.cpp")
types = read("web/src/runtime/core-types.ts")
layer = read("web/src/render/authoritative-spatial-target-layer.ts")
scene = read("web/src/render/world-scene.ts")
renderer = read("web/src/render/world-renderer.ts")
engine = read("web/src/observer-engine.ts")
cues = read("web/src/render/resident-action-context.ts")
spatial_test = read("Source/LifeLensCore/tests/test_civilization_spatial_targets.cpp")
presentation_test = read("web/tests/presentation/run.mjs")
action_test = read("web/tests/action-context/run.mjs")

natural_materials = (
    "Wood",
    "Stone",
    "Flint",
    "Fiber",
    "Clay",
    "PlantFood",
    "CopperOre",
    "TinOre",
)
for material in natural_materials:
    assert f"MaterialKind::{material}" in natural, (
        f"Core natural material missing from generation catalogue: {material}"
    )
    assert f"'{material}'" in layer, (
        f"Web authoritative target layer missing natural material: {material}"
    )

# Water uses the separately validated hydrology footprint. Do not duplicate it
# as a generic resource prop.
resource_list = layer.split(
    "export const AUTHORITATIVE_NATURAL_RESOURCE_MATERIALS = [",
    1,
)[1].split("] as const;", 1)[0]
assert "'Water'" not in resource_list
assert "WaterLayer" in scene

# Every resource has a deterministic interaction point. Non-water patches may
# stay on their immutable generated node when safe, but must fall back to a dry,
# same-chunk access position when blocked by water/physical world state.
for token in (
    "node->material != MaterialKind::Water",
    "if(validAccess(exact))",
    "for(int distance=1;distance<=10;++distance)",
    "!surfaceWaterGroundContainsGrid(facts,candidate)",
    "chunkCoordForGrid(candidate)!=coord",
):
    assert token in spatial, f"missing generic resource access guard: {token}"

for token in (
    "hasAccessPos",
    "accessPos",
    "resolveCivilizationResourceAccessGridPosition",
):
    assert token in observer, f"observer resource access contract missing: {token}"

for token in (
    '"hasAccessGrid":',
    '"accessGridX":',
    '"accessGridY":',
):
    assert token in bridge, f"Web bridge resource access JSON missing: {token}"

for token in (
    "hasAccessGrid?: boolean",
    "accessGridX?: number",
    "accessGridY?: number",
):
    assert token in types, f"Web resource type access field missing: {token}"

# Presentation must project actual Core access coordinates, never random
# scenery, and must hide depleted resources.
for token in (
    "resource.hasAccessGrid === true",
    "resource.accessGridX",
    "resource.accessGridY",
    "resource.quantity <= 0",
    "authoritativeGridWorldPosition",
    "visibleAuthoritativeResourceSites",
    "visibleAuthoritativeSanitationSites",
    "authoritative-spatial-targets",
):
    assert token in layer, f"authoritative target projection missing: {token}"
assert "Math.random(" not in layer

# Resource/sanitation authority has to reach the live Three.js scene.
for token in (
    "AuthoritativeSpatialTargetLayer",
    "authoritativeSpatialTargetLayer.setTargets",
):
    assert token in scene, f"WorldScene target layer wiring missing: {token}"
assert "setAuthoritativeSpatialTargets" in renderer
for token in (
    "snapshot.civilization",
    "snapshot.worldObjects",
    "setAuthoritativeSpatialTargets",
):
    assert token in engine, f"observer engine target feed missing: {token}"

# Empty ground is only truthful when the action itself is explicitly exploratory
# or an emergency outdoor fallback.
for token in (
    "있는 곳으로 이동 중",
    "탐색 지역으로 이동 중",
    "위생 장소로 이동 중",
    "야외 용변 장소로 이동 중",
):
    assert token in cues, f"target-aware action wording missing: {token}"

for token in (
    "blockedStone",
    "!surfaceWaterGroundContainsGrid",
    "checkedNonWaterAccess",
):
    assert token in spatial_test, f"Core resource access regression missing: {token}"

for token in (
    "all natural resource interaction targets project from Core access coordinates",
    "resource and resident grid projection share the exact world coordinate contract",
    "active sanitation sites render at their exact Core target",
):
    assert token in presentation_test, f"Web target regression missing: {token}"

assert "sanitation wording distinguishes real sites from outdoor fallback" in action_test

print("LifeLens authoritative spatial target visibility: PASS")
