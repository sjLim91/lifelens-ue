#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (root / path).read_text(encoding="utf-8")

natural = read("Source/LifeLensCore/include/lifelens/NaturalWorldChunk.h")
world = read("Source/LifeLensCore/include/lifelens/World.h")
sim = read("Source/LifeLensCore/src/Simulation.cpp")
codec = read("Source/LifeLensCore/src/SimulationSnapshotCodec.cpp")
wg_codec = read("Source/LifeLensCore/include/lifelens/WorldGenerationSnapshotCodec.h")
test = read("Source/LifeLensCore/tests/test_world_generation_milestone_a.cpp")
cmake = read("Source/LifeLensCore/CMakeLists.txt")

for token in (
    "struct GeneratedNaturalChunk",
    "struct NaturalResourcePatch",
    "deriveGeneratedNaturalChunk",
    "deriveNaturalResourceNodeId",
    "deriveUntouchedChunkBaseline",
    "deriveMacroRegionFacts",
    "resourcePatches",
    "NaturalSurfaceKind",
):
    assert token in natural, f"missing natural chunk contract: {token}"

assert "populationSeed" not in natural, "natural chunk detail must not depend on PopulationSeed"
assert "std::hash<" not in natural, "natural chunk detail must use stable world-genesis mixer"
assert "bHydrologyAlignedResources = identity.generationVersion >= 2" in natural
assert "isFreshSurfaceWater(hydrology)" in natural
assert "(bHydrologyAlignedResources ? 1 : 3)" in natural
assert "WorldChunkSpanGridCells / 2" in natural

for token in (
    "generatedNaturalChunks",
    "materializeNaturalChunk",
    "establishInitialStartRegion",
    "initialStartRegionCenterGrid",
):
    assert token in world, f"missing World materialization contract: {token}"

for token in (
    "world_.storageSites.clear()",
    "world_.materializeNaturalChunk",
    "initialRuntime.pos",
):
    assert token in sim, f"missing production New Game integration: {token}"

assert "WorldGenerationSnapshotExtensionMagic" in wg_codec
assert "writeWorldGenerationSnapshotExtension" in codec
assert "readWorldGenerationSnapshotExtension" in codec
assert "validateWorldGenerationSnapshotState" in codec

assert "lifelens_add_test(test_world_generation_milestone_a)" in cmake
for token in (
    "sameGeneratedNaturalChunkBaseline",
    "storageSites.empty()",
    "bytes==reencoded",
    "chunkCoordForGrid(pos)==selected.region.coord",
):
    assert token in test, f"missing milestone regression coverage: {token}"

print("World Generation Milestone A Core structural validation: PASS")
