#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include "ResidentSensedResourceAgency.h"

namespace lifelens {

// A source refinement for a Gather the ONE resident mind has ALREADY chosen.
// It is not another planner, goal, utility arbitration or executive.
struct ResidentNativeGatherSourceLearning {
    ResourceNodeId previousSource=0;
    ResourceNodeId preferredSource=0;
    int firsthandSuccessMinute=-1;
    bool changed() const {
        return previousSource!=0
            &&preferredSource!=0
            &&previousSource!=preferredSource
            &&firsthandSuccessMinute>=0;
    }
};

inline ResidentNativeGatherSourceLearning
applyFirsthandSuccessToAuthorizedGather(
    const World& world,const Character& actor,GridPos actualPosition,
    CivilizationUtilityDecision& selected)
{
    ResidentNativeGatherSourceLearning result;
    if(!actor.alive
       ||selected.intent!=CivilizationIntent::Gather
       ||selected.resourceNode==0
       ||selected.material==MaterialKind::Unknown)
        return result;
    const auto experience=buildResidentResourceOutcomeIndex(
        actor,world.minute);
    if(experience.entries.empty()) return result;

    const auto nearby=observeLocalResourceProximity(
        world,actor.id,actualPosition);
    const auto baselineFact=std::find_if(
        nearby.nearbyResources.begin(),nearby.nearbyResources.end(),
        [&](const ObservedLocalResource& fact){
            return fact.nodeId==selected.resourceNode
                &&fact.material==selected.material;
        });
    // Never convert a resident's distant/unknown source into a newly
    // invented knowledge of all World resources.
    if(baselineFact==nearby.nearbyResources.end()) return result;
    const ResourceNodeId nativeSource=baselineFact->nodeId;
    const int nativeDistance=baselineFact->distanceCells;
    const std::string key=std::string("Civilization:Gather:")
        +materialName(selected.material);
    const auto nativeOutcome=experience.find(nativeSource,key);
    double bestScore=nativeOutcome.utilityAdjustment;
    ResourceNodeId bestSource=nativeSource;
    int bestSuccessMinute=-1;
    for(const auto& fact:nearby.nearbyResources){
        if(fact.nodeId==nativeSource
           ||fact.material!=selected.material
           ||fact.distanceCells>nativeDistance+2)
            continue;
        const auto learned=experience.find(fact.nodeId,key);
        if(!learned.hasEvidence ||!learned.succeeded
           ||learned.reason!=ResidentResourceOutcomeReason::Success)
            continue;
        const double score=learned.utilityAdjustment
            -0.012*static_cast<double>(
                std::max(0,fact.distanceCells-nativeDistance));
        if(score<=bestScore+1e-12) continue;
        GridPos access{};
        std::vector<GridPos> route;
        if(!resolveCivilizationResourceAccessGridPosition(
                world,fact.nodeId,access)
           ||!buildCoreGroundRoute(
                world,actualPosition,access,1,route))
            continue;
        bestScore=score;
        bestSource=fact.nodeId;
        bestSuccessMinute=learned.minute;
    }
    if(bestSource==nativeSource) return result;
    result={nativeSource,bestSource,bestSuccessMinute};
    selected.resourceNode=bestSource;
    // All Core-owned goal, intent, material, quantity and utility
    // fields remain unchanged. Actual target and route are later resolved
    // from the refined live source by the SAME pending-token authority.
    return result;
}

} // namespace lifelens
