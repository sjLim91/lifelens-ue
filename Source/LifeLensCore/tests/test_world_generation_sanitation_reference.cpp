#include "lifelens/CivilizationDecision.h"
#include "lifelens/Simulation.h"
#include "lifelens/WorldGenesis.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

using namespace lifelens;

namespace {

void assertLocalToStartRegion(GridPos pos, GridPos startCenter, ChunkCoord startChunk)
{
    const int dx = std::abs(pos.x - startCenter.x);
    const int dy = std::abs(pos.y - startCenter.y);
    const int chebyshevDistance = std::max(dx, dy);
    assert(chebyshevDistance >= 5);
    assert(chebyshevDistance <= 7);
    assert(chunkCoordForGrid(pos) == startChunk);
}

} // namespace

int main()
{
    // Find a deterministic production world whose selected start chunk is not
    // the legacy absolute origin. The regression only matters when World
    // Genesis places the settlement elsewhere.
    std::uint64_t chosenSeed = 0;
    ChunkCoord startChunk{};
    GridPos startCenter{};

    for (std::uint64_t seed = 1; seed <= 128; ++seed)
    {
        Simulation candidate(seed);
        candidate.setupNewGame();
        const ChunkCoord chunk = candidate.world().initialStartRegion().region.coord;
        if (chunk.x != 0 || chunk.y != 0)
        {
            chosenSeed = seed;
            startChunk = chunk;
            startCenter = candidate.world().initialStartRegionCenterGrid();
            break;
        }
    }

    assert(chosenSeed != 0);

    Simulation simulation(chosenSeed);
    simulation.setupNewGame();
    assert(simulation.world().primitiveSanitationSites.empty());

    for (const Character& resident : simulation.world().characters)
    {
        GridPos runtime{};
        assert(simulation.runtimePosition(resident.id, runtime));

        GridPos recommended{};
        assert(simulation.recommendedOutdoorReliefPosition(resident.id, recommended));

        SanitationUseTarget target;
        assert(simulation.sanitationUseTarget(resident.id, target));
        assert(target.kind == SanitationUseTargetKind::EmergencyOutdoor);
        assert(target.siteId == 0);
        assert(target.pos.x == recommended.x);
        assert(target.pos.y == recommended.y);

        // A start-center-relative 5-7 cell target stays inside the selected
        // 32x32 start chunk instead of sending a founder toward grid (0,0).
        assertLocalToStartRegion(recommended, startCenter, startChunk);

        const int legacyOriginDistance = std::max(
            std::abs(recommended.x), std::abs(recommended.y));
        const int startDistanceFromOrigin = std::max(
            std::abs(startCenter.x), std::abs(startCenter.y));
        if (startDistanceFromOrigin > 16)
        {
            assert(legacyOriginDistance > 7);
        }
    }

    // The same reference must survive progression. Once a founder learns the
    // designated sanitation technique, autonomous Craft selection/execution
    // must establish the site around the generated settlement, not around the
    // pre-WG absolute origin.
    Character& builder = simulation.world().characters.front();
    builder.civilization.knowledge.learn(
        TechniqueId::DesignatedSanitationArea,
        KnowledgeLevel::Reproducible,
        0.90);

    const CivilizationUtilityDecision craft =
        bestCraftDecision(simulation.world(), builder);
    assert(craft.intent == CivilizationIntent::Craft);
    assert(craft.technique == TechniqueId::DesignatedSanitationArea);

    const CivilizationExecutionResult established =
        executeCivilizationDecision(simulation.world(), builder, craft);
    assert(established.executed);
    assert(established.success);
    assert(established.sanitationSiteId != 0);
    assert(simulation.world().primitiveSanitationSites.size() == 1);
    assertLocalToStartRegion(established.sanitationSitePos, startCenter, startChunk);

    const PrimitiveSanitationSite* site = findPrimitiveSanitationSite(
        simulation.world().primitiveSanitationSites,
        established.sanitationSiteId);
    assert(site != nullptr);
    assert(site->pos.x == established.sanitationSitePos.x);
    assert(site->pos.y == established.sanitationSitePos.y);

    SanitationUseTarget designated;
    assert(simulation.sanitationUseTarget(builder.id, designated));
    assert(designated.kind == SanitationUseTargetKind::DesignatedArea);
    assert(designated.siteId == established.sanitationSiteId);
    assert(designated.pos.x == established.sanitationSitePos.x);
    assert(designated.pos.y == established.sanitationSitePos.y);

    std::cout << "world-generation sanitation reference test passed\n";
    return 0;
}
