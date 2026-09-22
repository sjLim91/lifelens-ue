#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

def read(path):
    return (root / path).read_text(encoding='utf-8')

natural = read('Source/LifeLensCore/include/lifelens/NaturalWorldChunk.h')
world = read('Source/LifeLensCore/include/lifelens/World.h')
sim = read('Source/LifeLensCore/src/Simulation.cpp')
codec = read('Source/LifeLensCore/src/SimulationSnapshotCodec.cpp')
wg_codec = read('Source/LifeLensCore/include/lifelens/WorldGenerationSnapshotCodec.h')
bridge_h = read('Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h')
bridge = read('Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp')
director = read('Source/LifeLens/World/LLWorldDirector.cpp')
visual = read('Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.cpp')
test = read('Source/LifeLensCore/tests/test_world_generation_milestone_a.cpp')
cmake = read('Source/LifeLensCore/CMakeLists.txt')

for token in [
    'struct GeneratedNaturalChunk', 'struct NaturalResourcePatch',
    'deriveGeneratedNaturalChunk', 'deriveNaturalResourceNodeId',
    'deriveUntouchedChunkBaseline', 'deriveMacroRegionFacts',
    'resourcePatches', 'NaturalSurfaceKind']:
    assert token in natural, f'missing natural chunk contract: {token}'

assert 'populationSeed' not in natural, 'natural chunk detail must not depend on PopulationSeed'
assert 'std::hash<' not in natural, 'natural chunk detail must use stable world-genesis mixer'
assert 'bHydrologyAlignedResources = identity.generationVersion >= 2' in natural, (
    'freshwater/resource alignment must not rewrite v1 generated chunk replay'
)
assert 'isFreshSurfaceWater(hydrology)' in natural, 'v2 Water resource nodes must map to authoritative fresh surface water'
assert '(bHydrologyAlignedResources ? 1 : 3)' in natural, 'v1 Water patch-count contract must remain unchanged'
assert 'WorldChunkSpanGridCells / 2' in natural, 'v2 Water resource node must align with the visible water center'
for token in ['generatedNaturalChunks', 'materializeNaturalChunk', 'establishInitialStartRegion', 'initialStartRegionCenterGrid']:
    assert token in world, f'missing World materialization contract: {token}'
for token in ['world_.storageSites.clear()', 'world_.materializeNaturalChunk', 'initialRuntime.pos']:
    assert token in sim, f'missing production New Game integration: {token}'

assert 'WorldGenerationSnapshotExtensionMagic' in wg_codec
assert 'writeWorldGenerationSnapshotExtension' in codec
assert 'readWorldGenerationSnapshotExtension' in codec
assert 'validateWorldGenerationSnapshotState' in codec

assert 'GetWorldGenerationObservation' in bridge_h
assert 'GetNaturalChunkObservation' in bridge_h
assert 'FillNaturalChunkObservation' in bridge
assert 'InitialCenterGridX' in bridge

assert 'RefreshCorePresentationOrigin' in director
assert 'CorePresentationOriginGrid' in director
assert 'GridX - CorePresentationOriginGrid.X' in director
assert 'GridX - CoreOriginGridX' in visual

assert 'lifelens_add_test(test_world_generation_milestone_a)' in cmake
for token in ['sameGeneratedNaturalChunkBaseline', 'storageSites.empty()', 'bytes==reencoded', 'chunkCoordForGrid(pos)==selected.region.coord']:
    assert token in test, f'missing milestone regression coverage: {token}'

print('World Generation Milestone A structural validation: PASS')
