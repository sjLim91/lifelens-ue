#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

#include "AutonomousAffordancePlanner.h"
#include "AutonomousSocialOutcomeLearning.h"
#include "ResidentResourceOutcomePreference.h"

namespace lifelens {

// B5a is a read-only hypothesis competition over EXISTING, Core-validated
// affordances. This layer never dispatches actions, decrements inventories,
// grants an experiment, changes a Goal or learns from an imagined outcome.
// Time/effort and risk below are conservative POLICY UNITS, not measured
// minutes, actual resource debits, or a calibrated probability of success.
inline constexpr std::size_t MaxAutonomousExperimentAlternatives=8;
inline constexpr std::size_t MaxAutonomousExperimentEffortUnits=12;

struct AutonomousExperimentBudget {
    std::size_t maxSteps=MaxAutonomousAffordanceSteps;
    std::size_t maxEffortUnits=MaxAutonomousExperimentEffortUnits;
    double maxRiskProxy=0.45;
    std::size_t maxAlternatives=MaxAutonomousExperimentAlternatives;
};

enum class AutonomousExperimentStatus {
    InvalidBudget,
    RejectedContext,
    RejectedGoal,
    AlreadySatisfied,
    NoSafeCandidate,
    CandidatesOnly
};

struct AutonomousExperimentAlternative {
    std::vector<AutonomousAffordanceStep> steps;
    std::size_t effortUnits=0;
    double riskProxy=0.0;
    // Existing Core SocialUtility score: preference, NOT success probability.
    double preferenceScore=0.0;
};

struct AutonomousExperimentOptions {
    CharacterId actor=0;
    int minute=-1;
    std::uint64_t contextFingerprint=0;
    AutonomousExperimentStatus status=AutonomousExperimentStatus::RejectedContext;
    std::vector<AutonomousExperimentAlternative> alternatives;
};

inline bool validAutonomousExperimentBudget(
    const AutonomousExperimentBudget& budget)
{
    return budget.maxSteps>0
        &&budget.maxSteps<=MaxAutonomousAffordanceSteps
        &&budget.maxEffortUnits>0
        &&budget.maxEffortUnits<=MaxAutonomousExperimentEffortUnits
        &&budget.maxAlternatives>0
        &&budget.maxAlternatives<=MaxAutonomousExperimentAlternatives
        &&std::isfinite(budget.maxRiskProxy)
        &&budget.maxRiskProxy>=0.0
        &&budget.maxRiskProxy<=1.0;
}

inline std::size_t autonomousExperimentEffort(
    const AutonomousAffordanceStep& step)
{
    // Abstract budget units only. Actual duration is decided by Core movement
    // and execution; reservation here cannot authorize a physical action.
    return step.kind==AutonomousAffordanceKind::ExistingSocialIntent
        &&step.socialIntent==SocialIntent::Repair ? 2u : 1u;
}

inline double autonomousExperimentRiskProxy(
    const AutonomousAffordanceStep& step)
{
    if(step.kind==AutonomousAffordanceKind::ExistingSocialIntent)
        return step.socialIntent==SocialIntent::Repair ? 0.16 : 0.08;
    return 0.02;
}

inline bool identicalAutonomousExperimentSteps(
    const std::vector<AutonomousAffordanceStep>& first,
    const std::vector<AutonomousAffordanceStep>& second)
{
    if(first.size()!=second.size()) return false;
    for(std::size_t i=0;i<first.size();++i){
        const auto& a=first[i];
        const auto& b=second[i];
        if(a.kind!=b.kind ||a.physicalGoal!=b.physicalGoal
           ||a.socialIntent!=b.socialIntent ||a.target!=b.target
           ||a.goalPredicateIndex!=b.goalPredicateIndex
           ||a.material!=b.material ||a.resourceNode!=b.resourceNode)
            return false;
    }
    return true;
}

// This returns competing *possible approaches*, not an experiment result.
// The next B5 stage must revalidate exact Core state and budget for each real
// action token, and only credit changes proven by that token on completion.
inline AutonomousExperimentOptions composeAutonomousExperimentOptions(
    const World& world,
    const RelationshipBook& relationships,
    const CognitiveRequest& request,
    const AutonomousGoalCandidate& goal,
    const AutonomousExperimentBudget& budget={})
{
    AutonomousExperimentOptions result;
    result.actor=request.actor;
    result.minute=world.minute;
    if(!validAutonomousExperimentBudget(budget)){
        result.status=AutonomousExperimentStatus::InvalidBudget;
        return result;
    }

    const auto base=draftAutonomousAffordancePlan(
        world,relationships,request,goal);
    result.contextFingerprint=base.contextFingerprint;
    switch(base.status){
        case AutonomousAffordancePlanStatus::RejectedContext:
            result.status=AutonomousExperimentStatus::RejectedContext;
            return result;
        case AutonomousAffordancePlanStatus::RejectedGoal:
            result.status=AutonomousExperimentStatus::RejectedGoal;
            return result;
        case AutonomousAffordancePlanStatus::AlreadySatisfied:
            result.status=AutonomousExperimentStatus::AlreadySatisfied;
            return result;
        case AutonomousAffordancePlanStatus::Blocked:
        case AutonomousAffordancePlanStatus::Unsupported:
            result.status=AutonomousExperimentStatus::NoSafeCandidate;
            return result;
        case AutonomousAffordancePlanStatus::CandidateOnly: break;
    }
    if(base.proposedSteps.empty()
       ||base.proposedSteps.size()>budget.maxSteps){
        result.status=AutonomousExperimentStatus::NoSafeCandidate;
        return result;
    }
    const Character* actor=cognitiveCharacter(world,request.actor);
    if(actor==nullptr ||!actor->alive){
        result.status=AutonomousExperimentStatus::RejectedContext;
        return result;
    }
    // Only optional social/gather experimentation yields to rising bodily
    // pressure. A plan consisting of real Core self-care steps (e.g. Sleep)
    // remains available. These are read-only candidates: actual emergency
    // work always belongs to the single authoritative Core decision slot.
    constexpr double DiscretionaryExperimentNeedLimit=0.55;
    const double highestNeed=std::max({
        actor->needs.hunger,actor->needs.thirst,actor->needs.sleep,
        actor->needs.bladder,actor->needs.hygiene
    });
    const bool optionalWork=std::any_of(
        base.proposedSteps.begin(),base.proposedSteps.end(),
        [](const AutonomousAffordanceStep& step){
            return step.kind!=AutonomousAffordanceKind::ExistingPhysicalGoal;
        });
    if(optionalWork &&highestNeed>=DiscretionaryExperimentNeedLimit){
        result.status=AutonomousExperimentStatus::NoSafeCandidate;
        return result;
    }

    // Directly observed, same-resident Core Gather successes and failures
    // may inform candidate ranking (never the physical permission/result).
    // Build this ephemeral index ONCE per comparison, not for each source.
    const auto sourceOutcomes=buildResidentResourceOutcomeIndex(
        *actor,world.minute);

    // Branch only on ACTUALLY supported social methods (Approach/Repair).
    // Every other proposed physical action remains unchanged; there is no
    // fictitious construction/gathering/location or invented technology.
    std::vector<std::vector<AutonomousAffordanceStep>> drafts={
        base.proposedSteps};
    for(std::size_t index=0;index<base.proposedSteps.size();++index){
        const auto& step=base.proposedSteps[index];
        if(step.kind!=AutonomousAffordanceKind::ExistingSocialIntent)
            continue;
        const Character* target=cognitiveCharacter(world,step.target);
        if(target==nullptr ||!target->alive ||target->id==actor->id){
            result.status=AutonomousExperimentStatus::NoSafeCandidate;
            return result;
        }
        const double approach=scoreApproachIntent(
            world,*actor,step.target,relationships);
        const double repair=scoreRepairIntent(
            *actor,step.target,relationships);
        if(!std::isfinite(approach)||!std::isfinite(repair)){
            result.status=AutonomousExperimentStatus::NoSafeCandidate;
            return result;
        }
        std::vector<std::vector<AutonomousAffordanceStep>> next;
        for(const auto& option:drafts){
            for(const SocialIntent intent:{
                    SocialIntent::Approach,SocialIntent::Repair}){
                const double utility=intent==SocialIntent::Repair
                    ?repair:approach;
                if(utility<=0.0) continue;
                auto candidate=option;
                candidate[index].socialIntent=intent;
                next.push_back(std::move(candidate));
            }
        }
        drafts=std::move(next);
        if(drafts.empty()) break;
        // At most 6 steps, so at most 64 combinations before budget filtering.
    }

    for(auto& steps:drafts){
        AutonomousExperimentAlternative option;
        option.steps=std::move(steps);
        double totalUtility=0.0;
        std::size_t scoredSteps=0;
        bool valid=true;
        for(const auto& step:option.steps){
            option.effortUnits+=autonomousExperimentEffort(step);
            option.riskProxy+=autonomousExperimentRiskProxy(step);
            if(step.kind==AutonomousAffordanceKind::ExistingPhysicalGoal){
                if(!actionAvailableFor(world,*actor,step.physicalGoal)){
                    valid=false;
                    break;
                }
            }else if(step.kind==AutonomousAffordanceKind::ExistingSocialIntent){
                const Character* target=cognitiveCharacter(world,step.target);
                if(target==nullptr ||!target->alive){
                    valid=false;
                    break;
                }
                const double utility=step.socialIntent==SocialIntent::Repair
                    ?scoreRepairIntent(*actor,step.target,relationships)
                    :step.socialIntent==SocialIntent::Approach
                    ?scoreApproachIntent(world,*actor,step.target,relationships)
                    :0.0;
                if(!std::isfinite(utility)||utility<=0.0){
                    valid=false;
                    break;
                }
                // This ONE resident's personally felt result from a
                // previously completed Core social action may distinguish
                // two otherwise valid ways to reach the same relationship
                // goal. It does not predict success or bypass Core gates.
                const double personalMood=
                    learnedSocialMoodDisposition(
                        *actor,step.target,step.socialIntent);
                const double personalPreference=std::clamp(
                    utility*(1.0+0.25*personalMood),0.0,1.0);
                totalUtility+=personalPreference;
                ++scoredSteps;
            }else if(step.kind
                ==AutonomousAffordanceKind::ExistingCivilizationGather){
                // Candidate-only budget accounting must use THIS thought's
                // typed local evidence. Distance is an effort/risk proxy,
                // never an estimate of actual traversal minutes or a permit.
                const auto sensed=std::find_if(
                    request.localResources.begin(),request.localResources.end(),
                    [&](const ObservedLocalResource& observed){
                        return observed.nodeId==step.resourceNode
                            &&observed.material==step.material
                            &&observed.distanceCells>=0
                            &&observed.distanceCells<=request.localRadiusCells
                            &&perceptionGridDistance(
                                request.localObserverPosition,observed.position)
                                ==observed.distanceCells;
                    });
                if(!request.localProximityObserved
                   ||sensed==request.localResources.end()){
                    valid=false;
                    break;
                }
                option.effortUnits+=static_cast<std::size_t>(
                    (sensed->distanceCells+1)/2);
                option.riskProxy+=0.025*static_cast<double>(
                    sensed->distanceCells);
                // Introducing this typed candidate must NOT make Gather an
                // unvalidated "other" action. Check the exact Core resource
                // ledger every time the read-only alternative is composed.
                const auto live=std::find_if(world.resourceNodes.begin(),
                    world.resourceNodes.end(),[&](const ResourceNode& node){
                        return node.id==step.resourceNode
                            &&node.material==step.material
                            &&node.quantity>0;
                    });
                if(step.resourceNode==0
                   ||step.material==MaterialKind::Unknown
                   ||live==world.resourceNodes.end()){
                    valid=false;
                    break;
                }
                // A firsthand Core outcome at THIS exact source/material
                // changes the resident's personal preference, not world
                // availability or the likelihood of a future harvest.
                totalUtility+=sourceOutcomes.find(
                    step.resourceNode,
                    std::string("Civilization:Gather:")+
                        materialName(step.material)).utilityAdjustment;
                ++scoredSteps;
            }else{
                valid=false;
                break;
            }
        }
        if(!valid ||option.effortUnits>budget.maxEffortUnits
           ||option.riskProxy>budget.maxRiskProxy+1e-12)
            continue;
        option.preferenceScore=scoredSteps==0
            ?0.0:totalUtility/static_cast<double>(scoredSteps);
        bool duplicate=false;
        for(const auto& prior:result.alternatives){
            if(identicalAutonomousExperimentSteps(
                prior.steps,option.steps)){
                duplicate=true;
                break;
            }
        }
        if(!duplicate) result.alternatives.push_back(std::move(option));
    }

    std::stable_sort(result.alternatives.begin(),result.alternatives.end(),
        [](const auto& a,const auto& b){
            if(a.preferenceScore!=b.preferenceScore)
                return a.preferenceScore>b.preferenceScore;
            if(a.effortUnits!=b.effortUnits)
                return a.effortUnits<b.effortUnits;
            return a.riskProxy<b.riskProxy;
        });
    if(result.alternatives.size()>budget.maxAlternatives)
        result.alternatives.resize(budget.maxAlternatives);
    result.status=result.alternatives.empty()
        ?AutonomousExperimentStatus::NoSafeCandidate
        :AutonomousExperimentStatus::CandidatesOnly;
    return result;
}


 // Recompute the exact Core-derived alternative against CURRENT truth before
 // a later dispatcher is even permitted to consider it. This remains a
 // read-only admission check, NOT a commit permission or step executor.
enum class AutonomousExperimentRevalidation {
    InvalidBudget,
    StaleContext,
    GoalSatisfied,
    NoLongerAdmissible,
    CurrentCandidate
};

inline AutonomousExperimentRevalidation revalidateAutonomousExperimentChoice(
    const World& world,
    const RelationshipBook& relationships,
    const CognitiveRequest& currentRequest,
    const AutonomousGoalCandidate& goal,
    const AutonomousExperimentOptions& issued,
    const AutonomousExperimentAlternative& selected,
    const AutonomousExperimentBudget& budget={})
{
    if(!validAutonomousExperimentBudget(budget))
        return AutonomousExperimentRevalidation::InvalidBudget;
    if(issued.actor!=currentRequest.actor
       ||issued.actor==0 ||issued.minute!=world.minute
       ||issued.status!=AutonomousExperimentStatus::CandidatesOnly
       ||issued.contextFingerprint==0
       ||issued.contextFingerprint!=cognitiveRequestFingerprint(currentRequest))
        return AutonomousExperimentRevalidation::StaleContext;
    const auto current=composeAutonomousExperimentOptions(
        world,relationships,currentRequest,goal,budget);
    if(current.status==AutonomousExperimentStatus::AlreadySatisfied)
        return AutonomousExperimentRevalidation::GoalSatisfied;
    if(current.status==AutonomousExperimentStatus::RejectedContext)
        return AutonomousExperimentRevalidation::StaleContext;
    if(current.status!=AutonomousExperimentStatus::CandidatesOnly
       ||current.contextFingerprint!=issued.contextFingerprint)
        return AutonomousExperimentRevalidation::NoLongerAdmissible;
    for(const auto& alternative:current.alternatives){
        if(identicalAutonomousExperimentSteps(
                selected.steps,alternative.steps)
           &&selected.effortUnits==alternative.effortUnits
           &&std::isfinite(selected.riskProxy)
           &&std::isfinite(selected.preferenceScore)
           &&selected.riskProxy==alternative.riskProxy
           &&selected.preferenceScore==alternative.preferenceScore)
            return AutonomousExperimentRevalidation::CurrentCandidate;
    }
    return AutonomousExperimentRevalidation::NoLongerAdmissible;
}

} // namespace lifelens
