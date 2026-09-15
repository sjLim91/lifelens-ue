#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
macro = (root / 'Source/LifeLensCore/include/lifelens/MacroWorldGenesis.h').read_text(encoding='utf-8')
world = (root / 'Source/LifeLensCore/include/lifelens/World.h').read_text(encoding='utf-8')
test = (root / 'Source/LifeLensCore/tests/test_world_genesis_wg2.cpp').read_text(encoding='utf-8')
cmake = (root / 'Source/LifeLensCore/CMakeLists.txt').read_text(encoding='utf-8')
doc = (root / 'docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md').read_text(encoding='utf-8')

required_macro = [
    'enum class MacroBiome',
    'struct MacroRegionFacts',
    'MacroStartSearchRadiusChunks',
    'macroLatticeValue',
    'macroValueNoise',
    'deriveMacroRegionFacts',
    'scoreInitialStartRegion',
    'selectInitialStartRegion',
    'waterPotential',
    'fertilityPotential',
    'woodPotential',
    'stonePotential',
    'foodPotential',
    'traversalEase',
    'hazardPotential',
]
for token in required_macro:
    assert token in macro, f'missing WG-2 macro-world contract: {token}'

# Natural geography must not consume PopulationSeed or global RNG order.
assert 'populationSeed' not in macro, 'WG-2 macro geography must not depend on PopulationSeed'
assert 'mt19937' not in macro, 'WG-2 macro geography must not depend on mutable RNG streams'
assert 'std::hash<' not in macro, 'WG-2 macro geography must use the fixed world-genesis mixer'

assert '#include "MacroWorldGenesis.h"' in world
assert 'MacroRegionFacts macroRegionFacts(ChunkCoord coord) const' in world
assert 'InitialStartRegionSelection initialStartRegion() const' in world
assert 'lifelens_add_test(test_world_genesis_wg2)' in cmake
assert 'peopleA' in test and 'peopleB' in test
assert 'startA.region.coord==startB.region.coord' in test
assert 'simulation.world().objects.empty()' in test
assert 'simulation.world().primitiveSanitationSites.empty()' in test
assert 'Macro World' in doc and 'start-site' in doc
print('World Genesis WG-2 structural validation: PASS')
