#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "Civilization.h"
#include "CivilizationSpatial.h"
#include "ResourceExploration.h"
#include "SettlementDemand.h"
#include "World.h"

namespace lifelens {

// C6-A turns resource scarcity into an explicit Core-authored movement pressure.
// It does not relocate a resident or create a settlement by itself. C6-B owns
// the actual settlement split. This read model only answers: "is continuing to
// serve this lived area becoming expensive enough that migration is plausible?"
inline constexpr int MigrationLocalResourceRadiusChunks = 3;
inline constexpr int MigrationComfortTravelRadiusChunks = 2;
inline constexpr int MigrationHighTravelRadiusChunks = 10;
inline constexpr double MigrationCandidatePressureThreshold = 0.58;
inline constexpr double MigrationLongRangeExploreThreshold = 0.48;

struct MigrationPressureObservation {
    CharacterId residentId = 0;
    bool candidate = false;
    MaterialKind bottleneckMaterial = MaterialKind::Unknown;

    double resourceScarcity01 = 0.0;
    double travelBurden01 = 0.0;
    double populationPressure01 = 0.0;
    double settlementAttachment01 = 0.0;
    double explorationDisposition01 = 0.0;
    double pressure01 = 0.0;

    int localResourceUnits = 0;
    int localReserveUnits = 0;
    int nearestKnownResourceDistanceGrid = -1;

    bool hasFrontierTarget = false;
    ChunkCoord frontierChunk{};
    GridPos frontierTarget{};
    int frontierDistanceChunks = 0;
};

inline double clampMigration01(double value)
{
    return std::max(0.0, std::min(1.0, value));
}

inline int migrationComfortResourceUnits(MaterialKind material)
{
    switch(material){
        case MaterialKind::Water: return 16;
        case MaterialKind::PlantFood: return 16;
        case MaterialKind::Wood: return 12;
        case MaterialKind::Stone: return 10;
        case MaterialKind::Fiber: return 10;
        case MaterialKind::Clay: return 8;
        default: return 0;
    }
}

inline int migrationLocalResourceUnits(
    const World& world,
    MaterialKind material,
    GridPos anchor)
{
    const int radius =
        WorldChunkSpanGridCells * MigrationLocalResourceRadiusChunks;
    int total = 0;
    for(const ResourceNode& node : world.resourceNodes){
        if(node.id == 0 || node.material != material || node.quantity <= 0){
            continue;
        }
        GridPos access = node.pos;
        resolveCivilizationResourceAccessGridPosition(world, node.id, access);
        const int distance = std::max(
            std::abs(access.x - anchor.x),
            std::abs(access.y - anchor.y));
        if(distance <= radius) total += node.quantity;
    }
    return total;
}

inline int migrationLocalReserveUnits(
    const World& world,
    const Character& resident,
    MaterialKind material,
    GridPos anchor)
{
    int total = material == MaterialKind::Water
        ? portableWaterCount(resident.civilization.inventory)
        : resident.civilization.inventory.count(
            ItemKind::RawMaterial, material);

    for(const StorageSite& storage : world.storageSites){
        if(storage.id == 0
           || manhattan(storage.pos, anchor) > SettlementServiceRadiusGrid){
            continue;
        }
        total += material == MaterialKind::Water
            ? portableWaterCount(storage.inventory)
            : storage.inventory.count(ItemKind::RawMaterial, material);
    }
    return total;
}

inline int migrationNearestKnownResourceDistanceGrid(
    const World& world,
    MaterialKind material,
    GridPos anchor)
{
    int best = std::numeric_limits<int>::max();
    for(const ResourceNode& node : world.resourceNodes){
        if(node.id == 0 || node.material != material || node.quantity <= 0){
            continue;
        }
        GridPos access = node.pos;
        if(!resolveCivilizationResourceAccessGridPosition(
                world, node.id, access)){
            continue;
        }
        best = std::min(
            best,
            std::max(
                std::abs(access.x - anchor.x),
                std::abs(access.y - anchor.y)));
    }
    return best == std::numeric_limits<int>::max() ? -1 : best;
}

inline double migrationMaterialNeed01(
    const Character& resident,
    MaterialKind material)
{
    switch(material){
        case MaterialKind::Water:
            return clampMigration01(std::max(
                resident.needs.thirst,
                resident.needs.hygiene * 0.35));
        case MaterialKind::PlantFood:
            return clampMigration01(resident.needs.hunger);
        case MaterialKind::Wood:
            return 0.34;
        case MaterialKind::Stone:
            return 0.28;
        case MaterialKind::Fiber:
            return 0.24;
        case MaterialKind::Clay:
            return 0.20;
        default:
            return 0.0;
    }
}

inline double migrationSettlementAttachment01(
    const World& world,
    GridPos anchor)
{
    int weighted = 0;
    for(const ConstructedFacility& facility : world.facilities){
        if(!facilityOperationalAndActive(facility)
           || manhattan(facility.pos, anchor) > SettlementServiceRadiusGrid){
            continue;
        }
        // Storage/shelter/work infrastructure represents sunk local effort.
        weighted += facility.kind == FacilityKind::Shelter ? 2 : 1;
    }
    for(const StorageSite& storage : world.storageSites){
        if(storage.id != 0
           && manhattan(storage.pos, anchor) <= SettlementServiceRadiusGrid){
            ++weighted;
        }
    }
    return clampMigration01(static_cast<double>(weighted) / 8.0);
}

inline double migrationPopulationPressure01(
    const World& world,
    GridPos anchor,
    const SettlementPopulation* population)
{
    if(population == nullptr) return 0.0;
    int localResidents = 0;
    for(const auto& entry : *population){
        if(manhattan(entry.second, anchor) > SettlementServiceRadiusGrid){
            continue;
        }
        const auto character = std::find_if(
            world.characters.begin(),
            world.characters.end(),
            [&](const Character& value){
                return value.id == entry.first && value.alive;
            });
        if(character != world.characters.end()) ++localResidents;
    }

    // Four founders fit comfortably. Pressure begins only as the lived cluster
    // grows beyond that baseline, avoiding a fake "overcrowded at NEW GAME"
    // migration trigger.
    return clampMigration01(
        static_cast<double>(std::max(0, localResidents - 4)) / 8.0);
}

inline MigrationPressureObservation observeMigrationPressure(
    const World& world,
    const Character& resident,
    GridPos authoritativePosition,
    const SettlementPopulation* population = nullptr)
{
    MigrationPressureObservation result;
    result.residentId = resident.id;
    if(resident.id == 0 || !resident.alive) return result;

    const std::array<MaterialKind, 6> foundationalMaterials{{
        MaterialKind::Water,
        MaterialKind::PlantFood,
        MaterialKind::Wood,
        MaterialKind::Stone,
        MaterialKind::Fiber,
        MaterialKind::Clay
    }};

    double strongestMaterialPressure = 0.0;
    for(const MaterialKind material : foundationalMaterials){
        const int comfort = migrationComfortResourceUnits(material);
        if(comfort <= 0) continue;

        const int localResources =
            migrationLocalResourceUnits(world, material, authoritativePosition);
        const int localReserve =
            migrationLocalReserveUnits(
                world, resident, material, authoritativePosition);
        const int usableLocal = localResources + localReserve;

        const double scarcity = 1.0 - clampMigration01(
            static_cast<double>(usableLocal)
            / static_cast<double>(comfort));

        const int nearestDistance =
            migrationNearestKnownResourceDistanceGrid(
                world, material, authoritativePosition);
        const double nearGrid =
            WorldChunkSpanGridCells * MigrationComfortTravelRadiusChunks;
        const double farGrid =
            WorldChunkSpanGridCells * MigrationHighTravelRadiusChunks;
        const double travel = nearestDistance < 0
            ? (scarcity >= 0.55 ? 0.70 : 0.0)
            : clampMigration01(
                (static_cast<double>(nearestDistance) - nearGrid)
                / std::max(1.0, farGrid - nearGrid));

        const double need = migrationMaterialNeed01(resident, material);
        const double materialPressure = clampMigration01(
            0.62 * scarcity
            + 0.26 * travel
            + 0.12 * need);

        if(materialPressure > strongestMaterialPressure + 1e-12){
            strongestMaterialPressure = materialPressure;
            result.bottleneckMaterial = material;
            result.resourceScarcity01 = scarcity;
            result.travelBurden01 = travel;
            result.localResourceUnits = localResources;
            result.localReserveUnits = localReserve;
            result.nearestKnownResourceDistanceGrid = nearestDistance;
        }
    }

    result.populationPressure01 =
        migrationPopulationPressure01(
            world, authoritativePosition, population);
    result.settlementAttachment01 =
        migrationSettlementAttachment01(world, authoritativePosition);
    result.explorationDisposition01 = clampMigration01(
        0.55 * resident.personality.curiosity
        + 0.45 * resident.personality.adaptability);

    // Attachment deliberately resists migration. Scarcity + travel can still
    // overpower it, but an established settlement is harder to abandon than a
    // temporary camp with the same resource state.
    result.pressure01 = clampMigration01(
        0.56 * strongestMaterialPressure
        + 0.18 * result.travelBurden01
        + 0.12 * result.populationPressure01
        + 0.14 * result.explorationDisposition01
        - 0.16 * result.settlementAttachment01);

    result.candidate =
        result.bottleneckMaterial != MaterialKind::Unknown
        && result.resourceScarcity01 >= 0.58
        && result.travelBurden01 >= 0.28
        && result.pressure01 >= MigrationCandidatePressureThreshold;

    if(result.bottleneckMaterial != MaterialKind::Unknown
       && result.pressure01 >= MigrationLongRangeExploreThreshold){
        const ResourceExplorationOpportunity frontier =
            chooseCriticalResourceExplorationOpportunity(
                world,
                resident.id,
                result.bottleneckMaterial,
                authoritativePosition);
        if(frontier.available){
            result.hasFrontierTarget = true;
            result.frontierChunk = frontier.chunk;
            result.frontierTarget = frontier.target;
            const ChunkCoord center =
                chunkCoordForGrid(authoritativePosition);
            result.frontierDistanceChunks = std::max(
                std::abs(frontier.chunk.x - center.x),
                std::abs(frontier.chunk.y - center.y));
        }
    }

    return result;
}

inline bool migrationPressureWarrantsLongRangeExploration(
    const MigrationPressureObservation& observation,
    MaterialKind material)
{
    return observation.bottleneckMaterial == material
        && observation.pressure01 >= MigrationLongRangeExploreThreshold;
}

} // namespace lifelens
