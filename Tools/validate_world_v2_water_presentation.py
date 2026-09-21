#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "Source/LifeLens/WorldPresentation/LLWaterPresentationActor.h").read_text(encoding="utf-8")
cpp = (root / "Source/LifeLens/WorldPresentation/LLWaterPresentationActor.cpp").read_text(encoding="utf-8")

for token in (
    "ResolveObserverCenterChunk",
    "BuiltObserverCenterChunkX",
    "BuiltObserverCenterChunkY",
    "RefreshIntervalSeconds = 0.5f",
):
    assert token in header, f"missing observer-centered water state: {token}"

for token in (
    '#include "World/LLWorldStreamingSubsystem.h"',
    "GetSurfaceWaterPreviewObservationsAroundChunk(",
    "LLTerrainPresentationContract::RegionalPreviewRadiusChunks",
    "ObserverCenterChunk.X",
    "ObserverCenterChunk.Y",
    "PresentationLocationForLogicalChunk",
    "SpawnedWaterZone->SetActorLocation",
):
    assert token in cpp, f"water presentation is not following ObserverInterest: {token}"

refresh_start = cpp.index("void ALLWaterPresentationActor::RefreshFromCore")
refresh = cpp[refresh_start:]
assert "GetMaterializedSurfaceWaterPresentationObservations()" not in refresh

# Visual scrolling must not mutate simulation authority.
for forbidden in (
    "materializeNaturalChunk(",
    "generatedNaturalChunks.push",
    "resourceNodes.push",
    "facilities.push",
):
    assert forbidden not in refresh, f"water presentation mutates Core: {forbidden}"

# Remote water must still resolve terrain height from deterministic preview.
for token in (
    "GetTerrainPreviewObservation(",
    "LLTerrainPresentationContract::RegionalSurfaceZUU",
):
    assert token in cpp

# WaterZone movement is restricted to the zone owned by this actor; authored
# map zones must not be dragged around by observer motion.
zone_move = cpp.index("SpawnedWaterZone->SetActorLocation")
zone_context = cpp[max(0, zone_move - 300):zone_move + 350]
assert "bOwnsWaterZone" in zone_context

print("World v2 observer-centered water presentation validation: PASS")
