#include "lifelens/Simulation.h"
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
    assert((chunkCoordForGrid({0,0}) == ChunkCoord{0,0}));
    assert((chunkCoordForGrid({31,31}) == ChunkCoord{0,0}));
    assert((chunkCoordForGrid({32,32}) == ChunkCoord{1,1}));
    assert((chunkCoordForGrid({-1,-1}) == ChunkCoord{-1,-1}));
    assert((chunkCoordForGrid({-32,-32}) == ChunkCoord{-1,-1}));
    assert((chunkCoordForGrid({-33,-33}) == ChunkCoord{-2,-2}));
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
