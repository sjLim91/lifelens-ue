#pragma once

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>
#include "ResidentUnifiedAgency.h"
#include "AutonomousGoalComposer.h"
#include "CoreNavigation.h"
#include "ResidentPredatorResponse.h"
#include "ResidentGatherHypothesis.h"

namespace lifelens {

struct ResidentSensedResourceGoalChoice {
    ResidentAgencyChoice agency{};
    std::optional<AutonomousGoalCandidate> personalGoal;
};

// Core-internal continuation of the SAME resident decision slot. The caller
// supplies its authoritative runtime position, never an observer/model point.
// An optional generated goal can only prefer an existing, reachable Gather;
// it is not another executor and never mutates inventory, memory, RNG or time.
inline ResidentSensedResourceGoalChoice chooseResidentSensedResourceGoal(
    const World& world,const Character& actor,GridPos actualPosition,
    const RelationshipBook& relationships,const ResidentAgencyChoice& prior,
    const SettlementPopulation* population,bool urgentNeed,
    const SocialKnowledgeBook* socialKnowledge=nullptr,
    const HouseholdBook* households=nullptr)
{
    ResidentSensedResourceGoalChoice answer;
    answer.agency=prior;
    const auto& baseline=prior.decision;
    if(urgentNeed ||world.minute<0 ||world.minute%15!=0
       ||!std::isfinite(baseline.utility) ||baseline.utility<0.0 ||baseline.utility>1.0
       ||actor.needs.hunger>=DefaultSimulationRuleset.utilityAI.urgentThreshold
       ||actor.needs.thirst>=DefaultSimulationRuleset.utilityAI.urgentThreshold
       ||actor.needs.sleep>=DefaultSimulationRuleset.utilityAI.urgentThreshold
       ||actor.needs.bladder>=DefaultSimulationRuleset.utilityAI.urgentThreshold
       ||actor.needs.hygiene>=DefaultSimulationRuleset.utilityAI.urgentThreshold
       // Optional curiosity must yield before ordinary fatigue accumulates.
       // The urgent Core thresholds remain untouched.
       ||maximumResidentNeed(actor)>=0.55
       ||!actor.alive ||!lifeStageProfile(actor.lifeStage).canWork
       ||cognitiveCharacter(world,actor.id)!=&actor
       ||(prior.authority!=ResidentAgencyAuthority::EmbodiedFallback
          &&prior.authority!=ResidentAgencyAuthority::ResidentCognition))
        return answer;
    // A real live Core predator encounter suspends OPTIONAL material
    // curiosity: the single resident's existing flee/survival lane retains
    // authority. A hare or an unseen/distant animal grants no such warning.
    if(world.wildlifeEnabled
       &&observeResidentPredatorThreat(world,actualPosition).detected)
        return answer;
    // Protect existing social/care/avoidance, bodily work, committed facility
    // projects and all other supported optional actions. Only ordinary
    // gathering/frontier curiosity or an idle slot can be redirected.
    if(baseline.kind==UnifiedDecisionKind::Physical){
        if(baseline.physicalGoal!=Goal::Idle) return answer;
    }else if(baseline.kind!=UnifiedDecisionKind::Civilization
        ||(baseline.civilization.intent!=CivilizationIntent::Gather
           &&baseline.civilization.intent!=CivilizationIntent::Explore)){
        return answer;
    }

    // Cognition retains ownership of its actual strategic intent. Only a
    // failed Gather route may resolve to another Core-reachable source of
    // that SAME material. Never run optional embodied curiosity over a model
    // selected Explore or other strategy.
    if(prior.authority==ResidentAgencyAuthority::ResidentCognition
       &&(baseline.kind!=UnifiedDecisionKind::Civilization
          ||baseline.civilization.intent!=CivilizationIntent::Gather))
        return answer;

    auto request=buildCognitiveRequest(
        actor,relationships,world.minute,CognitiveTriggerKind::Reflection,
        cognitiveStrategicIntentCatalog(),
        DefaultCognitiveMemoryContextLimit,DefaultCognitiveBeliefContextLimit,
        DefaultCognitiveRelationshipContextLimit);
    const auto sensed=observeLocalResourceProximity(world,actor.id,actualPosition);
    if(sensed.actor!=actor.id ||sensed.nearbyResources.empty()) return answer;
    request.localProximityObserved=true;
    request.localObserverPosition=sensed.observerPosition;
    request.localRadiusCells=sensed.radiusCells;
    request.localResources=sensed.nearbyResources;
    request.worldSeed=world.seed;
    request.populationSeed=world.populationSeed;
    request.generationVersion=world.generationVersion;

    const auto goals=composeAutonomousGoalCandidates(
        world,request,MaxComposedAutonomousGoals);
    const auto sourceOutcomes=buildResidentResourceOutcomeIndex(actor,world.minute);

    // AL-1: the Core may have proved that this resident COULD NOT REACH a
    // resource selected by the ordinary, nonurgent Gather slot. The existing
    // personal-goal branch previously handled remembered source losses only
    // when a separate curiosity goal happened to outrank this fallback.
    // Preserve the same material goal, but retry another CURRENTLY SENSED
    // live source after firsthand navigation failure. This does not grant a
    // new Gather: the chosen context still travels and executes in Core.
    if(baseline.kind==UnifiedDecisionKind::Civilization
       &&baseline.civilization.intent==CivilizationIntent::Gather
       &&baseline.civilization.resourceNode!=0
       &&baseline.civilization.material!=MaterialKind::Unknown){
        const auto material=baseline.civilization.material;
        const std::string actionKey=std::string("Civilization:Gather:")
            +materialName(material);
        const auto failed=sourceOutcomes.find(
            baseline.civilization.resourceNode,actionKey);
        if(failed.hasEvidence &&!failed.succeeded
           &&failed.reason==ResidentResourceOutcomeReason::AccessUnavailable){
            std::vector<ResourceNodeId> allowed;
            allowed.reserve(sensed.nearbyResources.size());
            for(const auto& source:sensed.nearbyResources)
                if(source.material==material &&source.nodeId!=0)
                    allowed.push_back(source.nodeId);
            std::vector<ResourceNodeId> excluded{
                baseline.civilization.resourceNode};
            for(std::size_t n=0;n<allowed.size();++n){
                const auto alternative=bestGatherDecisionAtPosition(
                    world,actor,actualPosition,population,material,
                    &excluded,&sourceOutcomes,&allowed);
                if(alternative.intent!=CivilizationIntent::Gather
                   ||alternative.resourceNode==0) break;
                const auto past=sourceOutcomes.find(
                    alternative.resourceNode,actionKey);
                GridPos access{};
                std::vector<GridPos> route;
                const bool reachable=
                    resolveCivilizationResourceAccessGridPosition(
                        world,alternative.resourceNode,access)
                    &&buildCoreGroundRoute(
                        world,actualPosition,access,1,route);
                if((past.hasEvidence &&!past.succeeded
                    &&past.reason==ResidentResourceOutcomeReason::AccessUnavailable)
                   ||!reachable){
                    excluded.push_back(alternative.resourceNode);
                    continue;
                }
                if(!std::isfinite(alternative.utility)
                   ||alternative.utility<0.14) break;
                answer.agency.decision.civilization=alternative;
                answer.agency.decision.utility=alternative.utility;
                if(prior.authority==ResidentAgencyAuthority::EmbodiedFallback){
                    answer.agency.authority=ResidentAgencyAuthority::PersonalGoal;
                    answer.agency.contextFingerprint=cognitiveRequestFingerprint(
                        request);
                }
                // Existing accepted cognition stays the sole strategic
                // owner; the source substitution is merely Core execution
                // recovery, not a new or competing mind/fingerprint.
                return answer;
            }
        }
    }
    if(prior.authority==ResidentAgencyAuthority::ResidentCognition)
        return answer;
    // Compare candidates on the SAME existing Core score scale. Raw Gather
    // scores are not comparable to the disposition/role/coordination-biased
    // score already held by the single decision slot.
    const auto craft=applyTechnologyAdoptionUtility(world,actor,
        bestCraftDecisionAtPosition(world,actor,actualPosition,population));
    const bool settlementAuthorityActive=craft.intent==CivilizationIntent::Craft
        &&craft.facilityAction!=FacilityBuildAction::None;
    const auto nativeBias=[&](CivilizationUtilityDecision candidate){
        candidate=applyCivilizationDispositionBias(actor,candidate);
        if(settlementAuthorityActive) return candidate;
        candidate=applySocietyRoleAndDemandUtility(world,actor,candidate);
        return applySocietyCoordinationBias(
            world,actor,candidate,socialKnowledge,households);
    };
    double bestRank=baseline.utility;
    for(const auto& goal:goals){
        if(goal.desiredState.size()!=1
           ||goal.desiredState.front().metric
                !=AutonomousGoalMetric::ActorRawMaterialCount
           ||goal.desiredState.front().comparison
                !=AutonomousGoalCompare::AtLeast
           ||!validateAutonomousGoalCandidate(world,request,goal).accepted)
            continue;
        const auto satisfied=autonomousGoalSatisfiedByTruth(world,relationships,goal);
        if(!satisfied ||*satisfied) continue;
        const auto material=goal.desiredState.front().material;
        // Both filters are bounded by the <=12 actual observations. Never
        // put every distant World node in an exclusion list: a linear search
        // of that list for every World node would turn this into O(N^2).
        std::vector<ResourceNodeId> allowed;
        for(const auto& fact:sensed.nearbyResources)
            if(fact.material==material) allowed.push_back(fact.nodeId);
        std::vector<ResourceNodeId> excluded;
        // Exclude blocked visible nodes and retry only within this resident's
        // bounded current observations. Never fall through to a distant node.
        for(std::size_t attempt=0;attempt<sensed.nearbyResources.size();++attempt){
            const auto candidate=bestGatherDecisionAtPosition(
                world,actor,actualPosition,population,material,&excluded,&sourceOutcomes,&allowed);
            if(candidate.intent!=CivilizationIntent::Gather) break;
            // A directly witnessed source-exhausted/access-denied failure is
            // stronger evidence than an optional curiosity preference. Skip
            // that exact source for this goal window, but NEVER suppress the
            // native Core survival Gather/Explore fallback. A later verified
            // success or the bounded 1-day memory expiry restores eligibility.
            const auto history=sourceOutcomes.find(
                candidate.resourceNode,
                std::string("Civilization:Gather:")+materialName(candidate.material));
            // Recent first-hand depletion is a hypothesis, not permanent
            // world truth. A CURRENT, same-resident, local Core observation of
            // stock at this exact node refutes "still depleted" and may reopen
            // that OPTIONAL goal; access failures remain blocked until a real
            // successful retry or the existing memory horizon expires.
            const auto hypothesis=describeResidentGatherHypothesis(
                sourceOutcomes,actor.id,world.minute,candidate.resourceNode,
                std::string("Civilization:Gather:")+materialName(candidate.material));
            const bool replenished=history.reason
                ==ResidentResourceOutcomeReason::SourceUnavailable
                &&reviewResidentGatherHypothesisFromCore(
                    world,actor,actualPosition,hypothesis)
                    ==ResidentGatherHypothesisRecheck::FreshCoreStockContradictsDepletion;
            if(history.hasEvidence &&!history.succeeded
               &&(history.reason==ResidentResourceOutcomeReason::AccessUnavailable
                  ||(history.reason==ResidentResourceOutcomeReason::SourceUnavailable
                     &&!replenished))){
                excluded.push_back(candidate.resourceNode);
                continue;
            }
            GridPos access{};
            std::vector<GridPos> route;
            if(!resolveCivilizationResourceAccessGridPosition(
                    world,candidate.resourceNode,access)
               ||!buildCoreGroundRoute(world,actualPosition,access,1,route)){
                excluded.push_back(candidate.resourceNode);
                continue;
            }
            // This is a small personal-goal ranking preference, NOT a
            // successful harvest probability or a fabricated amount/result.
            const auto executable=nativeBias(candidate);
            if(executable.utility<0.14) break; // ordinary Core optional-work floor
            // Keep the existing exploration response to actual scarcity:
            // a curiosity bonus may break ties among ordinary gathering,
            // but must not displace a stronger native Explore decision.
            const bool scarcityExploration=
                baseline.kind==UnifiedDecisionKind::Civilization
                &&baseline.civilization.intent==CivilizationIntent::Explore;
            const double rank=executable.utility
                +(scarcityExploration ? 0.0 : 0.12*goal.priority);
            if(rank>bestRank+1e-12){
                bestRank=rank;
                answer.agency.decision.kind=UnifiedDecisionKind::Civilization;
                answer.agency.decision.civilization=executable;
                answer.agency.decision.utility=executable.utility;
                answer.agency.authority=ResidentAgencyAuthority::PersonalGoal;
                answer.agency.contextFingerprint=cognitiveRequestFingerprint(request);
                answer.personalGoal=goal;
            }
            break;
        }
    }
    return answer;
}

} // namespace lifelens
