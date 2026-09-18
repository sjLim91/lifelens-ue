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

    // Heat-production authority. Presentation only reads these values.
    int fuelUnits = 0;
    int charcoalUnits = 0;
    double heatLevel = 0.0;
    bool lit = false;
    int burnMinutesRemaining = 0;
    int lastFireMinute = -1;

    // Furnace-only runtime. v1 metalworking deliberately supports copper only;
    // tin/iron remain ores until later alloy/high-temperature progression.
    int oreUnits = 0;
    int metalUnits = 0;
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

inline bool facilityProducesHeat(FacilityKind kind)
{
    return kind == FacilityKind::FirePit || kind == FacilityKind::Furnace;
}

inline bool facilitySupportsCrafting(FacilityKind kind)
{
    return kind == FacilityKind::WorkSurface;
}

inline bool facilityProvidesSleep(FacilityKind kind)
{
    return kind == FacilityKind::SleepingPlace || kind == FacilityKind::Shelter;
}

inline bool facilityProvidesWeatherProtection(FacilityKind kind)
{
    return kind == FacilityKind::Shelter;
}

inline bool facilityOperationalAndActive(const ConstructedFacility& facility)
{
    return facility.state==FacilityState::Operational
        && facility.active
        && facility.durability>1e-9;
}

inline double facilityEffectiveness01(const ConstructedFacility& facility)
{
    if(!facilityOperationalAndActive(facility)) return 0.0;
    return std::clamp(facility.durability,0.0,1.0);
}

inline MaterialKind facilityRepairMaterial(FacilityKind kind)
{
    switch(kind){
        case FacilityKind::WorkSurface: return MaterialKind::Wood;
        case FacilityKind::SleepingPlace: return MaterialKind::Fiber;
        case FacilityKind::Shelter: return MaterialKind::Wood;
        default: return MaterialKind::Unknown;
    }
}

inline bool facilitySupportsMaintenance(FacilityKind kind)
{
    return facilityRepairMaterial(kind)!=MaterialKind::Unknown;
}

inline double facilityWearPerUse(FacilityKind kind)
{
    switch(kind){
        case FacilityKind::WorkSurface: return 0.018;
        case FacilityKind::SleepingPlace: return 0.012;
        case FacilityKind::Shelter: return 0.006;
        default: return 0.0;
    }
}

inline bool ruinConstructedFacility(ConstructedFacility& facility)
{
    if(facility.id==0 || facility.state!=FacilityState::Operational) return false;
    facility.durability=0.0;
    facility.state=FacilityState::Ruined;
    facility.active=false;
    facility.lit=false;
    facility.heatLevel=0.0;
    facility.burnMinutesRemaining=0;
    return true;
}

inline bool applyFacilityWear(ConstructedFacility& facility,double amount)
{
    if(!facilityOperationalAndActive(facility) || amount<=0.0) return false;
    facility.durability=std::max(0.0,facility.durability-amount);
    if(facility.durability<=1e-9) ruinConstructedFacility(facility);
    return true;
}

struct FacilityRepairResult {
    bool repaired=false;
    FacilityId facilityId=0;
    FacilityKind kind=FacilityKind::WorkSurface;
    MaterialKind material=MaterialKind::Unknown;
    double durabilityBefore=0.0;
    double durabilityAfter=0.0;
};

inline FacilityRepairResult repairConstructedFacility(
    ConstructedFacility& facility,
    CharacterId worker,
    Inventory& inventory,
    double craftingSkill)
{
    FacilityRepairResult result;
    result.facilityId=facility.id;
    result.kind=facility.kind;
    result.material=facilityRepairMaterial(facility.kind);
    result.durabilityBefore=facility.durability;
    result.durabilityAfter=facility.durability;

    if(worker==0 || !facilityOperationalAndActive(facility)
       || !facilitySupportsMaintenance(facility.kind)
       || facility.durability>=0.98
       || result.material==MaterialKind::Unknown) return result;

    if(!inventory.remove(ItemKind::RawMaterial,result.material,1)) return result;

    const double restored=0.18+0.22*std::clamp(craftingSkill,0.0,1.0);
    facility.durability=std::min(1.0,facility.durability+restored);
    facility.lastWorkedBy=worker;
    result.durabilityAfter=facility.durability;
    result.repaired=result.durabilityAfter>result.durabilityBefore;
    return result;
}

inline FacilityConstructionSpec facilityConstructionSpec(FacilityKind kind)
{
    switch(kind){
        case FacilityKind::PrimitiveStorage:
            return {kind,8.0,{
                {MaterialKind::Wood,4,0},
                {MaterialKind::Fiber,2,0}
            }};
        case FacilityKind::FirePit:
            return {kind,6.0,{
                {MaterialKind::Stone,5,0},
                {MaterialKind::Wood,2,0}
            }};
        case FacilityKind::WorkSurface:
            // A stable low work platform: no free starting furniture, only a
            // physically buildable surface from gathered wood and stone.
            return {kind,7.0,{
                {MaterialKind::Wood,3,0},
                {MaterialKind::Stone,2,0}
            }};
        case FacilityKind::SleepingPlace:
            // Primitive bedding is deliberately cheap enough to precede a full
            // shelter, but still consumes real gathered material and work.
            return {kind,5.0,{
                {MaterialKind::Fiber,4,0},
                {MaterialKind::Wood,2,0}
            }};
        case FacilityKind::Shelter:
            return {kind,14.0,{
                {MaterialKind::Wood,8,0},
                {MaterialKind::Fiber,5,0}
            }};
        case FacilityKind::Furnace:
            // A small clay-lined stone furnace. Knowledge gating belongs to the
            // progression layer; this contract owns only physical requirements.
            return {kind,12.0,{
                {MaterialKind::Stone,8,0},
                {MaterialKind::Clay,6,0}
            }};
        default:
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

constexpr int PrimitiveFireBurnMinutesPerWoodUnit = 30;
constexpr int PrimitiveFurnaceSmeltMinutesPerCopperUnit = 45;

inline int addFirePitWoodFuel(
    ConstructedFacility& facility,
    Inventory& source,
    int requested)
{
    if(facility.id == 0 || facility.kind != FacilityKind::FirePit
       || facility.state != FacilityState::Operational || !facility.active
       || requested <= 0) return 0;
    const int available = source.count(ItemKind::RawMaterial,MaterialKind::Wood);
    const int added = std::min(requested,available);
    if(added <= 0) return 0;
    if(!source.remove(ItemKind::RawMaterial,MaterialKind::Wood,added)) return 0;
    facility.fuelUnits += added;
    return added;
}

inline bool igniteFirePit(ConstructedFacility& facility,int minute)
{
    if(facility.id == 0 || facility.kind != FacilityKind::FirePit
       || facility.state != FacilityState::Operational || !facility.active
       || facility.fuelUnits <= 0 || facility.lit) return false;
    facility.lit = true;
    facility.heatLevel = 1.0;
    facility.burnMinutesRemaining = PrimitiveFireBurnMinutesPerWoodUnit;
    facility.lastFireMinute = std::max(0,minute);
    return true;
}

inline void advanceFirePitOneMinute(ConstructedFacility& facility,int minute)
{
    if(facility.kind != FacilityKind::FirePit
       || facility.state != FacilityState::Operational || !facility.active){
        facility.lit = false;
        facility.heatLevel = 0.0;
        facility.burnMinutesRemaining = 0;
        return;
    }

    if(!facility.lit){
        facility.heatLevel = std::max(0.0,facility.heatLevel - 0.04);
        if(facility.heatLevel <= 1e-9) facility.heatLevel = 0.0;
        return;
    }

    facility.lastFireMinute = std::max(facility.lastFireMinute,std::max(0,minute));
    facility.heatLevel = std::max(0.45,facility.heatLevel - 0.01);
    if(facility.burnMinutesRemaining > 0) --facility.burnMinutesRemaining;
    if(facility.burnMinutesRemaining > 0) return;

    if(facility.fuelUnits > 0){
        --facility.fuelUnits;
        ++facility.charcoalUnits;
    }
    if(facility.fuelUnits > 0){
        facility.burnMinutesRemaining = PrimitiveFireBurnMinutesPerWoodUnit;
        facility.heatLevel = 1.0;
    }else{
        facility.lit = false;
        facility.burnMinutesRemaining = 0;
        facility.heatLevel = 0.20;
    }
}

inline int collectFirePitCharcoal(
    ConstructedFacility& facility,
    Inventory& destination,
    int requested)
{
    if(facility.id == 0 || facility.kind != FacilityKind::FirePit
       || facility.state != FacilityState::Operational || !facility.active
       || requested <= 0 || facility.charcoalUnits <= 0) return 0;
    const int collected = std::min(requested,facility.charcoalUnits);
    facility.charcoalUnits -= collected;
    destination.add({ItemKind::RawMaterial,MaterialKind::Charcoal,collected,0.55,1.0});
    return collected;
}

inline int loadFurnaceCopperCharge(
    ConstructedFacility& facility,
    Inventory& source,
    int requested)
{
    if(facility.id == 0 || facility.kind != FacilityKind::Furnace
       || facility.state != FacilityState::Operational || !facility.active
       || facility.lit || requested <= 0) return 0;
    const int availableOre = source.count(ItemKind::RawMaterial,MaterialKind::CopperOre);
    const int availableCharcoal = source.count(ItemKind::RawMaterial,MaterialKind::Charcoal);
    const int loaded = std::min({requested,availableOre,availableCharcoal,2});
    if(loaded <= 0) return 0;
    if(!source.remove(ItemKind::RawMaterial,MaterialKind::CopperOre,loaded)) return 0;
    if(!source.remove(ItemKind::RawMaterial,MaterialKind::Charcoal,loaded)){
        source.add({ItemKind::RawMaterial,MaterialKind::CopperOre,loaded,0.5,1.0});
        return 0;
    }
    facility.oreUnits += loaded;
    facility.fuelUnits += loaded;
    return loaded;
}

inline bool igniteFurnace(ConstructedFacility& facility,int minute)
{
    if(facility.id == 0 || facility.kind != FacilityKind::Furnace
       || facility.state != FacilityState::Operational || !facility.active
       || facility.oreUnits <= 0 || facility.fuelUnits <= 0 || facility.lit) return false;
    facility.lit = true;
    facility.heatLevel = 1.0;
    facility.burnMinutesRemaining = PrimitiveFurnaceSmeltMinutesPerCopperUnit;
    facility.lastFireMinute = std::max(0,minute);
    return true;
}

inline void advanceFurnaceOneMinute(ConstructedFacility& facility,int minute)
{
    if(facility.kind != FacilityKind::Furnace
       || facility.state != FacilityState::Operational || !facility.active){
        facility.lit = false;
        facility.heatLevel = 0.0;
        facility.burnMinutesRemaining = 0;
        return;
    }
    if(!facility.lit){
        facility.heatLevel = std::max(0.0,facility.heatLevel - 0.025);
        if(facility.heatLevel <= 1e-9) facility.heatLevel = 0.0;
        return;
    }

    facility.lastFireMinute = std::max(facility.lastFireMinute,std::max(0,minute));
    facility.heatLevel = std::max(0.72,facility.heatLevel - 0.004);
    if(facility.burnMinutesRemaining > 0) --facility.burnMinutesRemaining;
    if(facility.burnMinutesRemaining > 0) return;

    if(facility.oreUnits > 0 && facility.fuelUnits > 0){
        --facility.oreUnits;
        --facility.fuelUnits;
        ++facility.metalUnits;
    }
    if(facility.oreUnits > 0 && facility.fuelUnits > 0){
        facility.burnMinutesRemaining = PrimitiveFurnaceSmeltMinutesPerCopperUnit;
        facility.heatLevel = 1.0;
    }else{
        facility.lit = false;
        facility.burnMinutesRemaining = 0;
        facility.heatLevel = 0.35;
    }
}

inline int collectFurnaceCopper(
    ConstructedFacility& facility,
    Inventory& destination,
    int requested)
{
    if(facility.id == 0 || facility.kind != FacilityKind::Furnace
       || facility.state != FacilityState::Operational || !facility.active
       || facility.lit || requested <= 0 || facility.metalUnits <= 0) return 0;
    const int collected = std::min(requested,facility.metalUnits);
    facility.metalUnits -= collected;
    destination.add({ItemKind::RawMaterial,MaterialKind::CopperMetal,collected,0.60,1.0});
    return collected;
}

inline bool validConstructedFacility(const ConstructedFacility& facility)
{
    if(facility.id == 0 || !validFacilityKind(facility.kind) || !validFacilityState(facility.state)
       || facility.initiatedBy == 0 || facility.startedMinute < 0
       || facility.requiredWork <= 0.0 || facility.constructionWork < 0.0
       || facility.constructionWork > facility.requiredWork + 1e-9
       || facility.durability < 0.0 || facility.durability > 1.0
       || facility.requirements.empty()
       || facility.fuelUnits < 0 || facility.charcoalUnits < 0
       || facility.oreUnits < 0 || facility.metalUnits < 0
       || facility.heatLevel < 0.0 || facility.heatLevel > 1.0
       || facility.burnMinutesRemaining < 0) return false;

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
        if(!facility.active || facility.durability<=0.0
           || hasIncomplete || !facilityWorkComplete(facility)
           || facility.completedMinute < facility.startedMinute) return false;
        if(facilityProvidesStorage(facility.kind) && facility.linkedStorage == 0) return false;
    }else if(facility.state == FacilityState::Ruined){
        if(facility.active || facility.durability>1e-9
           || hasIncomplete || !facilityWorkComplete(facility)
           || facility.completedMinute < facility.startedMinute) return false;
        if(facility.lit || facility.heatLevel!=0.0
           || facility.burnMinutesRemaining!=0) return false;
    }else{
        if(facility.active || facility.completedMinute >= 0) return false;
        if(facility.linkedStorage != 0) return false;
        if(facility.fuelUnits != 0 || facility.charcoalUnits != 0
           || facility.oreUnits != 0 || facility.metalUnits != 0
           || facility.heatLevel != 0.0 || facility.lit
           || facility.burnMinutesRemaining != 0 || facility.lastFireMinute >= 0) return false;
    }

    if(!facilityProducesHeat(facility.kind)){
        if(facility.fuelUnits != 0 || facility.charcoalUnits != 0
           || facility.oreUnits != 0 || facility.metalUnits != 0
           || facility.heatLevel != 0.0 || facility.lit
           || facility.burnMinutesRemaining != 0 || facility.lastFireMinute >= 0) return false;
    }
    if(facility.kind==FacilityKind::FirePit && (facility.oreUnits!=0 || facility.metalUnits!=0)) return false;
    if(facility.kind==FacilityKind::Furnace && facility.charcoalUnits!=0) return false;
    if(facility.lit){
        if(!facilityProducesHeat(facility.kind) || facility.fuelUnits <= 0
           || facility.heatLevel <= 0.0 || facility.burnMinutesRemaining <= 0
           || facility.lastFireMinute < 0) return false;
        if(facility.kind==FacilityKind::Furnace && facility.oreUnits<=0) return false;
    }
    return true;
}

} // namespace lifelens
