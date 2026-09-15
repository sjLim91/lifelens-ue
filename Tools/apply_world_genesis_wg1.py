from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise SystemExit(f'missing patch anchor: {label}')
    if text.count(old) != 1:
        raise SystemExit(f'non-unique patch anchor: {label} ({text.count(old)})')
    return text.replace(old, new, 1)

# New Core world-genesis contract.
Path('Source/LifeLensCore/include/lifelens/WorldGenesis.h').write_text(r'''#pragma once

#include <cstdint>

#include "SmartObject.h"

namespace lifelens {

using WorldSeed = std::uint64_t;
using PopulationSeed = std::uint64_t;
using WorldGenerationVersion = std::uint32_t;

inline constexpr WorldGenerationVersion CurrentWorldGenerationVersion = 1;

// Logical Core grid cells per chunk. This is a simulation-coordinate contract,
// not a statement that one grid cell equals one physical meter in presentation.
inline constexpr int WorldChunkSpanGridCells = 32;

struct ChunkCoord {
    int x = 0;
    int y = 0;

    friend bool operator==(ChunkCoord a, ChunkCoord b)
    {
        return a.x == b.x && a.y == b.y;
    }

    friend bool operator!=(ChunkCoord a, ChunkCoord b)
    {
        return !(a == b);
    }

    friend bool operator<(ChunkCoord a, ChunkCoord b)
    {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    }
};

struct WorldGenesisIdentity {
    WorldSeed worldSeed = 1;
    PopulationSeed populationSeed = 1;
    WorldGenerationVersion generationVersion = CurrentWorldGenerationVersion;
};

struct UntouchedChunkBaseline {
    ChunkCoord coord{};
    std::uint64_t chunkSeed = 0;
    std::uint64_t terrainStreamSeed = 0;
    std::uint64_t climateStreamSeed = 0;
    std::uint64_t resourceStreamSeed = 0;
    std::uint64_t detailStreamSeed = 0;

    friend bool operator==(const UntouchedChunkBaseline& a, const UntouchedChunkBaseline& b)
    {
        return a.coord == b.coord
            && a.chunkSeed == b.chunkSeed
            && a.terrainStreamSeed == b.terrainStreamSeed
            && a.climateStreamSeed == b.climateStreamSeed
            && a.resourceStreamSeed == b.resourceStreamSeed
            && a.detailStreamSeed == b.detailStreamSeed;
    }

    friend bool operator!=(const UntouchedChunkBaseline& a, const UntouchedChunkBaseline& b)
    {
        return !(a == b);
    }
};

inline WorldSeed normalizeWorldSeed(WorldSeed seed)
{
    return seed == 0 ? 1 : seed;
}

inline WorldGenerationVersion normalizeWorldGenerationVersion(WorldGenerationVersion version)
{
    return version == 0 ? CurrentWorldGenerationVersion : version;
}

// Fixed SplitMix64 finalizer. Unlike std::hash this contract is intentionally
// specified and therefore stable across platforms/processes for a given input.
inline std::uint64_t worldGenesisMix64(std::uint64_t value)
{
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

inline std::uint64_t stableSignedCoordinateWord(int value)
{
    const std::int64_t wide = static_cast<std::int64_t>(value);
    return wide >= 0
        ? static_cast<std::uint64_t>(wide) * 2ULL
        : static_cast<std::uint64_t>(-(wide + 1)) * 2ULL + 1ULL;
}

inline std::uint64_t combineWorldGenesisWord(std::uint64_t state, std::uint64_t value)
{
    return worldGenesisMix64(state ^ worldGenesisMix64(value));
}

inline PopulationSeed deriveDefaultPopulationSeed(WorldSeed worldSeed)
{
    constexpr std::uint64_t PopulationDomain = 0x504f50554c415449ULL; // "POPULATI"
    return worldGenesisMix64(normalizeWorldSeed(worldSeed) ^ PopulationDomain);
}

inline WorldGenesisIdentity makeWorldGenesisIdentity(
    WorldSeed worldSeed,
    PopulationSeed populationSeed = 0,
    WorldGenerationVersion generationVersion = CurrentWorldGenerationVersion)
{
    WorldGenesisIdentity identity;
    identity.worldSeed = normalizeWorldSeed(worldSeed);
    identity.populationSeed = populationSeed == 0
        ? deriveDefaultPopulationSeed(identity.worldSeed)
        : populationSeed;
    identity.generationVersion = normalizeWorldGenerationVersion(generationVersion);
    return identity;
}

inline int floorDivWorldGrid(int value, int positiveDivisor)
{
    const std::int64_t divisor = positiveDivisor > 0 ? positiveDivisor : 1;
    const std::int64_t wide = value;
    std::int64_t quotient = wide / divisor;
    const std::int64_t remainder = wide % divisor;
    if (remainder < 0) --quotient;
    return static_cast<int>(quotient);
}

inline ChunkCoord chunkCoordForGrid(GridPos position)
{
    return {
        floorDivWorldGrid(position.x, WorldChunkSpanGridCells),
        floorDivWorldGrid(position.y, WorldChunkSpanGridCells)
    };
}

inline GridPos chunkOriginGrid(ChunkCoord coord)
{
    return {
        coord.x * WorldChunkSpanGridCells,
        coord.y * WorldChunkSpanGridCells
    };
}

inline GridPos chunkLocalGrid(GridPos position)
{
    const ChunkCoord coord = chunkCoordForGrid(position);
    const GridPos origin = chunkOriginGrid(coord);
    return {position.x - origin.x, position.y - origin.y};
}

inline std::uint64_t deriveChunkSeed(
    WorldSeed worldSeed,
    ChunkCoord coord,
    WorldGenerationVersion generationVersion = CurrentWorldGenerationVersion)
{
    constexpr std::uint64_t ChunkDomain = 0x4348554e4b5f5731ULL; // "CHUNK_W1"
    std::uint64_t state = worldGenesisMix64(normalizeWorldSeed(worldSeed) ^ ChunkDomain);
    state = combineWorldGenesisWord(state, normalizeWorldGenerationVersion(generationVersion));
    state = combineWorldGenesisWord(state, stableSignedCoordinateWord(coord.x));
    state = combineWorldGenesisWord(state, stableSignedCoordinateWord(coord.y));
    return state;
}

inline std::uint64_t deriveChunkStreamSeed(std::uint64_t chunkSeed, std::uint64_t domain)
{
    return worldGenesisMix64(chunkSeed ^ domain);
}

inline UntouchedChunkBaseline deriveUntouchedChunkBaseline(
    WorldSeed worldSeed,
    ChunkCoord coord,
    WorldGenerationVersion generationVersion = CurrentWorldGenerationVersion)
{
    constexpr std::uint64_t TerrainDomain = 0x5445525241494e31ULL; // "TERRAIN1"
    constexpr std::uint64_t ClimateDomain = 0x434c494d41544531ULL; // "CLIMATE1"
    constexpr std::uint64_t ResourceDomain = 0x5245534f55524331ULL; // "RESOURC1"
    constexpr std::uint64_t DetailDomain = 0x44455441494c5f31ULL; // "DETAIL_1"

    UntouchedChunkBaseline baseline;
    baseline.coord = coord;
    baseline.chunkSeed = deriveChunkSeed(worldSeed, coord, generationVersion);
    baseline.terrainStreamSeed = deriveChunkStreamSeed(baseline.chunkSeed, TerrainDomain);
    baseline.climateStreamSeed = deriveChunkStreamSeed(baseline.chunkSeed, ClimateDomain);
    baseline.resourceStreamSeed = deriveChunkStreamSeed(baseline.chunkSeed, ResourceDomain);
    baseline.detailStreamSeed = deriveChunkStreamSeed(baseline.chunkSeed, DetailDomain);
    return baseline;
}

inline UntouchedChunkBaseline deriveUntouchedChunkBaseline(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord)
{
    // PopulationSeed is deliberately excluded. Natural geography must stay the
    // same when replaying a world with different initial residents.
    return deriveUntouchedChunkBaseline(identity.worldSeed, coord, identity.generationVersion);
}

} // namespace lifelens
''', encoding='utf-8')

# World owns the separated identity while retaining `seed` for compatibility.
p = Path('Source/LifeLensCore/include/lifelens/World.h')
t = p.read_text(encoding='utf-8')
t = replace_once(t,
    '#include "PrimitiveSanitation.h"\n',
    '#include "PrimitiveSanitation.h"\n#include "WorldGenesis.h"\n',
    'World include')
t = replace_once(t,
    '    std::uint64_t seed=1;\n    std::mt19937_64 rng{1};\n',
    '    // `seed` remains the compatibility spelling for authoritative WorldSeed.\n    std::uint64_t seed=1;\n    PopulationSeed populationSeed=1;\n    WorldGenerationVersion generationVersion=CurrentWorldGenerationVersion;\n    std::mt19937_64 rng{1};\n',
    'World seed fields')
t = replace_once(t,
    '    explicit World(std::uint64_t s=1) : seed(s?s:1), rng(seed)\n    {\n        resetCivilizationEnvironment();\n    }\n',
    '''    explicit World(\n        WorldSeed worldSeed=1,\n        PopulationSeed initialPopulationSeed=0,\n        WorldGenerationVersion initialGenerationVersion=CurrentWorldGenerationVersion)\n    {\n        const WorldGenesisIdentity identity=makeWorldGenesisIdentity(\n            worldSeed,initialPopulationSeed,initialGenerationVersion);\n        seed=identity.worldSeed;\n        populationSeed=identity.populationSeed;\n        generationVersion=identity.generationVersion;\n        rng.seed(seed);\n        resetCivilizationEnvironment();\n    }\n\n    WorldGenesisIdentity genesisIdentity() const\n    {\n        return {seed,populationSeed,generationVersion};\n    }\n\n    UntouchedChunkBaseline untouchedChunkBaseline(ChunkCoord coord) const\n    {\n        return deriveUntouchedChunkBaseline(genesisIdentity(),coord);\n    }\n''',
    'World constructor')
p.write_text(t, encoding='utf-8')

# Simulation API explicitly accepts the independent initial-population seed.
p = Path('Source/LifeLensCore/include/lifelens/Simulation.h')
t = p.read_text(encoding='utf-8')
t = replace_once(t,
    '    explicit Simulation(std::uint64_t seed=1);\n',
    '''    explicit Simulation(\n        WorldSeed worldSeed=1,\n        PopulationSeed populationSeed=0,\n        WorldGenerationVersion generationVersion=CurrentWorldGenerationVersion);\n''',
    'Simulation ctor declaration')
p.write_text(t, encoding='utf-8')

# setupNewGame must not consume the world RNG to build founders.
p = Path('Source/LifeLensCore/src/Simulation.cpp')
t = p.read_text(encoding='utf-8')
t = replace_once(t,
    'Simulation::Simulation(std::uint64_t seed):world_(seed){}\n',
    '''Simulation::Simulation(\n    WorldSeed worldSeed,\n    PopulationSeed populationSeed,\n    WorldGenerationVersion generationVersion)\n    :world_(worldSeed,populationSeed,generationVersion){}\n''',
    'Simulation ctor definition')
t = replace_once(t,
    '''    world_.rng.seed(world_.seed);\n    world_.characters=generateInitialFounders(world_.rng,world_.minute);\n\n    std::uniform_real_distribution<double> familiarity(0.0,0.04);\n''',
    '''    // World randomness and initial-population randomness are separate.\n    // Founder generation must not advance the world RNG or affect future chunk\n    // baselines merely because names/traits were regenerated.\n    world_.rng.seed(world_.seed);\n    std::mt19937_64 populationRng(world_.populationSeed);\n    world_.characters=generateInitialFounders(populationRng,world_.minute);\n\n    std::uniform_real_distribution<double> familiarity(0.0,0.04);\n''',
    'setupNewGame population rng')
t = replace_once(t,
    '            relation.familiarity=familiarity(world_.rng);\n',
    '            relation.familiarity=familiarity(populationRng);\n',
    'initial relationship population rng')
t = replace_once(t,
    '    emit("new game start seed="+std::to_string(world_.seed)+" founders=4");\n',
    '    emit("new game start seed="+std::to_string(world_.seed)+" populationSeed="+std::to_string(world_.populationSeed)+" founders=4");\n',
    'new game log')
p.write_text(t, encoding='utf-8')

# Core regression/contract test.
Path('Source/LifeLensCore/tests/test_world_genesis_wg1.cpp').write_text(r'''#include "lifelens/Simulation.h"
#include "lifelens/WorldGenesis.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace lifelens;

namespace {

std::string founderSignature(const Simulation& simulation)
{
    std::string result;
    for(const Character& character:simulation.world().characters){
        result += character.name + ":";
        result += std::to_string(static_cast<int>(character.sex)) + ":";
        result += std::to_string(character.personality.curiosity) + ":";
        result += std::to_string(character.genetics.learningPotential) + "|";
    }
    return result;
}

} // namespace

int main()
{
    static_assert(CurrentWorldGenerationVersion == 1, "WG-1 baseline version changed unexpectedly");
    static_assert(WorldChunkSpanGridCells > 0, "chunk span must be positive");

    // Chunk mapping must use floor semantics for negative world-grid positions.
    assert(chunkCoordForGrid({0,0}) == ChunkCoord{0,0});
    assert(chunkCoordForGrid({31,31}) == ChunkCoord{0,0});
    assert(chunkCoordForGrid({32,32}) == ChunkCoord{1,1});
    assert(chunkCoordForGrid({-1,-1}) == ChunkCoord{-1,-1});
    assert(chunkCoordForGrid({-32,-32}) == ChunkCoord{-1,-1});
    assert(chunkCoordForGrid({-33,-33}) == ChunkCoord{-2,-2});
    assert(chunkLocalGrid({-1,-1}).x == 31);
    assert(chunkLocalGrid({-1,-1}).y == 31);
    assert(chunkLocalGrid({32,33}).x == 0);
    assert(chunkLocalGrid({32,33}).y == 1);

    const WorldSeed worldSeed = 0x123456789abcdef0ULL;
    const WorldGenerationVersion generationVersion = CurrentWorldGenerationVersion;
    const std::vector<ChunkCoord> coords = {
        {0,0},{1,0},{-1,0},{17,22},{-17,22},{17,-22},{-17,-22},{1024,-2048}
    };

    std::map<std::pair<int,int>,UntouchedChunkBaseline> forward;
    for(const ChunkCoord coord:coords){
        forward[{coord.x,coord.y}] = deriveUntouchedChunkBaseline(worldSeed,coord,generationVersion);
    }

    // Requesting the same chunks in a different exploration order must produce
    // byte-for-byte-equivalent logical baseline seeds.
    std::vector<ChunkCoord> reverseOrder = coords;
    std::reverse(reverseOrder.begin(),reverseOrder.end());
    for(const ChunkCoord coord:reverseOrder){
        const UntouchedChunkBaseline regenerated =
            deriveUntouchedChunkBaseline(worldSeed,coord,generationVersion);
        assert(regenerated == forward.at({coord.x,coord.y}));
    }

    // The generation version is part of identity; PopulationSeed is not.
    const ChunkCoord probe{17,-22};
    const auto v1 = deriveUntouchedChunkBaseline(worldSeed,probe,1);
    const auto v2 = deriveUntouchedChunkBaseline(worldSeed,probe,2);
    assert(v1 != v2);

    const WorldGenesisIdentity peopleA = makeWorldGenesisIdentity(worldSeed,111,1);
    const WorldGenesisIdentity peopleB = makeWorldGenesisIdentity(worldSeed,222,1);
    assert(peopleA.populationSeed != peopleB.populationSeed);
    assert(deriveUntouchedChunkBaseline(peopleA,probe) == deriveUntouchedChunkBaseline(peopleB,probe));

    const auto otherWorld = deriveUntouchedChunkBaseline(worldSeed+1,probe,1);
    assert(otherWorld != v1);

    // Core NEW GAME now has a genuinely separate initial-population stream.
    Simulation first(worldSeed,111,1);
    Simulation second(worldSeed,222,1);
    Simulation replay(worldSeed,111,1);
    first.setupNewGame();
    second.setupNewGame();
    replay.setupNewGame();

    assert(first.world().seed == second.world().seed);
    assert(first.world().populationSeed != second.world().populationSeed);
    assert(first.world().generationVersion == second.world().generationVersion);
    assert(first.world().untouchedChunkBaseline(probe) == second.world().untouchedChunkBaseline(probe));
    assert(founderSignature(first) != founderSignature(second));
    assert(founderSignature(first) == founderSignature(replay));

    // Founder creation must not advance the world RNG. Same WorldSeed therefore
    // starts the world stream identically even with different people.
    const std::uint64_t firstWorldRandom = first.world().rng();
    const std::uint64_t secondWorldRandom = second.world().rng();
    assert(firstWorldRandom == secondWorldRandom);

    return 0;
}
''', encoding='utf-8')

# CMake registration => 46 Core tests.
p = Path('Source/LifeLensCore/CMakeLists.txt')
t = p.read_text(encoding='utf-8')
anchor = 'lifelens_add_test(test_primitive_latrine_progression)\n'
t = replace_once(t, anchor, anchor + '\nlifelens_add_test(test_world_genesis_wg1)\n', 'CMake WG-1 registration')
p.write_text(t, encoding='utf-8')

# Structural validator; this intentionally rejects std::hash in the stable seed contract.
Path('Tools/validate_world_genesis_wg1.py').write_text(r'''#!/usr/bin/env python3
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
assert 'std::hash' not in header, 'stable chunk identity must not depend on implementation-defined std::hash'
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
''', encoding='utf-8')

print('World Genesis WG-1 patch applied')
