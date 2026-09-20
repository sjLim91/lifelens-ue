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

    // Unknown ids must never fabricate a spatial target.
    GridPos missing{};
    CHECK(!resolveCivilizationResourceGridPosition(first.world(), 0, missing));
    CHECK(!resolveCivilizationResourceGridPosition(first.world(), 999999999ULL, missing));
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
