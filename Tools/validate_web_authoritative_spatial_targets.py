#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (root / path).read_text(encoding="utf-8")

natural = read("Source/LifeLensCore/include/lifelens/NaturalWorldChunk.h")
spatial = read("Source/LifeLensCore/include/lifelens/CivilizationSpatial.h")
context_action = read("Source/LifeLensCore/include/lifelens/ContextAction.h")
context_runtime = read("Source/LifeLensCore/src/ContextActionRuntime.cpp")
simulation_h = read("Source/LifeLensCore/include/lifelens/Simulation.h")
observer = read("Source/LifeLensCore/include/lifelens/CivilizationObserverReadModel.h")
bridge = read("Source/LifeLensCore/src/WebClientBridge.cpp")
types = read("web/src/runtime/core-types.ts")
core_bridge = read("web/src/runtime/core-bridge.ts")
world_session = read("web/src/runtime/world-session.ts")
wasm_bindings = read("Source/LifeLensCore/wasm/LifeLensWebBindings.cpp")
layer = read("web/src/render/authoritative-spatial-target-layer.ts")
water_geometry = read("web/src/render/water-geometry.ts")
vegetation = read("web/src/render/vegetation-layer.ts")
ground_detail = read("web/src/render/ground-detail-layer.ts")
terrain_signature = read("web/src/render/terrain-dressing-signature.ts")
scene = read("web/src/render/world-scene.ts")
renderer = read("web/src/render/world-renderer.ts")
engine = read("web/src/observer-engine.ts")
cues = read("web/src/render/resident-action-context.ts")
spatial_test = read("Source/LifeLensCore/tests/test_civilization_spatial_targets.cpp")
presentation_test = read("web/tests/presentation/run.mjs")
action_test = read("web/tests/action-context/run.mjs")

materials_block = natural.split(
    "const std::array<MaterialKind, 9> materials = {",
    1,
)[1].split("};", 1)[0]
core_natural_materials = set(re.findall(r"MaterialKind::([A-Za-z0-9_]+)", materials_block))
assert "Water" in core_natural_materials
core_non_water_materials = core_natural_materials - {"Water"}

resource_list = layer.split(
    "export const AUTHORITATIVE_NATURAL_RESOURCE_MATERIALS = [",
    1,
)[1].split("] as const;", 1)[0]
web_natural_materials = set(re.findall(r"'([A-Za-z0-9_]+)'", resource_list))
assert web_natural_materials == core_non_water_materials, (
    f"Core/Web natural resource visual coverage differs: "
    f"Core={sorted(core_non_water_materials)} Web={sorted(web_natural_materials)}"
)

# Water uses the separately validated hydrology footprint. Do not duplicate it
# as a generic resource prop.
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
    "hasAccessGrid",
    "accessGridX",
    "accessGridY",
):
    assert token in bridge, f"Web bridge resource access JSON missing: {token}"

for token in (
    "hasAccessGrid?: boolean",
    "accessGridX?: number",
    "accessGridY?: number",
):
    assert token in types, f"Web resource type access field missing: {token}"

# Coordinate-heavy resource observation is viewport-windowed before access
# resolution/JSON serialization. Global aggregate counts remain world-wide.
for token in (
    "CivilizationResourceObservationWindow",
    "resourceWindow!=nullptr && !resourceWindow->contains(node.pos)",
    "++dto.resourceNodeCount",
    "dto.totalResourceUnits+=",
):
    assert token in observer, f"windowed resource observer contract missing: {token}"

for token in (
    "civilizationWorldWindowJson",
    "observeCivilizationWorldWindow",
):
    assert token in bridge, f"windowed Web bridge contract missing: {token}"
assert 'function("civilizationWorldWindowJson"' in wasm_bindings
assert "civilizationWorldWindowJson?:" in types
assert "civilizationWorldWindow(" in core_bridge
assert "this.core.civilizationWorldWindow(" in world_session
assert "worldActivityWindowKey" in world_session

# Projection batches build one terrain lookup for the whole visible resource
# set and skip repeated 500 ms work for unchanged snapshot references.
for token in (
    "createAuthoritativeGridProjector",
    "const sampleElevation = createTerrainElevationSampler(terrain)",
    "resourcesRef === this.lastResourcesRef",
    "sanitationRef === this.lastSanitationRef",
    "visibleTargetSignature",
):
    assert token in layer, f"spatial target projection optimization missing: {token}"

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

# Ground dressing uses the same exact visible-water footprint instead of
# blanking whole River/Lake/Stream/Spring chunks.
assert "createVisibleWaterFootprintTester" in water_geometry
for source, name in (
    (vegetation, "vegetation"),
    (ground_detail, "ground detail"),
):
    assert "createVisibleWaterFootprintTester" in source, (
        f"{name} does not consume the shared water footprint"
    )
    assert "isInsideVisibleWater(worldX, worldZ)" in source, (
        f"{name} is not filtering individual placements against water"
    )

# Repeated 500 ms observer refreshes must not rebuild every natural-dressing
# instance when terrain/facility inputs are unchanged.
assert "terrainDressingSignature" in terrain_signature
for source, name in (
    (vegetation, "vegetation"),
    (ground_detail, "ground detail"),
):
    assert "terrainDressingSignature(window)" in source, (
        f"{name} is missing terrain dressing cache"
    )
    assert "if (nextSignature === this.terrainSignature) return;" in source, (
        f"{name} does not skip unchanged terrain rebuilds"
    )

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
    "river chunks suppress dressing only on the visible channel footprint",
    "fresh lake and wetland leave dry room outside their localized water radius",
):
    assert token in presentation_test, f"Web target regression missing: {token}"

assert "sanitation wording distinguishes real sites from outdoor fallback" in action_test

print("LifeLens authoritative spatial target visibility: PASS")
