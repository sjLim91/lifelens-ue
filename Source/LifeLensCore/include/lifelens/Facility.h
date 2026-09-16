#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "Civilization.h"

namespace lifelens {

using FacilityId = std::uint64_t;

enum class FacilityKind : std::uint8_t {
    PrimitiveStorage = 0,
    FirePit,
    WorkSurface,
    SleepingPlace,
    Shelter,
    Furnace
};

enum class FacilityState : std::uint8_t {
    Planned = 0,
    UnderConstruction,
    Operational,
    Ruined
};

struct FacilityMaterialRequirement {
    MaterialKind material = MaterialKind::Unknown;
    int required = 0;
    int delivered = 0;
};

struct FacilityConstructionSpec {
    FacilityKind kind = FacilityKind::PrimitiveStorage;
    double requiredWork = 0.0;
    std::vector<FacilityMaterialRequirement> requirements;
};

struct ConstructedFacility {
    FacilityId id = 0;
    FacilityKind kind = FacilityKind::PrimitiveStorage;
    FacilityState state = FacilityState::Planned;
    GridPos pos{};
    CharacterId initiatedBy = 0;
    CharacterId lastWorkedBy = 0;
    int startedMinute = -1;
    int completedMinute = -1;
    double constructionWork = 0.0;
    double requiredWork = 0.0;
    double durability = 1.0;
    bool active = false;
    StorageId linkedStorage = 0;
    std::vector<FacilityMaterialRequirement> requirements;
};

inline bool validFacilityKind(FacilityKind kind)
{
    return static_cast<int>(kind) >= static_cast<int>(FacilityKind::PrimitiveStorage)
        && static_cast<int>(kind) <= static_cast<int>(FacilityKind::Furnace);
}

inline bool validFacilityState(FacilityState state)
{
    return static_cast<int>(state) >= static_cast<int>(FacilityState::Planned)
        && static_cast<int>(state) <= static_cast<int>(FacilityState::Ruined);
}

inline bool facilityProvidesStorage(FacilityKind kind)
{
    return kind == FacilityKind::PrimitiveStorage;
}

inline FacilityConstructionSpec facilityConstructionSpec(FacilityKind kind)
{
    switch(kind){
        case FacilityKind::PrimitiveStorage:
            // A low-tech raised cache / covered stockpile. The material budget is
            // deliberately reachable with the existing early-game carrying model.
            return {kind,8.0,{
                {MaterialKind::Wood,4,0},
                {MaterialKind::Fiber,2,0}
            }};
        case FacilityKind::FirePit:
        case FacilityKind::WorkSurface:
        case FacilityKind::SleepingPlace:
        case FacilityKind::Shelter:
        case FacilityKind::Furnace:
        default:
            // Reserved kinds are intentionally non-constructible until their own
            // progression slice defines an authoritative recipe and work budget.
            return {kind,0.0,{}};
    }
}

inline bool facilityKindConstructible(FacilityKind kind)
{
    const FacilityConstructionSpec spec = facilityConstructionSpec(kind);
    return spec.requiredWork > 0.0 && !spec.requirements.empty();
}

inline FacilityId nextFacilityId(const std::vector<ConstructedFacility>& facilities)
{
    FacilityId next = 1;
    for(const auto& facility : facilities){
        if(facility.id >= next) next = facility.id + 1;
    }
    return next == 0 ? 1 : next;
}

inline ConstructedFacility makeFacilityConstructionSite(
    FacilityId id,
    FacilityKind kind,
    GridPos pos,
    CharacterId initiatedBy,
    int minute)
{
    ConstructedFacility facility;
    const FacilityConstructionSpec spec = facilityConstructionSpec(kind);
    if(id == 0 || initiatedBy == 0 || spec.requiredWork <= 0.0 || spec.requirements.empty())
        return facility;

    facility.id = id;
    facility.kind = kind;
    facility.state = FacilityState::Planned;
    facility.pos = pos;
    facility.initiatedBy = initiatedBy;
    facility.lastWorkedBy = initiatedBy;
    facility.startedMinute = std::max(0,minute);
    facility.requiredWork = spec.requiredWork;
    facility.requirements = spec.requirements;
    return facility;
}

inline const FacilityMaterialRequirement* findFacilityRequirement(
    const ConstructedFacility& facility,
    MaterialKind material)
{
    for(const auto& requirement : facility.requirements)
        if(requirement.material == material) return &requirement;
    return nullptr;
}

inline FacilityMaterialRequirement* findFacilityRequirement(
    ConstructedFacility& facility,
    MaterialKind material)
{
    for(auto& requirement : facility.requirements)
        if(requirement.material == material) return &requirement;
    return nullptr;
}

inline int facilityMissingMaterial(const ConstructedFacility& facility,MaterialKind material)
{
    const FacilityMaterialRequirement* requirement = findFacilityRequirement(facility,material);
    return requirement ? std::max(0,requirement->required - requirement->delivered) : 0;
}

inline bool facilityMaterialsComplete(const ConstructedFacility& facility)
{
    if(facility.requirements.empty()) return false;
    for(const auto& requirement : facility.requirements){
        if(requirement.required <= 0 || requirement.delivered < requirement.required) return false;
    }
    return true;
}

inline bool facilityWorkComplete(const ConstructedFacility& facility)
{
    return facility.requiredWork > 0.0
        && facility.constructionWork + 1e-9 >= facility.requiredWork;
}

inline int deliverFacilityMaterial(
    ConstructedFacility& facility,
    Inventory& source,
    MaterialKind material,
    int requested)
{
    if(facility.id == 0 || facility.state == FacilityState::Operational
       || facility.state == FacilityState::Ruined || requested <= 0) return 0;
    FacilityMaterialRequirement* requirement = findFacilityRequirement(facility,material);
    if(requirement == nullptr) return 0;
    const int missing = std::max(0,requirement->required - requirement->delivered);
    const int available = source.count(ItemKind::RawMaterial,material);
    const int delivered = std::min({requested,missing,available});
    if(delivered <= 0) return 0;
    if(!source.remove(ItemKind::RawMaterial,material,delivered)) return 0;
    requirement->delivered += delivered;
    if(facility.state == FacilityState::Planned) facility.state = FacilityState::UnderConstruction;
    return delivered;
}

inline bool applyFacilityConstructionWork(
    ConstructedFacility& facility,
    CharacterId worker,
    double work)
{
    if(facility.id == 0 || worker == 0 || work <= 0.0
       || facility.state == FacilityState::Operational
       || facility.state == FacilityState::Ruined
       || !facilityMaterialsComplete(facility)
       || facility.requiredWork <= 0.0) return false;

    facility.state = FacilityState::UnderConstruction;
    facility.lastWorkedBy = worker;
    facility.constructionWork = std::min(
        facility.requiredWork,
        facility.constructionWork + work);
    return facilityWorkComplete(facility);
}

inline bool activateConstructedFacility(
    ConstructedFacility& facility,
    StorageId linkedStorage,
    int minute)
{
    if(facility.id == 0 || facility.state == FacilityState::Operational
       || facility.state == FacilityState::Ruined
       || !facilityMaterialsComplete(facility)
       || !facilityWorkComplete(facility)) return false;
    if(facilityProvidesStorage(facility.kind) && linkedStorage == 0) return false;

    facility.state = FacilityState::Operational;
    facility.active = true;
    facility.linkedStorage = linkedStorage;
    facility.completedMinute = std::max(facility.startedMinute,std::max(0,minute));
    return true;
}

inline bool validConstructedFacility(const ConstructedFacility& facility)
{
    if(facility.id == 0 || !validFacilityKind(facility.kind) || !validFacilityState(facility.state)
       || facility.initiatedBy == 0 || facility.startedMinute < 0
       || facility.requiredWork <= 0.0 || facility.constructionWork < 0.0
       || facility.constructionWork > facility.requiredWork + 1e-9
       || facility.durability < 0.0 || facility.durability > 1.0
       || facility.requirements.empty()) return false;

    bool hasIncomplete = false;
    std::vector<int> seenMaterials;
    for(const auto& requirement : facility.requirements){
        if(requirement.material == MaterialKind::Unknown
           || requirement.required <= 0 || requirement.delivered < 0
           || requirement.delivered > requirement.required) return false;
        const int material = static_cast<int>(requirement.material);
        if(std::find(seenMaterials.begin(),seenMaterials.end(),material) != seenMaterials.end()) return false;
        seenMaterials.push_back(material);
        if(requirement.delivered < requirement.required) hasIncomplete = true;
    }

    if(facility.state == FacilityState::Operational){
        if(!facility.active || hasIncomplete || !facilityWorkComplete(facility)
           || facility.completedMinute < facility.startedMinute) return false;
        if(facilityProvidesStorage(facility.kind) && facility.linkedStorage == 0) return false;
    }else{
        if(facility.active || facility.completedMinute >= 0) return false;
        if(facility.linkedStorage != 0) return false;
    }
    return true;
}

} // namespace lifelens
