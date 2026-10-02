#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "CivilizationProgression.h"
#include "SettlementNetwork.h"
#include "SettlementTrade.h"
#include "SocietyEconomy.h"

namespace lifelens {

struct SettlementTechniqueProfile {
    TechnologyId technology=TechnologyId::None;
    TechniqueId technique=TechniqueId::None;
    int awareResidents=0;
    int reproducibleResidents=0;
    int practicedResidents=0;
    int masteredResidents=0;
    int successfulUses=0;
    double averageKnowledge01=0.0;
    double strength01=0.0;
};

struct SettlementKnowledgeProfile {
    SettlementClusterId settlementId=0;
    GridPos anchor{};
    int residentCount=0;
    int awareTechnologyCount=0;
    int reproducibleTechnologyCount=0;
    TechnologyId dominantTechnology=TechnologyId::None;
    double dominantStrength01=0.0;
    double specialization01=0.0;
    bool specialized=false;
    std::vector<SettlementTechniqueProfile> technologies;
};

struct SettlementKnowledgeNetworkObservation {
    int settlementCount=0;
    int inhabitedSettlementCount=0;
    int specializedSettlementCount=0;
    int divergentTechnologyCount=0;
    std::vector<SettlementKnowledgeProfile> settlements;
};

struct SettlementTeachingConnection {
    SettlementClusterId teacherSettlement=0;
    SettlementClusterId learnerSettlement=0;
    bool sameSettlement=false;
    bool localProximity=false;
    bool apprenticeship=false;
    bool tradePartnership=false;
    bool allowed=false;
};

inline double clampSettlementKnowledge01(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

inline const Character* settlementKnowledgeResident(
    const World& world,
    CharacterId id)
{
    for(const Character& resident:world.characters){
        if(resident.id==id) return &resident;
    }
    return nullptr;
}

inline int settlementTechniqueSuccessfulUses(
    const Character& resident,
    TechniqueId technique)
{
    for(const TechniqueKnowledge& record:
        resident.civilization.knowledge.all()){
        if(record.technique==technique){
            return std::max(0,record.successfulUses);
        }
    }
    return 0;
}

inline const SettlementKnowledgeProfile* settlementKnowledgeById(
    const SettlementKnowledgeNetworkObservation& observation,
    SettlementClusterId settlementId)
{
    for(const SettlementKnowledgeProfile& settlement:
        observation.settlements){
        if(settlement.settlementId==settlementId) return &settlement;
    }
    return nullptr;
}

inline const SettlementTechniqueProfile* settlementTechniqueById(
    const SettlementKnowledgeProfile& settlement,
    TechnologyId technology)
{
    for(const SettlementTechniqueProfile& profile:
        settlement.technologies){
        if(profile.technology==technology) return &profile;
    }
    return nullptr;
}

inline SettlementKnowledgeNetworkObservation
observeSettlementKnowledgeNetwork(
    const World& world,
    const SettlementPopulation& population)
{
    SettlementKnowledgeNetworkObservation result;
    const SettlementNetworkObservation network=
        observeSettlementNetwork(world,&population);
    const auto assignments=
        settlementResidentAssignments(network,population);

    result.settlementCount=network.settlementCount;
    result.settlements.reserve(network.settlements.size());

    for(const SettlementClusterObservation& cluster:
        network.settlements){
        SettlementKnowledgeProfile settlement;
        settlement.settlementId=cluster.id;
        settlement.anchor=cluster.anchor;
        settlement.residentCount=cluster.residentCount;
        settlement.technologies.reserve(TechnologyRegistry.size());

        double bestStrength=-1.0;
        double secondStrength=0.0;

        for(const TechnologyDefinition& definition:
            TechnologyRegistry){
            SettlementTechniqueProfile technique;
            technique.technology=definition.id;
            technique.technique=definition.legacyTechnique;

            double normalizedKnowledge=0.0;
            for(const Character& resident:world.characters){
                if(!resident.alive) continue;
                const auto assignment=assignments.find(resident.id);
                if(assignment==assignments.end()
                   || assignment->second!=cluster.id){
                    continue;
                }

                const KnowledgeLevel level=
                    resident.civilization.knowledge.level(
                        technique.technique);
                const int levelValue=
                    static_cast<int>(level);
                if(levelValue>=static_cast<int>(
                    KnowledgeLevel::Observed)){
                    ++technique.awareResidents;
                }
                if(levelValue>=static_cast<int>(
                    KnowledgeLevel::Reproducible)){
                    ++technique.reproducibleResidents;
                }
                if(levelValue>=static_cast<int>(
                    KnowledgeLevel::Practiced)){
                    ++technique.practicedResidents;
                }
                if(levelValue>=static_cast<int>(
                    KnowledgeLevel::Mastered)){
                    ++technique.masteredResidents;
                }
                technique.successfulUses+=
                    settlementTechniqueSuccessfulUses(
                        resident,technique.technique);
                normalizedKnowledge+=
                    static_cast<double>(levelValue)/6.0;
            }

            const double residents=
                static_cast<double>(
                    std::max(1,cluster.residentCount));
            technique.averageKnowledge01=
                cluster.residentCount>0
                    ? clampSettlementKnowledge01(
                        normalizedKnowledge/residents)
                    : 0.0;
            const double awareness=
                static_cast<double>(technique.awareResidents)
                /residents;
            const double reproducible=
                static_cast<double>(
                    technique.reproducibleResidents)
                /residents;
            const double practice=
                clampSettlementKnowledge01(
                    static_cast<double>(
                        technique.successfulUses)
                    /static_cast<double>(
                        std::max(
                            1,cluster.residentCount*8)));
            technique.strength01=
                cluster.residentCount>0
                    ? clampSettlementKnowledge01(
                        0.16*awareness
                        +0.30*reproducible
                        +0.34*technique.averageKnowledge01
                        +0.20*practice)
                    : 0.0;

            if(technique.awareResidents>0){
                ++settlement.awareTechnologyCount;
            }
            if(technique.reproducibleResidents>0){
                ++settlement.reproducibleTechnologyCount;
            }

            if(technique.strength01>bestStrength+1e-12){
                secondStrength=std::max(
                    0.0,bestStrength);
                bestStrength=technique.strength01;
                settlement.dominantTechnology=
                    technique.technology;
                settlement.dominantStrength01=
                    technique.strength01;
            }else if(
                technique.strength01>secondStrength){
                secondStrength=technique.strength01;
            }

            settlement.technologies.push_back(technique);
        }

        const double dominanceMargin=
            std::max(
                0.0,
                settlement.dominantStrength01
                -secondStrength);
        settlement.specialized=
            cluster.residentCount>0
            && settlement.dominantStrength01>=0.50
            && dominanceMargin>=0.08;
        settlement.specialization01=
            settlement.specialized
                ? clampSettlementKnowledge01(
                    0.72*settlement.dominantStrength01
                    +0.28*dominanceMargin)
                : clampSettlementKnowledge01(
                    0.45*settlement.dominantStrength01);

        if(cluster.residentCount>0){
            ++result.inhabitedSettlementCount;
        }
        if(settlement.specialized){
            ++result.specializedSettlementCount;
        }
        result.settlements.push_back(
            std::move(settlement));
    }

    if(result.inhabitedSettlementCount>=2){
        for(const TechnologyDefinition& definition:
            TechnologyRegistry){
            double minimum=1.0;
            double maximum=0.0;
            int seen=0;
            for(const SettlementKnowledgeProfile& settlement:
                result.settlements){
                if(settlement.residentCount<=0) continue;
                const SettlementTechniqueProfile* technique=
                    settlementTechniqueById(
                        settlement,definition.id);
                if(technique==nullptr) continue;
                minimum=std::min(
                    minimum,technique->strength01);
                maximum=std::max(
                    maximum,technique->strength01);
                ++seen;
            }
            if(seen>=2
               && maximum>=0.45
               && maximum-minimum>=0.25){
                ++result.divergentTechnologyCount;
            }
        }
    }

    std::sort(
        result.settlements.begin(),
        result.settlements.end(),
        [](const SettlementKnowledgeProfile& a,
           const SettlementKnowledgeProfile& b){
            return a.settlementId<b.settlementId;
        });
    return result;
}

inline SettlementTeachingConnection observeSettlementTeachingConnection(
    const SettlementNetworkObservation& network,
    const SettlementPopulation& population,
    const SocialKnowledgeBook& knowledge,
    CharacterId teacher,
    CharacterId learner)
{
    SettlementTeachingConnection result;
    if(teacher==0 || learner==0 || teacher==learner){
        return result;
    }

    const auto assignments=
        settlementResidentAssignments(network,population);
    const auto teacherAssignment=assignments.find(teacher);
    const auto learnerAssignment=assignments.find(learner);
    if(teacherAssignment!=assignments.end()){
        result.teacherSettlement=
            teacherAssignment->second;
    }
    if(learnerAssignment!=assignments.end()){
        result.learnerSettlement=
            learnerAssignment->second;
    }

    const auto teacherPos=population.find(teacher);
    const auto learnerPos=population.find(learner);
    if(teacherPos!=population.end()
       && learnerPos!=population.end()){
        result.localProximity=
            manhattan(
                teacherPos->second,
                learnerPos->second)
            <=SettlementServiceRadiusGrid;
    }

    result.sameSettlement=
        result.teacherSettlement!=0
        && result.teacherSettlement
            ==result.learnerSettlement;
    result.apprenticeship=
        hasSocietyApprenticeship(
            knowledge,teacher,learner);
    result.tradePartnership=
        hasSocietyTradePartnership(
            knowledge,teacher,learner);

    // Early camps can teach locally before infrastructure exists. Once two
    // residents belong to distinct settlements, long-range teaching requires a
    // real relationship formed through repeated teaching or trade.
    result.allowed=
        result.sameSettlement
        || (
            (result.teacherSettlement==0
             || result.learnerSettlement==0)
            && result.localProximity
        )
        || result.apprenticeship
        || result.tradePartnership;
    return result;
}

inline double settlementTeachingConnectionBonus(
    const SettlementTeachingConnection& connection)
{
    if(!connection.allowed) return 0.0;
    double bonus=0.0;
    if(connection.sameSettlement
       || (
            connection.teacherSettlement==0
            && connection.learnerSettlement==0
            && connection.localProximity
       )){
        bonus+=0.06;
    }
    if(connection.apprenticeship) bonus+=0.08;
    if(connection.tradePartnership) bonus+=0.04;
    return bonus;
}

inline double settlementTechniqueTeachingBoost(
    const SettlementKnowledgeNetworkObservation& observation,
    SettlementClusterId settlementId,
    TechniqueId technique)
{
    if(settlementId==0 || technique==TechniqueId::None){
        return 0.0;
    }
    const SettlementKnowledgeProfile* settlement=
        settlementKnowledgeById(
            observation,settlementId);
    if(settlement==nullptr || !settlement->specialized){
        return 0.0;
    }
    const TechnologyId technology=
        technologyIdForTechnique(technique);
    if(technology==TechnologyId::None
       || technology!=settlement->dominantTechnology){
        return 0.0;
    }
    return 0.08*settlement->specialization01;
}

} // namespace lifelens
