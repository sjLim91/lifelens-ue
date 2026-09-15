#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / 'Source/LifeLensCore/include/lifelens/WorldGenesis.h').read_text(encoding='utf-8')
world = (root / 'Source/LifeLensCore/include/lifelens/World.h').read_text(encoding='utf-8')
simulation_h = (root / 'Source/LifeLensCore/include/lifelens/Simulation.h').read_text(encoding='utf-8')
simulation_cpp = (root / 'Source/LifeLensCore/src/Simulation.cpp').read_text(encoding='utf-8')
test = (root / 'Source/LifeLensCore/tests/test_world_genesis_wg1.cpp').read_text(encoding='utf-8')
cmake = (root / 'Source/LifeLensCore/CMakeLists.txt').read_text(encoding='utf-8')
doc = (root / 'docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md').read_text(encoding='utf-8')

required_header = [
    'using WorldSeed',
    'using PopulationSeed',
    'CurrentWorldGenerationVersion',
    'struct ChunkCoord',
    'WorldChunkSpanGridCells',
    'floorDivWorldGrid',
    'deriveChunkSeed',
    'deriveUntouchedChunkBaseline',
    'terrainStreamSeed',
    'climateStreamSeed',
    'resourceStreamSeed',
]
for token in required_header:
    assert token in header, f'missing WG-1 header contract: {token}'
assert 'std::hash<' not in header, 'stable chunk identity must not depend on implementation-defined std::hash calls'
assert 'PopulationSeed populationSeed' in world
assert 'WorldGenerationVersion generationVersion' in world
assert 'WorldGenesisIdentity genesisIdentity() const' in world
assert 'PopulationSeed populationSeed=0' in simulation_h
assert 'std::mt19937_64 populationRng(world_.populationSeed);' in simulation_cpp
assert 'generateInitialFounders(populationRng,world_.minute)' in simulation_cpp
assert 'familiarity(populationRng)' in simulation_cpp
assert 'generateInitialFounders(world_.rng' not in simulation_cpp
assert 'lifelens_add_test(test_world_genesis_wg1)' in cmake
assert 'reverseOrder' in test and 'deriveUntouchedChunkBaseline' in test
assert 'ChunkCoord{-1,-1}' in test and 'ChunkCoord{-2,-2}' in test
assert 'founderSignature(first) != founderSignature(second)' in test
assert 'WorldSeed + ChunkCoord' in doc or 'WorldSeed + GenerationVersion + ChunkCoord' in doc
print('World Genesis WG-1 structural validation: PASS')
