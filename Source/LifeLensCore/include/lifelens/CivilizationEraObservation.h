#pragma once

#include <array>
#include <string>
#include <vector>
#include "CivilizationProgression.h"

namespace lifelens {

// Observer output only: never included by decisions, recipes or save codecs.
struct CivilizationEraEvidence { std::string id; bool satisfied=false; };
struct CivilizationEraDefinition {
    const char* id;
    int ordinal;
    std::vector<const char*> requirements;
};
inline const std::array<CivilizationEraDefinition,5> CivilizationEraRegistry={{
    {"NaturalSurvival",0,{}},
    {"EarlySettlement",1,{"OperationalSleep","SettlementInfrastructure","BasicLivingCapabilities"}},
    {"AgrarianSettlement",2,{"OperationalSleep","SettlementInfrastructure","BasicLivingCapabilities",
        "OperationalCultivation","CultivateFood","OperationalPlot","ManagedFoodProduction"}},
    {"CopperMetallurgy",3,{"OperationalSleep","SettlementInfrastructure","BasicLivingCapabilities",
        "OperationalCopperSmelting","SmeltMetal","OperationalFurnace","MetallurgicalProduction"}},
    {"BronzeTechnology",4,{"OperationalSleep","SettlementInfrastructure","BasicLivingCapabilities",
        "OperationalTinSmelting","OperationalBronzeAlloying","AlloyMetal","OperationalFurnace","AdvancedTooling"}}
}};

struct CivilizationEraObservation {
    std::string id="NaturalSurvival";
    int ordinal=0;
    std::vector<CivilizationEraEvidence> evidence;
    std::string nextEra;
    std::vector<CivilizationEraEvidence> nextEraRequirements;
};

inline CivilizationEraObservation evaluateCivilizationEra(
    const std::vector<CivilizationEraEvidence>& evidence)
{
    const auto requirementsFor=[&](const CivilizationEraDefinition& definition){
        std::vector<CivilizationEraEvidence> requirements;
        for(const char* id:definition.requirements){
            bool satisfied=false;
            for(const auto& fact:evidence) if(fact.id==id){ satisfied=fact.satisfied; break; }
            requirements.push_back({id,satisfied});
        }
        return requirements;
    };
    CivilizationEraObservation result;
    for(const auto& definition:CivilizationEraRegistry){
        const auto requirements=requirementsFor(definition);
        if(std::all_of(requirements.begin(),requirements.end(),
                [](const auto& requirement){ return requirement.satisfied; })){
            result.id=definition.id;
            result.ordinal=definition.ordinal;
            result.evidence=requirements;
        }
    }
    if(result.ordinal+1<static_cast<int>(CivilizationEraRegistry.size())){
        const auto& next=CivilizationEraRegistry[result.ordinal+1];
        result.nextEra=next.id;
        result.nextEraRequirements=requirementsFor(next);
    }
    return result;
}

inline CivilizationEraObservation buildCivilizationEraObservation(
    const World& world,
    const std::vector<CivilizationTechnologyPopulationStatus>& technologies,
    const std::vector<CivilizationTransformationStatus>& transformations)
{
    // Bounded registries + a single facility pass; reuse the Observer's existing
    // population and transformation summaries rather than recomputing them.
    bool sleep=false,storage=false,fire=false,shelter=false,plot=false,furnace=false;
    for(const auto& facility:world.facilities){
        if(!facilityOperationalAndActive(facility)) continue;
        sleep=sleep || facilityProvidesSleep(facility.kind);
        storage=storage || facility.kind==FacilityKind::PrimitiveStorage;
        fire=fire || facility.kind==FacilityKind::FirePit;
        shelter=shelter || facility.kind==FacilityKind::Shelter;
        plot=plot || facility.kind==FacilityKind::CultivatedPlot;
        furnace=furnace || facility.kind==FacilityKind::Furnace;
    }
    const auto operational=[&](TechnologyId id){
        const auto* status=findTechnologyPopulationStatus(technologies,id);
        return status!=nullptr && status->operationalResidentCount>0;
    };
    const auto active=[&](CivilizationTransformationId id){
        for(const auto& status:transformations) if(status.transformation==id) return status.active;
        return false;
    };
    bool storeGoods=false,controlFire=false,carryLiquid=false;
    bool cultivateFood=false,smeltMetal=false,alloyMetal=false;
    for(const auto& resident:world.characters){
        if(!resident.alive) continue;
        storeGoods=storeGoods || capabilityAvailable(world,resident,CapabilityId::StoreGoods);
        controlFire=controlFire || capabilityAvailable(world,resident,CapabilityId::ControlFire);
        carryLiquid=carryLiquid || capabilityAvailable(world,resident,CapabilityId::CarryLiquid);
        cultivateFood=cultivateFood || capabilityAvailable(world,resident,CapabilityId::CultivateFood);
        smeltMetal=smeltMetal || capabilityAvailable(world,resident,CapabilityId::SmeltMetal);
        alloyMetal=alloyMetal || capabilityAvailable(world,resident,CapabilityId::AlloyMetal);
    }
    const int supportingFacilities=static_cast<int>(storage)+static_cast<int>(fire)+static_cast<int>(shelter);
    return evaluateCivilizationEra({
        {"OperationalSleep",sleep},
        {"SettlementInfrastructure",supportingFacilities>=2
            || (supportingFacilities>=1 && active(CivilizationTransformationId::ResourceBuffering))},
        {"BasicLivingCapabilities",static_cast<int>(storeGoods)+static_cast<int>(controlFire)
            +static_cast<int>(carryLiquid)>=2},
        {"OperationalCultivation",operational(TechnologyId::Cultivation)},
        {"CultivateFood",cultivateFood}, {"OperationalPlot",plot},
        {"ManagedFoodProduction",active(CivilizationTransformationId::ManagedFoodProduction)},
        {"OperationalCopperSmelting",operational(TechnologyId::CopperSmelting)},
        {"SmeltMetal",smeltMetal}, {"OperationalFurnace",furnace},
        {"MetallurgicalProduction",active(CivilizationTransformationId::MetallurgicalProduction)},
        {"OperationalTinSmelting",operational(TechnologyId::TinSmelting)},
        {"OperationalBronzeAlloying",operational(TechnologyId::BronzeAlloying)},
        {"AlloyMetal",alloyMetal},
        {"AdvancedTooling",active(CivilizationTransformationId::AdvancedTooling)}
    });
}

} // namespace lifelens
