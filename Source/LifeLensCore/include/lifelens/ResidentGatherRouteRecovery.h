#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ResidentSensedResourceAgency.h"

namespace lifelens {

// Continuation of the SAME authorized Gather after Core has genuinely failed
// to navigate its original resource route. No new cognitive agent or proposed
// thought; the existing material and the resident's own firsthand failure
// determine a bounded local alternative. Core must STILL issue, navigate and
// physically execute this decision; this pure helper owns none of those acts.
//
// This deliberate off-cadence continuation prevents the failure-memory loop
// from depending on the next 15-minute optional planning boundary, which may
// pick another intention before the failed Gather can recover.
inline std::optional<CivilizationUtilityDecision>
chooseResidentGatherRecoveryAfterCoreRouteFailure(
    const World& world,
    const Character& actor,
    GridPos authoritativePosition,
    const CivilizationUtilityDecision& failedGather,
    const SettlementPopulation* population=nullptr)
{
    if(!actor.alive ||actor.id==0
       ||!lifeStageProfile(actor.lifeStage).canWork
       ||failedGather.intent!=CivilizationIntent::Gather
       ||failedGather.resourceNode==0
       ||failedGather.material==MaterialKind::Unknown)
        return std::nullopt;

    // A physical emergency must still get the next normal planning slot
    // instead of turning a failed optional resource errand into an endless
    // low-priority expedition. Provision failure can keep its own material
    // when THAT need is the one currently urgent.
    const auto strongestNeed=maximumResidentNeed(actor);
    if(strongestNeed>=DefaultSimulationRuleset.utilityAI.urgentThreshold){
        const bool sameProvisionIsUrgent=
            (failedGather.material==MaterialKind::PlantFood
                &&actor.needs.hunger>=strongestNeed-1e-12)
            ||(failedGather.material==MaterialKind::Water
                &&actor.needs.thirst>=strongestNeed-1e-12);
        if(!sameProvisionIsUrgent) return std::nullopt;
    }

    const std::string key="Civilization:Gather:"
        +std::string(materialName(failedGather.material));
    const auto firsthand=buildResidentResourceOutcomeIndex(actor,world.minute);
    if(firsthand.actor!=actor.id
       ||firsthand.currentMinute!=world.minute) return std::nullopt;
    const auto failure=firsthand.find(failedGather.resourceNode,key);
    if(!failure.hasEvidence ||failure.succeeded
       ||failure.reason!=ResidentResourceOutcomeReason::AccessUnavailable
       ||failure.minute!=world.minute)
        return std::nullopt;

    const auto sensed=observeLocalResourceProximity(
        world,actor.id,authoritativePosition,ResidentLocalPerceptionMaxRadius);
    if(sensed.actor!=actor.id ||sensed.nearbyResources.empty())
        return std::nullopt;
    std::vector<ResourceNodeId> allowed;
    allowed.reserve(sensed.nearbyResources.size());
    for(const auto& node:sensed.nearbyResources)
        if(node.nodeId!=0 &&node.material==failedGather.material)
            allowed.push_back(node.nodeId);
    if(allowed.empty()) return std::nullopt;

    std::vector<ResourceNodeId> excluded{failedGather.resourceNode};
    for(std::size_t n=0;n<allowed.size();++n){
        const auto choice=bestGatherDecisionAtPosition(
            world,actor,authoritativePosition,population,
            failedGather.material,&excluded,&firsthand,&allowed);
        if(choice.intent!=CivilizationIntent::Gather
           ||choice.resourceNode==0) break;
        const auto ownHistory=firsthand.find(choice.resourceNode,key);
        GridPos access{};
        std::vector<GridPos> route;
        const bool liveRoute=
            resolveCivilizationResourceAccessGridPosition(
                world,choice.resourceNode,access)
            &&buildCoreGroundRoute(
                world,authoritativePosition,access,1,route);
        if(!liveRoute
           ||(ownHistory.hasEvidence &&!ownHistory.succeeded
              &&ownHistory.reason
                   ==ResidentResourceOutcomeReason::AccessUnavailable)){
            excluded.push_back(choice.resourceNode);
            continue;
        }
        if(choice.utility<0.14) return std::nullopt;
        return choice;
    }
    return std::nullopt;
}

} // namespace lifelens
