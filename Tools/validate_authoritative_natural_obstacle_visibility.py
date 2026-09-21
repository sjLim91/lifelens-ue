#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
world = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")
proxy = (root / "Source/LifeLens/World/LLWorldObstacleCollisionProxyActor.cpp").read_text(encoding="utf-8")

# Core obstacle truth must drive both the invisible physical proxy and a visible
# natural mesh at the same authoritative XY. Ambient ecology remains decoration.
for token in (
    "Chunk.PhysicalObstacles.Num()",
    "for (const FLLCoreNaturalObstacleObservation& Obstacle : Chunk.PhysicalObstacles)",
    "Obstacle.ObstacleId",
    "Obstacle.SourceResourceNodeId",
    "Obstacle.Kind",
    "Obstacle.GridX",
    "Obstacle.GridY",
    "Obstacle.OffsetXCells",
    "Obstacle.OffsetYCells",
    "Obstacle.HalfExtentXCells",
    "Obstacle.HalfExtentYCells",
    "Obstacle.HalfHeightCells",
):
    assert token in world, f"visible natural obstacle contract missing: {token}"
    assert token in proxy or token == "Obstacle.SourceResourceNodeId", (
        f"physical obstacle proxy drifted from visible obstacle inputs: {token}"
    )

# Exact Core XY formula must remain aligned with the collision proxy. The visual
# layer may change art/scale/yaw but not obstacle identity/location.
for token in (
    "Obstacle.GridX - World.InitialCenterGridX",
    "Obstacle.GridY - World.InitialCenterGridY",
    "+ Obstacle.OffsetXCells",
    "+ Obstacle.OffsetYCells",
    "LLWorldSpatialContract::GridCellSizeUU",
    "TerrainSurfaceZUU(World, Terrain, ObstacleLocation)",
):
    assert token in world, f"visible obstacle coordinate drift: {token}"

for token in (
    "RelativeXCells",
    "Obstacle.GridX - World.InitialCenterGridX",
    "Obstacle.OffsetXCells",
    "RelativeYCells",
    "Obstacle.GridY - World.InitialCenterGridY",
    "Obstacle.OffsetYCells",
):
    assert token in proxy, f"collision proxy coordinate contract missing: {token}"

# Authoritative visuals are placed before ambient density filling, so density
# caps can suppress decoration but never hide a real movement blocker.
authoritative = world.index(
    "for (const FLLCoreNaturalObstacleObservation& Obstacle : Chunk.PhysicalObstacles)"
)
ambient = world.index(
    "Place(TreeInstances, TreeCount",
    authoritative,
)
assert authoritative < ambient

# WorldPresentation art itself must remain non-authoritative for collision.
# LLWorldObstacleCollisionProxyActor is the sole movement-collision projection.
component_start = world.index(
    "UHierarchicalInstancedStaticMeshComponent* ALLWorldPresentationActor::AddInstancedComponent"
)
component_end = world.index(
    "void ALLWorldPresentationActor::AddPhotorealStructureLog",
    component_start,
)
component = world[component_start:component_end]
assert "SetCollisionEnabled(ECollisionEnabled::NoCollision)" in component
assert "SetCanEverAffectNavigation(false)" in component

# Integration guard with the Android visible-terrain grounding audit: local
# terrain may expose query-only WorldStatic geometry for mesh grounding, but
# natural dressing itself must remain NoCollision and Core obstacle proxies
# remain the sole movement blockers.
if "#if PLATFORM_ANDROID" in world:
    assert "EnableVisualGroundQuery" in world
    assert "SetCollisionEnabled(ECollisionEnabled::QueryOnly)" in world
    assert "SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block)" in world
    assert "SetCollisionEnabled(ECollisionEnabled::NoCollision)" in component

print("LifeLens authoritative natural obstacle visibility: PASS")
