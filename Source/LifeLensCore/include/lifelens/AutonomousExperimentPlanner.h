#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "AutonomousAffordancePlanner.h"

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
           ||a.goalPredicateIndex!=b.goalPredicateIndex)
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
        std::size_t socialSteps=0;
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
                totalUtility+=std::clamp(utility,0.0,1.0);
                ++socialSteps;
            }else{
                valid=false;
                break;
            }
        }
        if(!valid ||option.effortUnits>budget.maxEffortUnits
           ||option.riskProxy>budget.maxRiskProxy+1e-12)
            continue;
        option.preferenceScore=socialSteps==0
            ?0.0:totalUtility/static_cast<double>(socialSteps);
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

} // namespace lifelens
