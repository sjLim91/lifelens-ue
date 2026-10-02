#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <vector>

#include "SettlementKnowledge.h"
#include "SettlementTrade.h"
#include "SocietyEconomy.h"

namespace lifelens {

// Production specialization is a slow settlement-level coordination signal.
// Utility consumes it only at fixed six-hour boundaries; the read model itself
// remains available at any minute for observation. A fixed boundary avoids a
// mutable decision cache and therefore preserves save/restore determinism.
inline constexpr int SettlementProductionDecisionIntervalMinutes = 6*60;

inline bool settlementProductionDecisionWindow(int minute)
{
    return minute>=0
        && minute%SettlementProductionDecisionIntervalMinutes==0;
}

enum class SettlementProductionKind : std::uint8_t {
    General=0,
    Food,
    Materials,
    Toolmaking,
    Metallurgy,
    Logistics
};

inline const char* settlementProductionKindName(
    SettlementProductionKind kind)
{
    switch(kind){
        case SettlementProductionKind::Food: return "Food";
        case SettlementProductionKind::Materials: return "Materials";
        case SettlementProductionKind::Toolmaking: return "Toolmaking";
        case SettlementProductionKind::Metallurgy: return "Metallurgy";
        case SettlementProductionKind::Logistics: return "Logistics";
        case SettlementProductionKind::General:
        default: return "General";
    }
}

struct SettlementProductionProfile {
    SettlementClusterId settlementId=0;
    GridPos anchor{};
    int residentCount=0;
    SettlementProductionKind dominantKind=
        SettlementProductionKind::General;
    double specialization01=0.0;
    bool specialized=false;

    double food01=0.0;
    double materials01=0.0;
    double toolmaking01=0.0;
    double metallurgy01=0.0;
    double logistics01=0.0;

    MaterialKind dominantSurplusMaterial=
        MaterialKind::Unknown;
    int dominantSurplusUnits=0;
    int surplusMaterialCount=0;
};

struct SettlementProductionNetworkObservation {
    int settlementCount=0;
    int inhabitedSettlementCount=0;
    int specializedSettlementCount=0;
    std::vector<SettlementProductionProfile> settlements;
};

inline double clampSettlementProduction01(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

inline int settlementNaturalResourceUnits(
    const World& world,
    GridPos anchor,
    const std::array<MaterialKind,5>& materials)
{
    int total=0;
    for(const ResourceNode& node:world.resourceNodes){
        if(node.quantity<=0
           || manhattan(node.pos,anchor)
                >SettlementServiceRadiusGrid){
            continue;
        }
        if(std::find(
            materials.begin(),materials.end(),
            node.material)!=materials.end()){
            total+=node.quantity;
        }
    }
    return total;
}

inline int settlementNaturalResourceUnits(
    const World& world,
    GridPos anchor,
    MaterialKind material)
{
    int total=0;
    for(const ResourceNode& node:world.resourceNodes){
        if(node.material==material
           && node.quantity>0
           && manhattan(node.pos,anchor)
                <=SettlementServiceRadiusGrid){
            total+=node.quantity;
        }
    }
    return total;
}

inline int settlementOperationalFacilityCount(
    const World& world,
    GridPos anchor,
    FacilityKind kind)
{
    int count=0;
    for(const ConstructedFacility& facility:world.facilities){
        if(facility.kind==kind
           && facilityOperationalAndActive(facility)
           && manhattan(facility.pos,anchor)
                <=SettlementServiceRadiusGrid){
            ++count;
        }
    }
    return count;
}

inline int settlementStoredUnitsNear(
    const World& world,
    GridPos anchor)
{
    int total=0;
    for(const StorageSite& storage:world.storageSites){
        if(manhattan(storage.pos,anchor)
            >SettlementServiceRadiusGrid){
            continue;
        }
        for(const ItemStack& stack:storage.inventory.stacks()){
            total+=std::max(0,stack.quantity);
        }
    }
    return total;
}

inline double settlementTechnologyStrength(
    const SettlementKnowledgeProfile* knowledge,
    TechnologyId technology)
{
    if(knowledge==nullptr) return 0.0;
    const SettlementTechniqueProfile* technique=
        settlementTechniqueById(*knowledge,technology);
    return technique==nullptr
        ? 0.0
        : technique->strength01;
}

inline double settlementMaxTechnologyStrength(
    const SettlementKnowledgeProfile* knowledge,
    std::initializer_list<TechnologyId> technologies)
{
    double result=0.0;
    for(const TechnologyId technology:technologies){
        result=std::max(
            result,
            settlementTechnologyStrength(
                knowledge,technology));
    }
    return result;
}

inline const SettlementProductionProfile* settlementProductionById(
    const SettlementProductionNetworkObservation& observation,
    SettlementClusterId settlementId)
{
    for(const SettlementProductionProfile& settlement:
        observation.settlements){
        if(settlement.settlementId==settlementId) return &settlement;
    }
    return nullptr;
}

inline const SettlementProductionProfile* settlementProductionAtPosition(
    const SettlementProductionNetworkObservation& observation,
    GridPos position)
{
    const SettlementProductionProfile* best=nullptr;
    int bestDistance=SettlementServiceRadiusGrid+1;
    for(const SettlementProductionProfile& settlement:
        observation.settlements){
        const int distance=
            manhattan(position,settlement.anchor);
        if(distance>SettlementServiceRadiusGrid) continue;
        if(best==nullptr
           || distance<bestDistance
           || (
                distance==bestDistance
                && settlement.settlementId
                    <best->settlementId
           )){
            best=&settlement;
            bestDistance=distance;
        }
    }
    return best;
}

inline SettlementProductionNetworkObservation
observeSettlementProductionNetwork(
    const World& world,
    const SettlementPopulation& population)
{
    SettlementProductionNetworkObservation result;
    const SettlementNetworkObservation network=
        observeSettlementNetwork(world,&population);
    const auto assignments=
        settlementResidentAssignments(network,population);
    const SettlementKnowledgeNetworkObservation knowledgeNetwork=
        observeSettlementKnowledgeNetwork(
            world,population);

    result.settlementCount=network.settlementCount;
    result.settlements.reserve(network.settlements.size());

    constexpr std::array<MaterialKind,5> baseMaterials={{
        MaterialKind::Wood,
        MaterialKind::Fiber,
        MaterialKind::Clay,
        MaterialKind::Stone,
        MaterialKind::Flint
    }};
    constexpr std::array<MaterialKind,12> surplusMaterials={{
        MaterialKind::PlantFood,
        MaterialKind::Wood,
        MaterialKind::Fiber,
        MaterialKind::Clay,
        MaterialKind::Stone,
        MaterialKind::Flint,
        MaterialKind::CopperOre,
        MaterialKind::TinOre,
        MaterialKind::Charcoal,
        MaterialKind::CopperMetal,
        MaterialKind::TinMetal,
        MaterialKind::Bronze
    }};

    for(const SettlementClusterObservation& cluster:
        network.settlements){
        SettlementProductionProfile profile;
        profile.settlementId=cluster.id;
        profile.anchor=cluster.anchor;
        profile.residentCount=cluster.residentCount;

        if(cluster.residentCount<=0){
            result.settlements.push_back(profile);
            continue;
        }
        ++result.inhabitedSettlementCount;

        double farmerRole=0.0;
        double foragerRole=0.0;
        double crafterRole=0.0;
        double metallurgistRole=0.0;
        double storekeeperRole=0.0;
        double gatheringSkill=0.0;
        double craftingSkill=0.0;
        int localResidents=0;

        for(const Character& resident:world.characters){
            if(!resident.alive) continue;
            const auto assignment=assignments.find(resident.id);
            if(assignment==assignments.end()
               || assignment->second!=cluster.id){
                continue;
            }
            ++localResidents;
            const ResidentSocietyStatus status=
                observeResidentSocietyStatus(resident);
            if(status.role==SocietyRole::Farmer){
                farmerRole+=status.roleStrength01;
            }else if(status.role==SocietyRole::Forager){
                foragerRole+=status.roleStrength01;
            }else if(status.role==SocietyRole::Craftsperson){
                crafterRole+=status.roleStrength01;
            }else if(status.role==SocietyRole::Metallurgist){
                metallurgistRole+=status.roleStrength01;
            }else if(status.role==SocietyRole::Storekeeper){
                storekeeperRole+=status.roleStrength01;
            }
            gatheringSkill+=resident.civilization.gatheringSkill;
            craftingSkill+=resident.civilization.craftingSkill;
        }

        const double residents=
            static_cast<double>(
                std::max(1,localResidents));
        farmerRole/=residents;
        foragerRole/=residents;
        crafterRole/=residents;
        metallurgistRole/=residents;
        storekeeperRole/=residents;
        gatheringSkill=
            clampSettlementProduction01(
                gatheringSkill/residents);
        craftingSkill=
            clampSettlementProduction01(
                craftingSkill/residents);

        const SettlementKnowledgeProfile* knowledge=
            settlementKnowledgeById(
                knowledgeNetwork,cluster.id);
        const double cultivationKnowledge=
            settlementTechnologyStrength(
                knowledge,TechnologyId::Cultivation);
        const double toolKnowledge=
            settlementMaxTechnologyStrength(
                knowledge,{
                    TechnologyId::SharpFlake,
                    TechnologyId::ChippedStoneTool,
                    TechnologyId::FiberCordage,
                    TechnologyId::DiggingStick,
                    TechnologyId::StoneHammer,
                    TechnologyId::BronzeAxe,
                    TechnologyId::BronzePick
                });
        const double metallurgyKnowledge=
            settlementMaxTechnologyStrength(
                knowledge,{
                    TechnologyId::CopperSmelting,
                    TechnologyId::TinSmelting,
                    TechnologyId::BronzeAlloying
                });
        const double storageKnowledge=
            std::max(
                settlementTechnologyStrength(
                    knowledge,TechnologyId::PrimitiveStorage),
                settlementTechnologyStrength(
                    knowledge,TechnologyId::SimpleContainer));

        const int foodResource=
            settlementNaturalResourceUnits(
                world,cluster.anchor,
                MaterialKind::PlantFood);
        const int materialResource=
            settlementNaturalResourceUnits(
                world,cluster.anchor,
                baseMaterials);
        const int oreResource=
            settlementNaturalResourceUnits(
                world,cluster.anchor,
                MaterialKind::CopperOre)
            +settlementNaturalResourceUnits(
                world,cluster.anchor,
                MaterialKind::TinOre);

        const double localScale=
            static_cast<double>(
                std::max(8,cluster.residentCount*8));
        const double foodResource01=
            clampSettlementProduction01(
                static_cast<double>(foodResource)
                /localScale);
        const double materialResource01=
            clampSettlementProduction01(
                static_cast<double>(materialResource)
                /localScale);
        const double oreResource01=
            clampSettlementProduction01(
                static_cast<double>(oreResource)
                /static_cast<double>(
                    std::max(
                        4,cluster.residentCount*4)));

        const int plots=
            settlementOperationalFacilityCount(
                world,cluster.anchor,
                FacilityKind::CultivatedPlot);
        const int workSurfaces=
            settlementOperationalFacilityCount(
                world,cluster.anchor,
                FacilityKind::WorkSurface);
        const int furnaces=
            settlementOperationalFacilityCount(
                world,cluster.anchor,
                FacilityKind::Furnace);
        const int storageFacilities=
            settlementOperationalFacilityCount(
                world,cluster.anchor,
                FacilityKind::PrimitiveStorage);
        const int storedUnits=
            settlementStoredUnitsNear(
                world,cluster.anchor);

        const double plot01=
            clampSettlementProduction01(
                static_cast<double>(plots)
                /static_cast<double>(
                    std::max(1,cluster.residentCount)));
        const double workSurface01=
            clampSettlementProduction01(
                static_cast<double>(workSurfaces)
                /static_cast<double>(
                    std::max(1,cluster.residentCount)));
        const double furnace01=
            clampSettlementProduction01(
                static_cast<double>(furnaces)
                /static_cast<double>(
                    std::max(1,cluster.residentCount)));
        const double storage01=
            clampSettlementProduction01(
                static_cast<double>(
                    storageFacilities
                    +cluster.storageSiteCount)
                /static_cast<double>(
                    std::max(1,cluster.residentCount)));
        const double storedReserve01=
            clampSettlementProduction01(
                static_cast<double>(storedUnits)
                /static_cast<double>(
                    std::max(
                        1,cluster.residentCount*10)));

        int foodSurplus=0;
        int materialSurplus=0;
        int metalSurplus=0;
        for(const MaterialKind material:
            surplusMaterials){
            const int surplus=
                settlementMaterialSurplusUnits(
                    world,population,network,
                    cluster.id,material);
            if(surplus>0){
                ++profile.surplusMaterialCount;
                if(surplus>profile.dominantSurplusUnits){
                    profile.dominantSurplusUnits=surplus;
                    profile.dominantSurplusMaterial=material;
                }
            }
            if(material==MaterialKind::PlantFood){
                foodSurplus+=surplus;
            }else if(
                material==MaterialKind::Wood
                || material==MaterialKind::Fiber
                || material==MaterialKind::Clay
                || material==MaterialKind::Stone
                || material==MaterialKind::Flint){
                materialSurplus+=surplus;
            }else if(
                material==MaterialKind::CopperMetal
                || material==MaterialKind::TinMetal
                || material==MaterialKind::Bronze){
                metalSurplus+=surplus;
            }
        }

        const double foodSurplus01=
            clampSettlementProduction01(
                static_cast<double>(foodSurplus)
                /static_cast<double>(
                    std::max(
                        1,cluster.residentCount*3)));
        const double materialSurplus01=
            clampSettlementProduction01(
                static_cast<double>(materialSurplus)
                /static_cast<double>(
                    std::max(
                        1,cluster.residentCount*4)));
        const double metalSurplus01=
            clampSettlementProduction01(
                static_cast<double>(metalSurplus)
                /static_cast<double>(
                    std::max(
                        1,cluster.residentCount*2)));
        const double surplusDiversity01=
            clampSettlementProduction01(
                static_cast<double>(
                    profile.surplusMaterialCount)/5.0);

        profile.food01=
            clampSettlementProduction01(
                0.22*foodResource01
                +0.24*cultivationKnowledge
                +0.18*plot01
                +0.20*farmerRole
                +0.16*foodSurplus01);
        profile.materials01=
            clampSettlementProduction01(
                0.30*materialResource01
                +0.24*foragerRole
                +0.20*gatheringSkill
                +0.16*materialSurplus01
                +0.10*surplusDiversity01);
        profile.toolmaking01=
            clampSettlementProduction01(
                0.26*toolKnowledge
                +0.24*crafterRole
                +0.22*craftingSkill
                +0.18*workSurface01
                +0.10*materialSurplus01);
        profile.metallurgy01=
            clampSettlementProduction01(
                0.26*metallurgyKnowledge
                +0.22*metallurgistRole
                +0.18*furnace01
                +0.16*oreResource01
                +0.18*metalSurplus01);
        profile.logistics01=
            clampSettlementProduction01(
                0.26*storageKnowledge
                +0.22*storekeeperRole
                +0.22*storage01
                +0.18*storedReserve01
                +0.12*surplusDiversity01);

        struct Candidate {
            SettlementProductionKind kind;
            double strength;
        };
        const std::array<Candidate,5> candidates={{
            {SettlementProductionKind::Food,profile.food01},
            {SettlementProductionKind::Materials,profile.materials01},
            {SettlementProductionKind::Toolmaking,profile.toolmaking01},
            {SettlementProductionKind::Metallurgy,profile.metallurgy01},
            {SettlementProductionKind::Logistics,profile.logistics01}
        }};

        double best=-1.0;
        double second=0.0;
        for(const Candidate& candidate:candidates){
            if(candidate.strength>best+1e-12){
                second=std::max(0.0,best);
                best=candidate.strength;
                profile.dominantKind=candidate.kind;
            }else if(candidate.strength>second){
                second=candidate.strength;
            }
        }

        const double margin=
            std::max(0.0,best-second);
        profile.specialized=
            best>=0.48 && margin>=0.06;
        profile.specialization01=
            profile.specialized
                ? clampSettlementProduction01(
                    0.72*best+0.28*margin)
                : clampSettlementProduction01(
                    0.42*std::max(0.0,best));
        if(!profile.specialized){
            profile.dominantKind=
                SettlementProductionKind::General;
        }else{
            ++result.specializedSettlementCount;
        }

        result.settlements.push_back(profile);
    }

    std::sort(
        result.settlements.begin(),
        result.settlements.end(),
        [](const SettlementProductionProfile& a,
           const SettlementProductionProfile& b){
            return a.settlementId<b.settlementId;
        });
    return result;
}

inline double settlementProductionMaterialAffinity(
    const SettlementProductionProfile& profile,
    MaterialKind material)
{
    switch(material){
        case MaterialKind::PlantFood:
            return profile.food01;
        case MaterialKind::Wood:
        case MaterialKind::Fiber:
        case MaterialKind::Clay:
        case MaterialKind::Stone:
        case MaterialKind::Flint:
            return profile.materials01;
        case MaterialKind::CopperOre:
        case MaterialKind::TinOre:
        case MaterialKind::Charcoal:
        case MaterialKind::CopperMetal:
        case MaterialKind::TinMetal:
        case MaterialKind::Bronze:
            return profile.metallurgy01;
        default:
            return 0.0;
    }
}

inline double settlementProductionTechniqueAffinity(
    const SettlementProductionProfile& profile,
    TechniqueId technique)
{
    switch(technique){
        case TechniqueId::Cultivation:
            return profile.food01;
        case TechniqueId::SharpFlake:
        case TechniqueId::ChippedStoneTool:
        case TechniqueId::FiberCordage:
        case TechniqueId::DiggingStick:
        case TechniqueId::StoneHammer:
        case TechniqueId::BronzeAxe:
        case TechniqueId::BronzePick:
            return profile.toolmaking01;
        case TechniqueId::CopperSmelting:
        case TechniqueId::TinSmelting:
        case TechniqueId::BronzeAlloying:
            return profile.metallurgy01;
        case TechniqueId::PrimitiveStorage:
        case TechniqueId::SimpleContainer:
            return profile.logistics01;
        default:
            return 0.0;
    }
}

} // namespace lifelens
