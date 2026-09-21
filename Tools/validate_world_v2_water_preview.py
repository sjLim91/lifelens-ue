#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
bridge_h = (root / "Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h").read_text(encoding="utf-8")
bridge_cpp = (root / "Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp").read_text(encoding="utf-8")

for token in (
    "GetHydrologyPreviewObservationsAroundChunk",
    "GetSurfaceWaterPreviewObservationsAroundChunk",
):
    assert token in bridge_h
    assert token in bridge_cpp

hydro_start = bridge_cpp.index("ULLCoreBridgeSubsystem::GetHydrologyPreviewObservationsAroundChunk")
hydro_end = bridge_cpp.index("ULLCoreBridgeSubsystem::GetMaterializedSurfaceWaterPresentationObservations", hydro_start)
hydro = bridge_cpp[hydro_start:hydro_end]

for token in (
    "CenterChunkX + X",
    "CenterChunkY + Y",
    "FMath::Clamp(RadiusChunks, 0, 16)",
    "deriveHydrologyFacts(Identity, Coord)",
    "FillHydrologyObservation",
):
    assert token in hydro, f"missing read-only hydrology preview token: {token}"

water_start = bridge_cpp.index("ULLCoreBridgeSubsystem::GetSurfaceWaterPreviewObservationsAroundChunk")
water_end = bridge_cpp.index("ULLCoreBridgeSubsystem::GetMaterializedFreshSurfaceWaterTraversalObservations", water_start)
water = bridge_cpp[water_start:water_end]

for token in (
    "CenterChunkX + X",
    "CenterChunkY + Y",
    "FMath::Clamp(RadiusChunks, 0, 16)",
    "deriveHydrologyFacts(Identity, Coord)",
    "FillSurfaceWaterPresentationObservation",
):
    assert token in water, f"missing read-only surface-water preview token: {token}"

# Observer preview is not simulation interest. It must never author Core state.
for body in (hydro, water):
    for forbidden in (
        "materializeNaturalChunk(",
        "generatedNaturalChunks.push",
        "resourceNodes.push",
        "facilities.push",
        "storageSites.push",
        "findGeneratedNaturalChunk(",
    ):
        assert forbidden not in body, f"observer preview mutates or depends on materialized state: {forbidden}"

# Existing local/materialized APIs remain explicitly materialized authority.
single_start = bridge_cpp.index("bool ULLCoreBridgeSubsystem::GetHydrologyObservation")
single_end = bridge_cpp.index("ULLCoreBridgeSubsystem::GetHydrologyPreviewObservationsAroundChunk", single_start)
assert "findGeneratedNaturalChunk(Coord)" in bridge_cpp[single_start:single_end]

water_single_start = bridge_cpp.index("bool ULLCoreBridgeSubsystem::GetSurfaceWaterPresentationObservation")
water_single_end = bridge_cpp.index("ULLCoreBridgeSubsystem::GetSurfaceWaterPreviewObservationsAroundChunk", water_single_start)
assert "findGeneratedNaturalChunk(Coord)" in bridge_cpp[water_single_start:water_single_end]

print("World v2 observer water-preview provider structural validation: PASS")
