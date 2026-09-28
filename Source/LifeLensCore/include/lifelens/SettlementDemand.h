#pragma once

#include <algorithm>
#include <unordered_map>
#include "World.h"

namespace lifelens {

// A transient view of Simulation::runtime_ for one decision/completion. It is
// never persisted and never advances or overrides authoritative positions.
using SettlementPopulation = std::unordered_map<CharacterId, GridPos>;

// Planning service budgets, not spawn quotas or simultaneous-use reservations.
// Bedding remains useful even under a roof; a shelter is not free bedding for
// every resident. Work surfaces can be shared by two working residents.
inline int settlementPlanningCapacity(FacilityKind kind)
{
    switch(kind){
        case FacilityKind::SleepingPlace: return 1;
        case FacilityKind::Shelter: return 4;
        case FacilityKind::WorkSurface: return 2;
        default: return 0;
    }
}

inline constexpr int SettlementServiceRadiusGrid = WorldChunkSpanGridCells * 2;

struct SettlementFacilityDemand {
    int residents = 0;
    int operationalCapacity = 0;
    int committedCapacity = 0;
    int pendingProjects = 0;
    double peakSleepNeed = 0.0;
    double peakExposureBurden = 0.0;

    bool unmet() const { return residents > operationalCapacity; }
    bool canPlan() const {
        return residents > committedCapacity && pendingProjects == 0;
    }
};

inline SettlementFacilityDemand observeSettlementFacilityDemand(
    const World& world, CharacterId planner, FacilityKind kind, GridPos anchor,
    const SettlementPopulation* population = nullptr)
{
    SettlementFacilityDemand result;
    const int capacity = settlementPlanningCapacity(kind);
    if(capacity == 0 || planner == 0) return result;

    for(const auto& resident : world.characters){
        if(!resident.alive) continue;
        // Callers without runtime access may reason about their known actor;
        // they must not assume that all other residents live at that position.
        if(population){
            const auto location = population->find(resident.id);
            if(location == population->end()
               || manhattan(location->second, anchor) > SettlementServiceRadiusGrid) continue;
        }else if(resident.id != planner){
            continue;
        }
        if(kind == FacilityKind::WorkSurface
           && !lifeStageProfile(resident.lifeStage).canWork) continue;
        ++result.residents;
        result.peakSleepNeed = std::max(result.peakSleepNeed, resident.needs.sleep);
        result.peakExposureBurden = std::max({result.peakExposureBurden,
            resident.needs.sleep, resident.needs.thirst, resident.needs.hygiene});
    }

    for(const auto& facility : world.facilities){
        if(facility.kind != kind || facility.state == FacilityState::Ruined
           || manhattan(facility.pos, anchor) > SettlementServiceRadiusGrid) continue;
        if(facilityOperationalAndActive(facility)){
            result.operationalCapacity += capacity;
            result.committedCapacity += capacity;
        }else if(facility.state == FacilityState::Planned
                 || facility.state == FacilityState::UnderConstruction){
            result.committedCapacity += capacity;
            ++result.pendingProjects;
        }
    }
    return result;
}

inline const ConstructedFacility* settlementConstructionProjectNear(
    const World& world, FacilityKind kind, GridPos anchor)
{
    const ConstructedFacility* best = nullptr;
    int bestDistance = SettlementServiceRadiusGrid + 1;
    for(const auto& facility : world.facilities){
        if(facility.kind != kind || (facility.state != FacilityState::Planned
           && facility.state != FacilityState::UnderConstruction)) continue;
        const int distance = manhattan(facility.pos, anchor);
        if(distance < bestDistance
           || (best && distance == bestDistance && facility.id < best->id)){
            best = &facility;
            bestDistance = distance;
        }
    }
    return best;
}

} // namespace lifelens
