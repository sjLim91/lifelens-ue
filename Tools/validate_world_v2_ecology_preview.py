#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
types = (root / "Source/LifeLens/Simulation/LLWorldGenerationReadTypes.h").read_text(encoding="utf-8")
bridge = (root / "Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp").read_text(encoding="utf-8")

for token in (
    "EcologyBiome",
    "EcologyMoisture01",
    "EcologyTemperature01",
    "ForestCoverage01",
    "GrassCoverage01",
    "ShrubCoverage01",
    "RockCoverage01",
    "WetlandCoverage01",
):
    assert token in types, f"Unreal ecology preview DTO missing: {token}"

for token in (
    '#include "lifelens/ContinuousEcology.h"',
    "deriveContinuousEcologySample(",
    "continuousEcologyBiomeName(",
    "Ecology.forestCoverage01",
    "Ecology.grassCoverage01",
    "Ecology.shrubCoverage01",
    "Ecology.rockCoverage01",
    "Ecology.wetlandCoverage01",
):
    assert token in bridge, f"Unreal ecology preview provider missing: {token}"

# Provider is read-only deterministic preview. It must not mutate/materialize
# resources simply because Observer/Presentation asks for ecology.
fill_start = bridge.index("void FillTerrainPresentationObservation(")
fill_end = bridge.index("void FillNaturalChunkObservation(", fill_start)
fill = bridge[fill_start:fill_end]
for forbidden in (
    "materializeNaturalChunk",
    "createResource",
    "spawnResource",
    "resourceNodes.push_back",
):
    assert forbidden not in fill, f"ecology preview crossed simulation authority: {forbidden}"

print("World v2 Unreal ecology-preview provider validation: PASS")
