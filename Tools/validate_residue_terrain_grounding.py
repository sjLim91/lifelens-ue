#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.cpp").read_text(encoding="utf-8")
header = (root / "Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.h").read_text(encoding="utf-8")

for token in (
    "LLTerrainPresentationContract::LocalSurfaceZUU",
    "GetWorldGenerationObservation()",
    "GetCivilizationWorldObservation(0)",
    "GetTerrainPresentationObservation",
    "GetTerrainPreviewObservation",
    "FacilityCentersUU",
    "World.InitialCenterGridX",
    "World.InitialCenterGridY",
):
    assert token in cpp, f"missing shared terrain residue projection token: {token}"

# Residue transforms must rebuild when the deterministic world frame or the
# facility flattening envelope changes, even if residue DTOs are unchanged.
sig_start = cpp.index("BuildVisualSignature(")
sig_end = cpp.index("ResolveSurfaceLocation(", sig_start)
sig = cpp[sig_start:sig_end]
for token in (
    "World.WorldSeed",
    "World.GenerationVersion",
    "World.InitialChunkX",
    "World.InitialChunkY",
    "World.InitialCenterGridX",
    "World.InitialCenterGridY",
    "World.InitialChunk.Elevation",
    "Civilization.Facilities.Num()",
    "Facility.FacilityId",
    "Facility.GridX",
    "Facility.GridY",
):
    assert token in sig, f"residue terrain signature misses input: {token}"

# Startup actor ordering must not determine residue Z. The old trace could hit
# the hidden bootstrap floor before terrain presentation actors existed.
surface_start = cpp.index("ResolveSurfaceLocation(")
surface_end = cpp.index("RefreshFromCore(", surface_start)
surface = cpp[surface_start:surface_end]
for forbidden in (
    "LineTraceSingleByChannel",
    "ECC_WorldStatic",
    "SCENE_QUERY_STAT",
):
    assert forbidden not in surface, (
        f"residue surface projection must not regress to actor-order-dependent trace: {forbidden}"
    )

assert "const ULLCoreBridgeSubsystem& CoreBridge" in header
assert "const FLLCoreWorldGenerationObservation& World" in header
assert "const TArray<FVector2D>& FacilityCentersUU" in header

print("LifeLens environmental residue shared terrain grounding: PASS")
