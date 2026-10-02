#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "CivilizationProgression.h"
#include "Facility.h"
#include "World.h"

namespace lifelens {

// Observer-only civilization summary. These stable ids describe what the
// authoritative world can currently operate; they never unlock or mutate
// technology, facilities, AI decisions, or snapshot state.
enum class CivilizationEraId : std::uint8_t {
    NaturalSurvival=0,
    EarlySettlement=1,
    AgrarianSettlement=2,
    CopperMetallurgy=3,
    BronzeTechnology=4
};

enum class CivilizationEraEvidenceId : std::uint8_t {
    OperationalSleepInfrastructure=0,
    OperationalPrimitiveStorage,
    OperationalFirePit,
    OperationalShelter,
    StoreGoodsCapability,
    ControlFireCapability,
    CarryLiquidCapability,
    ResourceBuffering,
    CultivateFoodCapability,
    OperationalCultivatedPlot,
    ManagedFoodProduction,
    CopperSmeltingOperational,
    SmeltMetalCapability,
    OperationalFurnace,
    MetallurgicalProduction,
    TinSmeltingOperational,
    BronzeAlloyingOperational,
    AlloyMetalCapability,
    BronzeToolOperational,
    AdvancedTooling,
    Count
};

inline const char* civilizationEraIdName(CivilizationEraId id)
{
    switch(id){
        case CivilizationEraId::NaturalSurvival: return "NaturalSurvival";
        case CivilizationEraId::EarlySettlement: return "EarlySettlement";
        case CivilizationEraId::AgrarianSettlement: return "AgrarianSettlement";
        case CivilizationEraId::CopperMetallurgy: return "CopperMetallurgy";
        case CivilizationEraId::BronzeTechnology: return "BronzeTechnology";
    }
    return "NaturalSurvival";
}

inline const char* civilizationEraEvidenceIdName(CivilizationEraEvidenceId id)
{
    switch(id){
        case CivilizationEraEvidenceId::OperationalSleepInfrastructure:
            return "OperationalSleepInfrastructure";
        case CivilizationEraEvidenceId::OperationalPrimitiveStorage:
            return "OperationalPrimitiveStorage";
        case CivilizationEraEvidenceId::OperationalFirePit:
            return "OperationalFirePit";
        case CivilizationEraEvidenceId::OperationalShelter:
            return "OperationalShelter";
        case CivilizationEraEvidenceId::StoreGoodsCapability:
            return "StoreGoodsCapability";
        case CivilizationEraEvidenceId::ControlFireCapability:
            return "ControlFireCapability";
        case CivilizationEraEvidenceId::CarryLiquidCapability:
            return "CarryLiquidCapability";
        case CivilizationEraEvidenceId::ResourceBuffering:
            return "ResourceBuffering";
        case CivilizationEraEvidenceId::CultivateFoodCapability:
            return "CultivateFoodCapability";
        case CivilizationEraEvidenceId::OperationalCultivatedPlot:
            return "OperationalCultivatedPlot";
        case CivilizationEraEvidenceId::ManagedFoodProduction:
            return "ManagedFoodProduction";
        case CivilizationEraEvidenceId::CopperSmeltingOperational:
            return "CopperSmeltingOperational";
        case CivilizationEraEvidenceId::SmeltMetalCapability:
            return "SmeltMetalCapability";
        case CivilizationEraEvidenceId::OperationalFurnace:
            return "OperationalFurnace";
        case CivilizationEraEvidenceId::MetallurgicalProduction:
            return "MetallurgicalProduction";
        case CivilizationEraEvidenceId::TinSmeltingOperational:
            return "TinSmeltingOperational";
        case CivilizationEraEvidenceId::BronzeAlloyingOperational:
            return "BronzeAlloyingOperational";
        case CivilizationEraEvidenceId::AlloyMetalCapability:
            return "AlloyMetalCapability";
        case CivilizationEraEvidenceId::BronzeToolOperational:
            return "BronzeToolOperational";
        case CivilizationEraEvidenceId::AdvancedTooling:
            return "AdvancedTooling";
        case CivilizationEraEvidenceId::Count:
            break;
    }
    return "Unknown";
}

struct CivilizationEraRequirementObservation {
    CivilizationEraEvidenceId id=
        CivilizationEraEvidenceId::OperationalSleepInfrastructure;
    bool satisfied=false;
    bool mandatory=false;
};

struct CivilizationEraObservation {
    CivilizationEraId currentEra=CivilizationEraId::NaturalSurvival;
    int ordinal=0;
    std::size_t minimumSatisfied=0;
    std::vector<CivilizationEraRequirementObservation> evidence;

    bool hasNextEra=false;
    CivilizationEraId nextEra=CivilizationEraId::NaturalSurvival;
    int nextOrdinal=0;
    std::size_t nextMinimumSatisfied=0;
    std::vector<CivilizationEraRequirementObservation> nextRequirements;
};

struct CivilizationEraDefinition {
    CivilizationEraId id=CivilizationEraId::NaturalSurvival;
    int ordinal=0;
    std::vector<CivilizationEraEvidenceId> requirements;
    std::vector<CivilizationEraEvidenceId> mandatory;
    std::size_t minimumSatisfied=0;
};

inline const std::array<CivilizationEraDefinition,4>&
civilizationEraRegistry()
{
    // Bounded observer registry. Early settlement deliberately combines one
    // mandatory lived-in sleep facility with at least two independent signs of
    // durable settlement. Later stages require actual operational/production
    // evidence rather than mere observation or elapsed time.
    static const std::array<CivilizationEraDefinition,4> registry={{
        {
            CivilizationEraId::EarlySettlement,
            1,
            {
                CivilizationEraEvidenceId::OperationalSleepInfrastructure,
                CivilizationEraEvidenceId::OperationalPrimitiveStorage,
                CivilizationEraEvidenceId::OperationalFirePit,
                CivilizationEraEvidenceId::OperationalShelter,
                CivilizationEraEvidenceId::StoreGoodsCapability,
                CivilizationEraEvidenceId::ControlFireCapability,
                CivilizationEraEvidenceId::CarryLiquidCapability,
                CivilizationEraEvidenceId::ResourceBuffering
            },
            {CivilizationEraEvidenceId::OperationalSleepInfrastructure},
            3
        },
        {
            CivilizationEraId::AgrarianSettlement,
            2,
            {
                CivilizationEraEvidenceId::CultivateFoodCapability,
                CivilizationEraEvidenceId::OperationalCultivatedPlot,
                CivilizationEraEvidenceId::ManagedFoodProduction
            },
            {
                CivilizationEraEvidenceId::CultivateFoodCapability,
                CivilizationEraEvidenceId::OperationalCultivatedPlot,
                CivilizationEraEvidenceId::ManagedFoodProduction
            },
            3
        },
        {
            CivilizationEraId::CopperMetallurgy,
            3,
            {
                CivilizationEraEvidenceId::CopperSmeltingOperational,
                CivilizationEraEvidenceId::SmeltMetalCapability,
                CivilizationEraEvidenceId::OperationalFurnace,
                CivilizationEraEvidenceId::MetallurgicalProduction
            },
            {
                CivilizationEraEvidenceId::CopperSmeltingOperational,
                CivilizationEraEvidenceId::SmeltMetalCapability,
                CivilizationEraEvidenceId::OperationalFurnace,
                CivilizationEraEvidenceId::MetallurgicalProduction
            },
            4
        },
        {
            CivilizationEraId::BronzeTechnology,
            4,
            {
                CivilizationEraEvidenceId::TinSmeltingOperational,
                CivilizationEraEvidenceId::BronzeAlloyingOperational,
                CivilizationEraEvidenceId::AlloyMetalCapability,
                CivilizationEraEvidenceId::BronzeToolOperational,
                CivilizationEraEvidenceId::AdvancedTooling
            },
            {
                CivilizationEraEvidenceId::TinSmeltingOperational,
                CivilizationEraEvidenceId::BronzeAlloyingOperational,
                CivilizationEraEvidenceId::AlloyMetalCapability,
                CivilizationEraEvidenceId::BronzeToolOperational,
                CivilizationEraEvidenceId::AdvancedTooling
            },
            5
        }
    }};
    return registry;
}

using CivilizationEraEvidenceTable=
    std::array<bool,static_cast<std::size_t>(
        CivilizationEraEvidenceId::Count)>;

inline bool civilizationEraTransformationActive(
    const std::vector<CivilizationTransformationStatus>& transformations,
    CivilizationTransformationId id)
{
    for(const auto& status:transformations){
        if(status.transformation==id) return status.active;
    }
    return false;
}

inline bool civilizationEraTechnologyOperational(
    const std::vector<CivilizationTechnologyPopulationStatus>& technologies,
    TechnologyId id)
{
    for(const auto& status:technologies){
        if(status.technology==id){
            return status.operationalResidentCount>0
                && status.state!=TechnologyPopulationState::Lost;
        }
    }
    return false;
}

inline bool civilizationEraTechnologyOperationalOrAdopted(
    const std::vector<CivilizationTechnologyPopulationStatus>& technologies,
    TechnologyId id)
{
    for(const auto& status:technologies){
        if(status.technology==id){
            return status.state!=TechnologyPopulationState::Lost
                && (status.operationalResidentCount>0
                    || status.adoptedResidentCount>0);
        }
    }
    return false;
}

inline bool civilizationEraCapabilityAvailable(
    const World& world,
    CapabilityId capability)
{
    for(const Character& resident:world.characters){
        if(resident.alive && capabilityAvailable(world,resident,capability)){
            return true;
        }
    }
    return false;
}

inline bool civilizationEraOperationalFacility(
    const World& world,
    FacilityKind kind)
{
    for(const ConstructedFacility& facility:world.facilities){
        if(facility.kind==kind && facilityOperationalAndActive(facility)){
            return true;
        }
    }
    return false;
}

inline CivilizationEraEvidenceTable buildCivilizationEraEvidence(
    const World& world,
    const std::vector<CivilizationTechnologyPopulationStatus>& technologies,
    const std::vector<CivilizationTransformationStatus>& transformations)
{
    CivilizationEraEvidenceTable evidence{};
    const auto set=[&](CivilizationEraEvidenceId id,bool value){
        evidence[static_cast<std::size_t>(id)]=value;
    };

    bool sleepInfrastructure=false;
    for(const ConstructedFacility& facility:world.facilities){
        if(facilityProvidesSleep(facility.kind)
           && facilityOperationalAndActive(facility)){
            sleepInfrastructure=true;
            break;
        }
    }

    set(CivilizationEraEvidenceId::OperationalSleepInfrastructure,
        sleepInfrastructure);
    set(CivilizationEraEvidenceId::OperationalPrimitiveStorage,
        civilizationEraOperationalFacility(
            world,FacilityKind::PrimitiveStorage));
    set(CivilizationEraEvidenceId::OperationalFirePit,
        civilizationEraOperationalFacility(world,FacilityKind::FirePit));
    set(CivilizationEraEvidenceId::OperationalShelter,
        civilizationEraOperationalFacility(world,FacilityKind::Shelter));
    set(CivilizationEraEvidenceId::StoreGoodsCapability,
        civilizationEraCapabilityAvailable(world,CapabilityId::StoreGoods));
    set(CivilizationEraEvidenceId::ControlFireCapability,
        civilizationEraCapabilityAvailable(world,CapabilityId::ControlFire));
    set(CivilizationEraEvidenceId::CarryLiquidCapability,
        civilizationEraCapabilityAvailable(world,CapabilityId::CarryLiquid));
    set(CivilizationEraEvidenceId::ResourceBuffering,
        civilizationEraTransformationActive(
            transformations,
            CivilizationTransformationId::ResourceBuffering));

    set(CivilizationEraEvidenceId::CultivateFoodCapability,
        civilizationEraCapabilityAvailable(world,CapabilityId::CultivateFood));
    set(CivilizationEraEvidenceId::OperationalCultivatedPlot,
        civilizationEraOperationalFacility(
            world,FacilityKind::CultivatedPlot));
    set(CivilizationEraEvidenceId::ManagedFoodProduction,
        civilizationEraTransformationActive(
            transformations,
            CivilizationTransformationId::ManagedFoodProduction));

    set(CivilizationEraEvidenceId::CopperSmeltingOperational,
        civilizationEraTechnologyOperational(
            technologies,TechnologyId::CopperSmelting));
    set(CivilizationEraEvidenceId::SmeltMetalCapability,
        civilizationEraCapabilityAvailable(world,CapabilityId::SmeltMetal));
    set(CivilizationEraEvidenceId::OperationalFurnace,
        civilizationEraOperationalFacility(world,FacilityKind::Furnace));
    set(CivilizationEraEvidenceId::MetallurgicalProduction,
        civilizationEraTransformationActive(
            transformations,
            CivilizationTransformationId::MetallurgicalProduction));

    set(CivilizationEraEvidenceId::TinSmeltingOperational,
        civilizationEraTechnologyOperational(
            technologies,TechnologyId::TinSmelting));
    set(CivilizationEraEvidenceId::BronzeAlloyingOperational,
        civilizationEraTechnologyOperational(
            technologies,TechnologyId::BronzeAlloying));
    set(CivilizationEraEvidenceId::AlloyMetalCapability,
        civilizationEraCapabilityAvailable(world,CapabilityId::AlloyMetal));
    set(CivilizationEraEvidenceId::BronzeToolOperational,
        civilizationEraTechnologyOperationalOrAdopted(
            technologies,TechnologyId::BronzeAxe)
        || civilizationEraTechnologyOperationalOrAdopted(
            technologies,TechnologyId::BronzePick));
    set(CivilizationEraEvidenceId::AdvancedTooling,
        civilizationEraTransformationActive(
            transformations,
            CivilizationTransformationId::AdvancedTooling));

    return evidence;
}

inline bool civilizationEraEvidenceSatisfied(
    const CivilizationEraEvidenceTable& evidence,
    CivilizationEraEvidenceId id)
{
    return evidence[static_cast<std::size_t>(id)];
}

inline bool civilizationEraDefinitionSatisfied(
    const CivilizationEraDefinition& definition,
    const CivilizationEraEvidenceTable& evidence)
{
    std::size_t satisfied=0;
    for(const auto id:definition.requirements){
        if(civilizationEraEvidenceSatisfied(evidence,id)) ++satisfied;
    }
    if(satisfied<definition.minimumSatisfied) return false;
    for(const auto id:definition.mandatory){
        if(!civilizationEraEvidenceSatisfied(evidence,id)) return false;
    }
    return true;
}

inline std::vector<CivilizationEraRequirementObservation>
civilizationEraRequirementObservations(
    const CivilizationEraDefinition& definition,
    const CivilizationEraEvidenceTable& evidence)
{
    std::vector<CivilizationEraRequirementObservation> result;
    result.reserve(definition.requirements.size());
    for(const auto id:definition.requirements){
        result.push_back({
            id,
            civilizationEraEvidenceSatisfied(evidence,id),
            std::find(
                definition.mandatory.begin(),
                definition.mandatory.end(),
                id)!=definition.mandatory.end()
        });
    }
    return result;
}

inline CivilizationEraObservation buildCivilizationEraObservation(
    const World& world,
    const std::vector<CivilizationTechnologyPopulationStatus>& technologies,
    const std::vector<CivilizationTransformationStatus>& transformations)
{
    const CivilizationEraEvidenceTable evidence=
        buildCivilizationEraEvidence(world,technologies,transformations);

    CivilizationEraObservation result;
    const CivilizationEraDefinition* currentDefinition=nullptr;

    // Highest currently-operational description wins. No value is persisted,
    // so loss of people, facilities, capability, or production can naturally
    // move the observer summary backward on the next read.
    for(const auto& definition:civilizationEraRegistry()){
        if(!civilizationEraDefinitionSatisfied(definition,evidence)) continue;
        if(definition.ordinal>result.ordinal){
            result.currentEra=definition.id;
            result.ordinal=definition.ordinal;
            currentDefinition=&definition;
        }
    }

    if(currentDefinition!=nullptr){
        result.minimumSatisfied=currentDefinition->minimumSatisfied;
        result.evidence=civilizationEraRequirementObservations(
            *currentDefinition,evidence);
    }

    for(const auto& definition:civilizationEraRegistry()){
        if(definition.ordinal!=result.ordinal+1) continue;
        result.hasNextEra=true;
        result.nextEra=definition.id;
        result.nextOrdinal=definition.ordinal;
        result.nextMinimumSatisfied=definition.minimumSatisfied;
        result.nextRequirements=civilizationEraRequirementObservations(
            definition,evidence);
        break;
    }

    return result;
}

} // namespace lifelens
