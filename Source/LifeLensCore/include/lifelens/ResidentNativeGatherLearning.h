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

    // Unlike a first encounter, a resident can revisit a source which
    // THEY ACTUALLY HARVESTED from, even when it is no longer in view.
    // Candidate identities come exclusively from strict firsthand, dated
    // Gather receipts on this actor. This is not a World-wide resource scan,
    // hearsay, an inferred proximity hint, or an AI-generated destination.
    const ResourceNodeId nativeSource=selected.resourceNode;
    const ResourceNode* existing=findCivilizationResourceNodeSpatial(
        world,nativeSource);
    if(existing==nullptr ||existing->material!=selected.material
       ||existing->quantity<=0) return result;
    GridPos nativeAccess{};
    if(!resolveCivilizationResourceAccessGridPosition(
           world,nativeSource,nativeAccess)) return result;
    const auto nativeDistance=perceptionGridDistance(
        actualPosition,nativeAccess);
    // A remembered source must not quietly lengthen a fatigued resident's
    // real physical trip. Compare actual Core routes (not only straight-line
    // cell distance) against the already-authorized native destination.
    // A well-rested resident may spend at most ONE extra movement step to
    // revisit a personally proven source. As fatigue accumulates, allow
    // only an equal-or-shorter real route. Never refuse a reachable learned
    // alternative solely because the original native route was impassable.
    std::vector<GridPos> nativeRoute;
    const bool nativeReachable=buildCoreGroundRoute(
        world,actualPosition,nativeAccess,1,nativeRoute);
    const std::size_t maxExtraTravelSteps=
        actor.needs.sleep<0.45 ? 1U : 0U;
    const std::string key=std::string("Civilization:Gather:")
        +materialName(selected.material);
    const auto nativeOutcome=experience.find(nativeSource,key);
    double bestScore=nativeOutcome.utilityAdjustment;
    ResourceNodeId bestSource=nativeSource;
    int bestSuccessMinute=-1;
    // A bound on strictly personal episodic candidates, not global nodes.
    // The indexed receipt order is deterministic and never advances RNG.
    int evaluatedMemories=0;
    constexpr int MaxPersonallyRememberedGatherSources=24;
    for(const auto& entry:experience.entries){
        if(entry.first.first!=key
           ||entry.first.second==nativeSource
           ||!entry.second.hasEvidence
           ||!entry.second.succeeded
           ||entry.second.reason!=ResidentResourceOutcomeReason::Success)
            continue;
        if(++evaluatedMemories>MaxPersonallyRememberedGatherSources)
            break;
        const ResourceNodeId knownSource=entry.first.second;
        const ResourceNode* node=findCivilizationResourceNodeSpatial(
            world,knownSource);
        if(node==nullptr ||node->material!=selected.material
           ||node->quantity<=0)
            continue;
        GridPos access{};
        if(!resolveCivilizationResourceAccessGridPosition(
               world,knownSource,access))
            continue;
        // An already authorized Gather into another terrain chunk may
        // open a new settlement/resource frontier. A remembered source in
        // a DIFFERENT chunk must not silently cancel that regional trip.
        // Personal learning refines a choice WITHIN its original physical
        // destination region; it does not override world expansion.
        if(chunkCoordForGrid(access)!=chunkCoordForGrid(nativeAccess))
            continue;
        const auto distance=perceptionGridDistance(
            actualPosition,access);
        if(distance>nativeDistance+2)
            continue;
        const double score=entry.second.utilityAdjustment
            -0.012*static_cast<double>(
                std::max<std::int64_t>(0,distance-nativeDistance));
        if(score<=bestScore+1e-12) continue;
        std::vector<GridPos> route;
        if(!buildCoreGroundRoute(world,actualPosition,access,1,route))
            continue;
        if(nativeReachable
           &&route.size()>nativeRoute.size()+maxExtraTravelSteps)
            continue;
        bestScore=score;
        bestSource=knownSource;
        bestSuccessMinute=entry.second.minute;
    }
    if(bestSource==nativeSource) return result;
    result={nativeSource,bestSource,bestSuccessMinute};
    selected.resourceNode=bestSource;
    // Neither a prior success nor an old position grants new-world facts.
    // Only the resident's own time-bounded executed Gather receipts are
    // eligible; the Core resolves CURRENT physical stock and route before
    // approving a replacement. No unvisited source is inferred as known.
    // All Core-owned goal, intent, material, quantity and utility
    // fields remain unchanged. Actual target and route are later resolved
    // from the refined live source by the SAME pending-token authority.
    return result;
}

} // namespace lifelens
