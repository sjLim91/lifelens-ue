#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
hydrology = (root / "Source/LifeLensCore/include/lifelens/Hydrology.h").read_text(encoding="utf-8")
spatial = (root / "Source/LifeLensCore/include/lifelens/CivilizationSpatial.h").read_text(encoding="utf-8")
context = (root / "Source/LifeLensCore/include/lifelens/ContextAction.h").read_text(encoding="utf-8")
read_types = (root / "Source/LifeLens/Simulation/LLWorldGenerationReadTypes.h").read_text(encoding="utf-8")
bridge_h = (root / "Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h").read_text(encoding="utf-8")
bridge_cpp = (root / "Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp").read_text(encoding="utf-8")
action_bridge = (root / "Source/LifeLens/Simulation/LLCoreActionBridge.cpp").read_text(encoding="utf-8")
director = (root / "Source/LifeLens/World/LLWorldDirector.cpp").read_text(encoding="utf-8")

for token in (
    "struct SurfaceWaterGroundTraversalProfile",
    "deriveSurfaceWaterGroundTraversalProfile",
    "surfaceWaterGroundContainsGrid",
    "surfaceWaterGroundAccessGrid",
    "blocksGroundTraversal",
    "halfWidthCells",
    "radiusCells",
):
    assert token in hydrology, f"missing Core freshwater traversal contract: {token}"

# Gameplay footprint remains a Core contract. Rendering hints are still allowed
# in the presentation DTO but must not be used as A* authority.
assert "struct FLLCoreFreshSurfaceWaterTraversalObservation" in read_types
for token in (
    "bBlocksGroundTraversal",
    "GroundHalfWidthCells",
    "GroundRadiusCells",
    "bHasGroundAccessTarget",
    "GroundAccessGridX",
    "GroundAccessGridY",
):
    assert token in read_types, f"missing traversal DTO field: {token}"

for token in (
    "FillFreshSurfaceWaterTraversalObservation",
    "deriveSurfaceWaterGroundTraversalProfile",
    "surfaceWaterGroundAccessGrid",
    "GetMaterializedFreshSurfaceWaterTraversalObservations",
):
    assert token in bridge_cpp or token in bridge_h, (
        f"freshwater traversal bridge missing: {token}"
    )

for token in (
    "resolveCivilizationResourceAccessGridPosition",
    "node->material != MaterialKind::Water",
    "surfaceWaterGroundAccessGrid",
    "surfaceWaterGroundContainsGrid",
    "deriveNaturalPhysicalObstacles",
    "world.facilities",
    "isEnvironmentBlocked",
    "validAccess",
    "chunkCoordForGrid(candidate)!=coord",
):
    assert token in spatial, f"water resource access contract missing: {token}"

resolver_start = context.index("inline bool resolveCivilizationContextTarget(")
gather_start = context.index("case CivilizationIntent::Gather:", resolver_start)
gather_end = context.index("case CivilizationIntent::Store:", gather_start)
gather = context[gather_start:gather_end]
assert "resolveCivilizationResourceAccessGridPosition" in gather
assert "resolveCivilizationResourceGridPosition" not in gather

# Bridge fallback must agree with the Core pending target when a legacy/read
# observation lacks a spatial target.
assert "resolveCivilizationResourceAccessGridPosition" in action_bridge

# A dry bank is not sufficient if a tree/rock/facility occupies the same
# A* grid cell. Core must deterministically reroute to another local bank.
for token in (
    "for(int extra=0;extra<=4;++extra)",
    "for(std::size_t ordinal=0;ordinal<directions.size();++ordinal)",
    "if(validAccess(candidate))",
):
    assert token in spatial, f"freshwater bank fallback search missing: {token}"

astar_start = director.index("TArray<FVector> ALLWorldDirector::BuildLocalAStarPath")
astar_end = director.index("void ALLWorldDirector::MoveResidentToward", astar_start)
astar = director[astar_start:astar_end]
for token in (
    "GetMaterializedFreshSurfaceWaterTraversalObservations",
    "Water.bBlocksGroundTraversal",
    "Water.GroundHalfWidthCells",
    "Water.GroundRadiusCells",
    "Water.GroundAccessGridX",
    "Water.GroundAccessGridY",
):
    assert token in astar, f"A* freshwater traversal missing: {token}"

assert "SuggestedChannelWidthCells" not in astar
assert "SuggestedAreaRadiusCells" not in astar
assert "GetMaterializedSurfaceWaterPresentationObservations" in astar, (
    "marine coastline orientation still needs its existing presentation projection"
)

print("LifeLens authoritative freshwater ground traversal: PASS")
