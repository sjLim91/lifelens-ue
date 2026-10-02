#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "Relationship.h"
#include "SettlementProduction.h"
#include "SettlementTrade.h"
#include "SocietyEconomy.h"

namespace lifelens {

enum class SettlementGroupRelationState : std::uint8_t {
    Neutral=0,
    Cooperative,
    Strained,
    Hostile
};

inline const char* settlementGroupRelationStateName(
    SettlementGroupRelationState state)
{
    switch(state){
        case SettlementGroupRelationState::Cooperative:
            return "Cooperative";
        case SettlementGroupRelationState::Strained:
            return "Strained";
        case SettlementGroupRelationState::Hostile:
            return "Hostile";
        case SettlementGroupRelationState::Neutral:
        default:
            return "Neutral";
    }
}

struct SettlementGroupRelationObservation {
    SettlementClusterId firstSettlement=0;
    SettlementClusterId secondSettlement=0;
    GridPos firstAnchor{};
    GridPos secondAnchor{};
    int distanceGrid=0;

    int crossResidentPairCount=0;
    int relationshipEvidenceCount=0;
    int exchangeEvidenceCount=0;
    int tradePartnershipCount=0;

    double averageBond01=0.0;
    double averageTrust01=0.0;
    double averageConflict01=0.0;
    double averageFear01=0.0;
    double averageGrudge01=0.0;

    // Complementarity is an opportunity for cooperation, not proof of it.
    double productionComplementarity01=0.0;
    // Shared scarcity raises pressure but cannot create hostility by itself.
    double sharedScarcityPressure01=0.0;

    double cooperation01=0.0;
    double tension01=0.0;
    SettlementGroupRelationState state=
        SettlementGroupRelationState::Neutral;
};

struct SettlementGroupRelationsObservation {
    int settlementCount=0;
    int relationCount=0;
    int cooperativeCount=0;
    int strainedCount=0;
    int hostileCount=0;
    std::vector<SettlementGroupRelationObservation> relations;
};

inline double clampSettlementRelation01(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

inline const SettlementGroupRelationObservation*
settlementGroupRelationBetween(
    const SettlementGroupRelationsObservation& observation,
    SettlementClusterId first,
    SettlementClusterId second)
{
    const SettlementClusterId low=std::min(first,second);
    const SettlementClusterId high=std::max(first,second);
    for(const SettlementGroupRelationObservation& relation:
        observation.relations){
        if(relation.firstSettlement==low
           && relation.secondSettlement==high){
            return &relation;
        }
    }
    return nullptr;
}

inline double settlementDeficitPressure01(
    const World& world,
    const SettlementPopulation& population,
    const SettlementNetworkObservation& network,
    SettlementClusterId settlement,
    MaterialKind material)
{
    const int desired=settlementMaterialDesiredUnits(
        world,population,network,settlement,material);
    if(desired<=0) return 0.0;
    const int deficit=settlementMaterialDeficitUnits(
        world,population,network,settlement,material);
    return clampSettlementRelation01(
        static_cast<double>(deficit)
        /static_cast<double>(desired));
}

inline double settlementPairProductionComplementarity01(
    const World& world,
    const SettlementPopulation& population,
    const SettlementNetworkObservation& network,
    SettlementClusterId first,
    SettlementClusterId second)
{
    int complementaryUnits=0;
    int desiredScale=0;
    for(const MaterialKind material:SocietyExchangeMaterials){
        const int firstSurplus=
            settlementMaterialSurplusUnits(
                world,population,network,first,material);
        const int firstDeficit=
            settlementMaterialDeficitUnits(
                world,population,network,first,material);
        const int secondSurplus=
            settlementMaterialSurplusUnits(
                world,population,network,second,material);
        const int secondDeficit=
            settlementMaterialDeficitUnits(
                world,population,network,second,material);

        complementaryUnits+=
            std::min(firstSurplus,secondDeficit)
            +std::min(secondSurplus,firstDeficit);
        desiredScale+=
            settlementMaterialDesiredUnits(
                world,population,network,first,material)
            +settlementMaterialDesiredUnits(
                world,population,network,second,material);
    }
    return desiredScale>0
        ? clampSettlementRelation01(
            4.0*static_cast<double>(complementaryUnits)
            /static_cast<double>(desiredScale))
        : 0.0;
}

inline double settlementPairSharedScarcityPressure01(
    const World& world,
    const SettlementPopulation& population,
    const SettlementNetworkObservation& network,
    SettlementClusterId first,
    SettlementClusterId second)
{
    double pressure=0.0;
    constexpr std::array<MaterialKind,2> critical={{
        MaterialKind::PlantFood,
        MaterialKind::Water
    }};
    for(const MaterialKind material:critical){
        const double firstPressure=
            settlementDeficitPressure01(
                world,population,network,first,material);
        const double secondPressure=
            settlementDeficitPressure01(
                world,population,network,second,material);
        pressure=std::max(
            pressure,
            std::min(firstPressure,secondPressure));
    }
    return pressure;
}

inline SettlementGroupRelationsObservation
observeSettlementGroupRelations(
    const World& world,
    const RelationshipBook& relationships,
    const SocialKnowledgeBook& knowledge,
    const SettlementNetworkObservation& network,
    const SettlementPopulation& population)
{
    SettlementGroupRelationsObservation result;
    result.settlementCount=network.settlementCount;
    const auto assignments=
        settlementResidentAssignments(
            network,population);

    for(std::size_t i=0;i<network.settlements.size();++i){
        const SettlementClusterObservation& a=
            network.settlements[i];
        if(!a.established || a.residentCount<=0) continue;

        for(std::size_t j=i+1;j<network.settlements.size();++j){
            const SettlementClusterObservation& b=
                network.settlements[j];
            if(!b.established || b.residentCount<=0) continue;

            SettlementGroupRelationObservation relation;
            relation.firstSettlement=std::min(a.id,b.id);
            relation.secondSettlement=std::max(a.id,b.id);
            relation.firstAnchor=
                a.id==relation.firstSettlement
                    ? a.anchor : b.anchor;
            relation.secondAnchor=
                b.id==relation.secondSettlement
                    ? b.anchor : a.anchor;
            relation.distanceGrid=
                manhattan(a.anchor,b.anchor);

            double bondTotal=0.0;
            double trustTotal=0.0;
            double conflictTotal=0.0;
            double fearTotal=0.0;
            double grudgeTotal=0.0;

            for(const Character& first:world.characters){
                if(!first.alive) continue;
                const auto firstAssignment=
                    assignments.find(first.id);
                if(firstAssignment==assignments.end()
                   || firstAssignment->second!=a.id){
                    continue;
                }

                for(const Character& second:world.characters){
                    if(!second.alive || second.id==first.id) continue;
                    const auto secondAssignment=
                        assignments.find(second.id);
                    if(secondAssignment==assignments.end()
                       || secondAssignment->second!=b.id){
                        continue;
                    }

                    ++relation.crossResidentPairCount;
                    if(hasSocietyTradePartnership(
                        knowledge,first.id,second.id)){
                        ++relation.tradePartnershipCount;
                    }

                    const Relationship* firstToSecond=
                        relationships.find(
                            first.id,second.id);
                    const Relationship* secondToFirst=
                        relationships.find(
                            second.id,first.id);
                    const Relationship* directional[2]={
                        firstToSecond,
                        secondToFirst
                    };
                    for(const Relationship* evidence:
                        directional){
                        if(evidence==nullptr) continue;
                        ++relation.relationshipEvidenceCount;
                        bondTotal+=evidence->socialBond();
                        trustTotal+=evidence->trust;
                        conflictTotal+=evidence->conflict;
                        fearTotal+=evidence->fear;
                        grudgeTotal+=evidence->grudge;
                    }
                }
            }

            if(relation.relationshipEvidenceCount>0){
                const double scale=
                    static_cast<double>(
                        relation.relationshipEvidenceCount);
                relation.averageBond01=
                    clampSettlementRelation01(
                        bondTotal/scale);
                relation.averageTrust01=
                    clampSettlementRelation01(
                        trustTotal/scale);
                relation.averageConflict01=
                    clampSettlementRelation01(
                        conflictTotal/scale);
                relation.averageFear01=
                    clampSettlementRelation01(
                        fearTotal/scale);
                relation.averageGrudge01=
                    clampSettlementRelation01(
                        grudgeTotal/scale);
            }

            relation.exchangeEvidenceCount=
                interSettlementTradeFactCount(
                    knowledge,a.id,b.id);
            relation.productionComplementarity01=
                settlementPairProductionComplementarity01(
                    world,population,network,a.id,b.id);
            relation.sharedScarcityPressure01=
                settlementPairSharedScarcityPressure01(
                    world,population,network,a.id,b.id);

            const double exchangeEvidence=
                clampSettlementRelation01(
                    static_cast<double>(
                        relation.exchangeEvidenceCount)/4.0);
            const double partnershipEvidence=
                clampSettlementRelation01(
                    static_cast<double>(
                        relation.tradePartnershipCount)
                    /static_cast<double>(
                        std::max(
                            1,relation.crossResidentPairCount)));

            relation.cooperation01=
                clampSettlementRelation01(
                    0.28*relation.averageBond01
                    +0.22*relation.averageTrust01
                    +0.22*exchangeEvidence
                    +0.12*partnershipEvidence
                    +0.16*relation.productionComplementarity01);

            const double distrust=
                relation.relationshipEvidenceCount>0
                    ? 1.0-relation.averageTrust01
                    : 0.0;
            relation.tension01=
                clampSettlementRelation01(
                    0.36*relation.averageConflict01
                    +0.28*relation.averageGrudge01
                    +0.20*relation.averageFear01
                    +0.10*relation.sharedScarcityPressure01
                    +0.06*distrust);

            const bool negativeRelationshipEvidence=
                relation.relationshipEvidenceCount>0
                && (
                    relation.averageConflict01>=0.20
                    || relation.averageGrudge01>=0.20
                    || relation.averageFear01>=0.20
                );
            const bool positiveContactEvidence=
                relation.exchangeEvidenceCount>0
                || relation.tradePartnershipCount>0
                || (
                    relation.relationshipEvidenceCount>0
                    && (
                        relation.averageBond01>=0.20
                        || relation.averageTrust01>=0.20
                    )
                );

            if(negativeRelationshipEvidence
               && relation.tension01>=0.55
               && relation.tension01
                    >=relation.cooperation01+0.12){
                relation.state=
                    SettlementGroupRelationState::Hostile;
                ++result.hostileCount;
            }else if(
                negativeRelationshipEvidence
                && relation.tension01>=0.32
                && relation.tension01
                    >relation.cooperation01+0.05){
                relation.state=
                    SettlementGroupRelationState::Strained;
                ++result.strainedCount;
            }else if(
                positiveContactEvidence
                && relation.cooperation01>=0.42
                && relation.cooperation01
                    >relation.tension01+0.08){
                relation.state=
                    SettlementGroupRelationState::Cooperative;
                ++result.cooperativeCount;
            }

            result.relations.push_back(relation);
        }
    }

    result.relationCount=
        static_cast<int>(result.relations.size());
    std::sort(
        result.relations.begin(),
        result.relations.end(),
        [](const SettlementGroupRelationObservation& a,
           const SettlementGroupRelationObservation& b){
            if(a.firstSettlement!=b.firstSettlement){
                return a.firstSettlement<b.firstSettlement;
            }
            return a.secondSettlement<b.secondSettlement;
        });
    return result;
}

inline double settlementGroupTradeUtilityAdjustment(
    const SettlementGroupRelationObservation* relation)
{
    if(relation==nullptr) return 0.0;
    const double raw=
        0.08*relation->cooperation01
        -0.10*relation->tension01;
    return std::max(-0.10,std::min(0.08,raw));
}

} // namespace lifelens
