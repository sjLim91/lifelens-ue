#include <iostream>
#include <string>
#include <vector>

#include "lifelens/CivilizationSpatial.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static const ResourceNode* findResourceById(const World& world, ResourceNodeId id)
{
    for(const auto& node : world.resourceNodes){
        if(node.id == id) return &node;
    }
    return nullptr;
}

static ResourceNode* findResourceById(World& world, ResourceNodeId id)
{
    for(auto& node : world.resourceNodes){
        if(node.id == id) return &node;
    }
    return nullptr;
}


static const StorageSite* findStorageById(const World& world, StorageId id)
{
    for(const auto& storage : world.storageSites){
        if(storage.id == id) return &storage;
    }
    return nullptr;
}

int main()
{
    constexpr WorldSeed Seed = 20260916;
    constexpr PopulationSeed Population = 424242;

    Simulation first(Seed, Population);
    Simulation second(Seed, Population);
    first.setupNewGame();
    second.setupNewGame();

    CHECK(first.world().generatedNaturalChunks.size() == 2);
    CHECK(second.world().generatedNaturalChunks.size() == 2);
    CHECK(!first.world().resourceNodes.empty());
    CHECK(first.world().resourceNodes.size() == second.world().resourceNodes.size());

    // The materialized ResourceNode must preserve the exact generated patch
    // position, and same seeds must generate the same authoritative targets.
    const GeneratedNaturalChunk& firstChunk = first.world().generatedNaturalChunks.front();
    CHECK(!firstChunk.resourcePatches.empty());
    // Current worlds should resolve the owning generated patch through the
    // ResourceNode position's chunk instead of scanning every materialized
    // chunk. A legacy/mismatched node position must still fall back to the
    // immutable generated patch identity.
    const NaturalResourcePatch& positionProbePatch=
        firstChunk.resourcePatches.front();
    ResourceNode* positionProbeNode=
        findResourceById(first.world(), positionProbePatch.nodeId);
    CHECK(positionProbeNode != nullptr);
    const NaturalResourcePatch* directPatch=
        findGeneratedNaturalResourcePatch(first.world(), *positionProbeNode);
    CHECK(directPatch != nullptr);
    CHECK(directPatch->nodeId == positionProbePatch.nodeId);
    CHECK(directPatch->pos.x == positionProbePatch.pos.x);
    CHECK(directPatch->pos.y == positionProbePatch.pos.y);

    const GridPos savedProbePosition=positionProbeNode->pos;
    positionProbeNode->pos={
        savedProbePosition.x + WorldChunkSpanGridCells * 32,
        savedProbePosition.y + WorldChunkSpanGridCells * 32
    };
    const NaturalResourcePatch* legacyFallbackPatch=
        findGeneratedNaturalResourcePatch(first.world(), *positionProbeNode);
    CHECK(legacyFallbackPatch != nullptr);
    CHECK(legacyFallbackPatch->nodeId == positionProbePatch.nodeId);
    CHECK(legacyFallbackPatch->pos.x == positionProbePatch.pos.x);
    CHECK(legacyFallbackPatch->pos.y == positionProbePatch.pos.y);

    GridPos fallbackResolved{};
    CHECK(resolveCivilizationResourceGridPosition(
        first.world(), positionProbePatch.nodeId, fallbackResolved));
    CHECK(fallbackResolved.x == positionProbePatch.pos.x);
    CHECK(fallbackResolved.y == positionProbePatch.pos.y);
    positionProbeNode->pos=savedProbePosition;

    for(const auto& patch : firstChunk.resourcePatches){
        const ResourceNode* firstNode = findResourceById(first.world(), patch.nodeId);
        const ResourceNode* secondNode = findResourceById(second.world(), patch.nodeId);
        CHECK(firstNode != nullptr);
        CHECK(secondNode != nullptr);
        CHECK(firstNode->pos.x == patch.pos.x);
        CHECK(firstNode->pos.y == patch.pos.y);
        CHECK(secondNode->pos.x == patch.pos.x);
        CHECK(secondNode->pos.y == patch.pos.y);

        GridPos resolved{};
        CHECK(resolveCivilizationResourceGridPosition(first.world(), patch.nodeId, resolved));
        CHECK(resolved.x == patch.pos.x);
        CHECK(resolved.y == patch.pos.y);
    }

    // Generated v2 Water stays at the hydrology center as resource authority,
    // while Gather resolves to a deterministic dry-bank access position.
    bool checkedAnyWaterAccess = false;
    bool checkedNonWaterAccess = false;
    ResourceNodeId checkedWaterNodeId = 0;
    GridPos firstWaterAccess{};
    ChunkCoord checkedWaterChunk{};
    for(const auto& chunk : first.world().generatedNaturalChunks){
        for(const auto& patch : chunk.resourcePatches){
            GridPos exact{};
            GridPos access{};
            CHECK(resolveCivilizationResourceGridPosition(
                first.world(), patch.nodeId, exact));
            CHECK(resolveCivilizationResourceAccessGridPosition(
                first.world(), patch.nodeId, access));

            if(patch.material == MaterialKind::Water){
                const HydrologyFacts facts =
                    deriveHydrologyFacts(first.world().genesisIdentity(), chunk.coord);
                CHECK(isFreshSurfaceWater(facts));
                const GridPos waterCenter = surfaceWaterCenterGrid(facts);
                CHECK(exact.x == waterCenter.x);
                CHECK(exact.y == waterCenter.y);
                CHECK(surfaceWaterGroundContainsGrid(facts, exact));
                CHECK(!surfaceWaterGroundContainsGrid(facts, access));
                CHECK(chunkCoordForGrid(access) == chunk.coord);

                const std::vector<NaturalPhysicalObstacle> obstacles =
                    deriveNaturalPhysicalObstacles(chunk);
                for(const NaturalPhysicalObstacle& obstacle : obstacles){
                    CHECK(obstacle.grid.x != access.x || obstacle.grid.y != access.y);
                }
                for(const ConstructedFacility& facility : first.world().facilities){
                    CHECK(facility.pos.x != access.x || facility.pos.y != access.y);
                }

                GridPos replayAccess{};
                CHECK(resolveCivilizationResourceAccessGridPosition(
                    second.world(), patch.nodeId, replayAccess));
                CHECK(replayAccess.x == access.x);
                CHECK(replayAccess.y == access.y);

                if(!checkedAnyWaterAccess){
                    checkedWaterNodeId = patch.nodeId;
                    firstWaterAccess = access;
                    checkedWaterChunk = chunk.coord;
                }
                checkedAnyWaterAccess = true;
            }else{
                const HydrologyFacts facts =
                    deriveHydrologyFacts(first.world().genesisIdentity(), chunk.coord);
                CHECK(!surfaceWaterGroundContainsGrid(facts, access));
                CHECK(chunkCoordForGrid(access) == chunk.coord);

                const std::vector<NaturalPhysicalObstacle> obstacles =
                    deriveNaturalPhysicalObstacles(chunk);
                for(const NaturalPhysicalObstacle& obstacle : obstacles){
                    CHECK(obstacle.grid.x != access.x || obstacle.grid.y != access.y);
                }
                for(const ConstructedFacility& facility : first.world().facilities){
                    CHECK(facility.pos.x != access.x || facility.pos.y != access.y);
                }

                GridPos replayAccess{};
                CHECK(resolveCivilizationResourceAccessGridPosition(
                    second.world(), patch.nodeId, replayAccess));
                CHECK(replayAccess.x == access.x);
                CHECK(replayAccess.y == access.y);
                checkedNonWaterAccess = true;
            }
        }
    }
    CHECK(checkedAnyWaterAccess);
    CHECK(checkedNonWaterAccess);

    // If the preferred dry bank later becomes occupied by a constructed
    // facility, the same Water ResourceNode must deterministically resolve to
    // another dry, materialized, obstacle-free bank cell instead of becoming
    // permanently unreachable.
    CHECK(checkedWaterNodeId != 0);
    ConstructedFacility waterBankBlocker;
    waterBankBlocker.id = 990001;
    waterBankBlocker.pos = firstWaterAccess;
    first.world().facilities.push_back(waterBankBlocker);

    GridPos reroutedWaterAccess{};
    CHECK(resolveCivilizationResourceAccessGridPosition(
        first.world(), checkedWaterNodeId, reroutedWaterAccess));
    CHECK(
        reroutedWaterAccess.x != firstWaterAccess.x
        || reroutedWaterAccess.y != firstWaterAccess.y);
    CHECK(chunkCoordForGrid(reroutedWaterAccess) == checkedWaterChunk);
    const HydrologyFacts reroutedWaterFacts =
        deriveHydrologyFacts(first.world().genesisIdentity(), checkedWaterChunk);
    CHECK(!surfaceWaterGroundContainsGrid(
        reroutedWaterFacts, reroutedWaterAccess));
    for(const ConstructedFacility& facility : first.world().facilities){
        CHECK(
            facility.pos.x != reroutedWaterAccess.x
            || facility.pos.y != reroutedWaterAccess.y);
    }
    if(const GeneratedNaturalChunk* waterChunk =
            first.world().findGeneratedNaturalChunk(checkedWaterChunk)){
        for(const NaturalPhysicalObstacle& obstacle :
                deriveNaturalPhysicalObstacles(*waterChunk)){
            CHECK(
                obstacle.grid.x != reroutedWaterAccess.x
                || obstacle.grid.y != reroutedWaterAccess.y);
        }
    }

    // The temporary blocker exists only to exercise bank re-resolution.
    // Do not leak an intentionally incomplete fixture into the unrelated
    // snapshot-persistence assertions below.
    CHECK(!first.world().facilities.empty());
    // A non-Water resource that lands inside the water footprint must also
    // resolve to a deterministic dry interaction point instead of becoming an
    // unreachable/invisible gathering target.
    ResourceNode blockedStone;
    blockedStone.id = 990002;
    blockedStone.material = MaterialKind::Stone;
    blockedStone.quantity = 10;
    blockedStone.maxQuantity = 10;
    blockedStone.pos = surfaceWaterCenterGrid(reroutedWaterFacts);
    first.world().resourceNodes.push_back(blockedStone);

    GridPos blockedStoneAccess{};
    CHECK(resolveCivilizationResourceAccessGridPosition(
        first.world(), blockedStone.id, blockedStoneAccess));
    CHECK(
        blockedStoneAccess.x != blockedStone.pos.x
        || blockedStoneAccess.y != blockedStone.pos.y);
    CHECK(chunkCoordForGrid(blockedStoneAccess) == checkedWaterChunk);
    CHECK(!surfaceWaterGroundContainsGrid(
        reroutedWaterFacts, blockedStoneAccess));
    for(const ConstructedFacility& facility : first.world().facilities){
        CHECK(
            facility.pos.x != blockedStoneAccess.x
            || facility.pos.y != blockedStoneAccess.y);
    }
    first.world().resourceNodes.pop_back();

    CHECK(first.world().facilities.back().id == waterBankBlocker.id);
    first.world().facilities.pop_back();

    // Storage is also a Core-owned spatial entity. Presentation should only
    // consume this resolved GridPos, never guess a nearby scenery object.
    const GridPos center = first.world().initialStartRegionCenterGrid();
    StorageSite storage;
    storage.id = 9001;
    storage.pos = {center.x + 5, center.y - 3};
    storage.inventory.add({ItemKind::RawMaterial, MaterialKind::Wood, 4, 0.5, 1.0});
    first.world().storageSites.push_back(storage);

    GridPos storageResolved{};
    CHECK(resolveCivilizationStorageGridPosition(first.world(), storage.id, storageResolved));
    CHECK(storageResolved.x == storage.pos.x);
    CHECK(storageResolved.y == storage.pos.y);

    // P0 living-causality guard: resource and stock changes are only legal at
    // the authoritative physical target. A caller cannot mutate the world from
    // an unrelated position even when it knows the correct entity id.
    Character& actor = first.world().characters.front();
    actor.civilization.character = actor.id;

    GridPos gatherTarget{};
    CHECK(resolveCivilizationResourceAccessGridPosition(
        first.world(), checkedWaterNodeId, gatherTarget));
    ResourceNode* gatherNode = nullptr;
    for(auto& node : first.world().resourceNodes){
        if(node.id == checkedWaterNodeId){
            gatherNode = &node;
            break;
        }
    }
    CHECK(gatherNode != nullptr);
    CHECK(gatherNode->quantity > 0);

    CivilizationUtilityDecision gatherDecision;
    gatherDecision.intent = CivilizationIntent::Gather;
    gatherDecision.resourceNode = checkedWaterNodeId;
    gatherDecision.item = ItemKind::RawMaterial;
    gatherDecision.material = MaterialKind::Water;
    gatherDecision.quantity = 1;

    actor.civilization.inventory.add({
        ItemKind::SimpleContainer, MaterialKind::Clay, 1, 0.5, 1.0});
    CHECK(emptySimpleContainerCount(actor.civilization.inventory) >= 1);

    const int waterBeforeGather = actor.civilization.inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water);
    const int nodeBeforeGather = gatherNode->quantity;
    const GridPos remoteGatherPos{gatherTarget.x + 20, gatherTarget.y + 20};
    const CivilizationExecutionResult remoteGather =
        executeCivilizationDecisionAtPosition(
            first.world(), actor, gatherDecision, remoteGatherPos);
    CHECK(!remoteGather.executed);
    CHECK(gatherNode->quantity == nodeBeforeGather);
    CHECK(actor.civilization.inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water) == waterBeforeGather);

    const CivilizationExecutionResult localGather =
        executeCivilizationDecisionAtPosition(
            first.world(), actor, gatherDecision, gatherTarget);
    CHECK(localGather.executed && localGather.success);
    CHECK(gatherNode->quantity < nodeBeforeGather);
    CHECK(actor.civilization.inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water) > waterBeforeGather);

    CivilizationUtilityDecision storeDecision;
    storeDecision.intent = CivilizationIntent::Store;
    storeDecision.storage = storage.id;
    storeDecision.item = ItemKind::RawMaterial;
    storeDecision.material = MaterialKind::Water;
    storeDecision.quantity = 1;

    const int carriedBeforeStore = actor.civilization.inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water);
    const int storedBeforeStore = first.world().storageSites.back().inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water);
    const GridPos remoteStoragePos{storageResolved.x + 20, storageResolved.y + 20};
    const CivilizationExecutionResult remoteStore =
        executeCivilizationDecisionAtPosition(
            first.world(), actor, storeDecision, remoteStoragePos);
    CHECK(!remoteStore.executed);
    CHECK(actor.civilization.inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water) == carriedBeforeStore);
    CHECK(first.world().storageSites.back().inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water) == storedBeforeStore);

    const CivilizationExecutionResult localStore =
        executeCivilizationDecisionAtPosition(
            first.world(), actor, storeDecision, storageResolved);
    CHECK(localStore.executed && localStore.success);
    CHECK(actor.civilization.inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water) == carriedBeforeStore - 1);
    CHECK(first.world().storageSites.back().inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water) == storedBeforeStore + 1);

    CivilizationUtilityDecision retrieveDecision;
    retrieveDecision.intent = CivilizationIntent::Retrieve;
    retrieveDecision.storage = storage.id;
    retrieveDecision.item = ItemKind::RawMaterial;
    retrieveDecision.material = MaterialKind::Water;
    retrieveDecision.quantity = 1;

    const int carriedBeforeRetrieve = actor.civilization.inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water);
    const int storedBeforeRetrieve = first.world().storageSites.back().inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water);
    const CivilizationExecutionResult remoteRetrieve =
        executeCivilizationDecisionAtPosition(
            first.world(), actor, retrieveDecision, remoteStoragePos);
    CHECK(!remoteRetrieve.executed);
    CHECK(actor.civilization.inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water) == carriedBeforeRetrieve);
    CHECK(first.world().storageSites.back().inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water) == storedBeforeRetrieve);

    const CivilizationExecutionResult localRetrieve =
        executeCivilizationDecisionAtPosition(
            first.world(), actor, retrieveDecision, storageResolved);
    CHECK(localRetrieve.executed && localRetrieve.success);
    CHECK(actor.civilization.inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water) == carriedBeforeRetrieve + 1);
    CHECK(first.world().storageSites.back().inventory.count(
        ItemKind::RawMaterial, MaterialKind::Water) == storedBeforeRetrieve - 1);

    GridPos actorPosition{};
    CHECK(first.runtimePosition(actor.id, actorPosition));
    const SettlementFacilitySiteOpportunity sleepSite =
        chooseSettlementFacilitySite(
            first.world(), actor.id, FacilityKind::SleepingPlace, actorPosition);
    CHECK(sleepSite.available);

    CivilizationUtilityDecision planSleep;
    planSleep.intent = CivilizationIntent::Craft;
    planSleep.facilityKind = FacilityKind::SleepingPlace;
    planSleep.facilityAction = FacilityBuildAction::Plan;
    planSleep.hasFacilityTarget = true;
    planSleep.facilityTargetPos = sleepSite.pos;

    const std::size_t facilitiesBeforeRemotePlan = first.world().facilities.size();
    const GridPos remoteFacilityPos{
        sleepSite.pos.x + 20,
        sleepSite.pos.y + 20
    };
    const CivilizationExecutionResult remotePlan =
        executeCivilizationDecisionAtPosition(
            first.world(), actor, planSleep, remoteFacilityPos);
    CHECK(!remotePlan.executed);
    CHECK(first.world().facilities.size() == facilitiesBeforeRemotePlan);

    const CivilizationExecutionResult localPlan =
        executeCivilizationDecisionAtPosition(
            first.world(), actor, planSleep, sleepSite.pos);
    CHECK(localPlan.executed && localPlan.success);
    CHECK(first.world().facilities.size() == facilitiesBeforeRemotePlan + 1);

    // Unknown ids must never fabricate a spatial target.
    GridPos missing{};
    CHECK(!resolveCivilizationResourceGridPosition(first.world(), 0, missing));
    CHECK(!resolveCivilizationResourceGridPosition(first.world(), 999999999ULL, missing));
    CHECK(!resolveCivilizationResourceAccessGridPosition(first.world(), 0, missing));
    CHECK(!resolveCivilizationResourceAccessGridPosition(first.world(), 999999999ULL, missing));
    CHECK(!resolveCivilizationStorageGridPosition(first.world(), 0, missing));
    CHECK(!resolveCivilizationStorageGridPosition(first.world(), 999999999ULL, missing));

    // Snapshot v2 persists both entity positions, while generated resource
    // resolution still agrees with the immutable natural-patch position.
    std::vector<std::uint8_t> bytes;
    std::string error;
    CHECK(encodeSimulationSnapshot(first.captureSnapshot(), bytes, &error));
    CHECK(error.empty());

    SimulationStateSnapshot decoded;
    CHECK(decodeSimulationSnapshot(bytes, decoded, &error));
    CHECK(error.empty());

    const StorageSite* decodedStorage = findStorageById(decoded.world, storage.id);
    CHECK(decodedStorage != nullptr);
    CHECK(decodedStorage->pos.x == storage.pos.x);
    CHECK(decodedStorage->pos.y == storage.pos.y);

    for(const auto& patch : decoded.world.generatedNaturalChunks.front().resourcePatches){
        const ResourceNode* node = findResourceById(decoded.world, patch.nodeId);
        CHECK(node != nullptr);
        CHECK(node->pos.x == patch.pos.x);
        CHECK(node->pos.y == patch.pos.y);

        GridPos resolved{};
        CHECK(resolveCivilizationResourceGridPosition(decoded.world, patch.nodeId, resolved));
        CHECK(resolved.x == patch.pos.x);
        CHECK(resolved.y == patch.pos.y);
    }

    Simulation restored(1);
    CHECK(restored.restoreSnapshot(decoded, &error));
    CHECK(error.empty());

    const StorageSite* restoredStorage = findStorageById(restored.world(), storage.id);
    CHECK(restoredStorage != nullptr);
    CHECK(restoredStorage->pos.x == storage.pos.x);
    CHECK(restoredStorage->pos.y == storage.pos.y);

    std::cout << "civilization spatial targets passed\n";
    return 0;
}
