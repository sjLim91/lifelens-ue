#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include "AutonomousGoalContract.h"

namespace lifelens {

// COG-GOAL B3b: deterministic and purely observational candidate generation.
// No shared RNG, World mutations, persistence, external API or executive power.
// Current CognitiveRequest must be supplied by Core, NOT accepted from a model.
// Candidate proposals are distinct from accepted cognition and actual actions.
inline constexpr std::size_t MaxComposedAutonomousGoals=6;

struct WeightedAutonomousGoal {
    AutonomousGoalCandidate goal;
    std::size_t deterministicOrder=0;
};

inline double safeAutonomousPreference(double value)
{
    return std::isfinite(value) ? std::clamp(value,0.0,1.0) : 0.5;
}

inline bool memoryRefersToNeed(
    const CognitiveMemoryEvidence& evidence,
    AutonomousNeedKind kind)
{
    const auto matches=[&](const char* label){
        return std::find(evidence.tags.begin(),evidence.tags.end(),label)
            !=evidence.tags.end();
    };
    switch(kind){
        case AutonomousNeedKind::Hunger:
            return matches("food") || matches("hunger");
        case AutonomousNeedKind::Thirst:
            return matches("water") || matches("thirst");
        case AutonomousNeedKind::Sleep:
            return matches("sleep") || matches("fatigue");
        case AutonomousNeedKind::Bladder:
            return matches("sanitation") || matches("toilet");
        case AutonomousNeedKind::Hygiene:
            return matches("hygiene") || matches("contamination");
        default: return false;
    }
}

inline std::vector<AutonomousGoalCandidate> composeAutonomousGoalCandidates(
    const World& world,
    const CognitiveRequest& request,
    std::size_t limit=4)
{
    std::vector<AutonomousGoalCandidate> result;
    if(limit==0 ||request.actor==0 ||request.minute!=world.minute
       ||request.minute<0
       ||request.minute>std::numeric_limits<int>::max()-1440
       ||request.trigger==CognitiveTriggerKind::None) return result;

    const Character* actor=cognitiveCharacter(world,request.actor);
    if(actor==nullptr ||!actor->alive
       ||requiresDirectCare(actor->lifeStage)) return result;

    limit=std::min(limit,MaxComposedAutonomousGoals);
    std::vector<WeightedAutonomousGoal> weighted;
    weighted.reserve(14);
    const auto makeGoal=[&](double priority){
        AutonomousGoalCandidate goal;
        goal.actor=request.actor;
        goal.createdMinute=request.minute;
        goal.expiresMinute=request.minute+1440;
        goal.priority=std::clamp(priority,0.0,1.0);
        return goal;
    };
    const auto emit=[&](AutonomousGoalCandidate goal){
        // Even locally generated candidates pass the SAME Core validation
        // gate as any optional provider proposal. Invalid candidates vanish.
        if(validateAutonomousGoalCandidate(world,request,goal).accepted)
            weighted.push_back({std::move(goal),weighted.size()});
    };

    const std::array<double,5> needs={
        request.needs.hunger,request.needs.thirst,request.needs.sleep,
        request.needs.bladder,request.needs.hygiene
    };
    std::vector<AutonomousGoalCandidate> needGoals;
    needGoals.reserve(5);

    for(std::size_t i=0;i<needs.size();++i){
        const double pressure=needs[i];
        if(!std::isfinite(pressure) ||pressure<0.28 ||pressure>1.0)
            continue;
        const auto need=static_cast<AutonomousNeedKind>(i);
        double witnessedSalience=0.0;
        std::size_t relevantEvidence=0;
        bool evidenceFound=false;
        for(std::size_t j=0;j<request.memories.size();++j){
            const auto& memory=request.memories[j];
            if(memory.minute>request.minute
               ||!std::isfinite(memory.confidence)
               ||!memoryRefersToNeed(memory,need)) continue;
            const double sourceWeight=
                memory.source==MemorySource::DirectWitness &&memory.witnessed
                ? 0.08
                : memory.source==MemorySource::ToldByOther
                ? 0.015 : 0.0;
            const double salience=sourceWeight
                *std::clamp(memory.confidence,0.0,1.0);
            if(salience>witnessedSalience){
                witnessedSalience=salience;
                relevantEvidence=j;
                evidenceFound=true;
            }
        }
        const double selfCarePriority=std::clamp(
            0.08+0.82*pressure+witnessedSalience,0.0,1.0);
        auto goal=makeGoal(selfCarePriority);
        AutonomousGoalPredicate desired;
        desired.metric=AutonomousGoalMetric::ActorNeed;
        desired.need=need;
        desired.comparison=AutonomousGoalCompare::AtMost;
        desired.threshold=std::max(0.1,pressure-0.20);
        goal.desiredState.push_back(desired);
        goal.evidence.push_back({AutonomousGoalEvidenceKind::ActorNeed,0});
        if(evidenceFound)
            goal.evidence.push_back({
                AutonomousGoalEvidenceKind::Remembered,relevantEvidence
            });
        needGoals.push_back(goal);
        emit(std::move(goal));
    }

    // Personal opportunities arise from this resident's CURRENT bounded
    // Core proximity hints, not an omniscient sweep over all World nodes.
    // These are modest curiosity/material goals, not harvesting commands.
    // LOS, path, free inventory room and actual resource extraction must
    // all be established later by authoritative action execution.
    if(request.localProximityObserved){
        std::vector<MaterialKind> proposedMaterials;
        proposedMaterials.reserve(3);
        for(std::size_t i=0;i<request.localResources.size()
            &&i<ResidentLocalPerceptionMaxFacts
            &&proposedMaterials.size()<3;++i){
            const auto& nearby=request.localResources[i];
            if(nearby.nodeId==0
               ||static_cast<int>(nearby.material)
                   <static_cast<int>(MaterialKind::Stone)
               ||static_cast<int>(nearby.material)
                   >static_cast<int>(MaterialKind::Bronze)
               ||std::find(proposedMaterials.begin(),proposedMaterials.end(),
                            nearby.material)!=proposedMaterials.end())
                continue;
            proposedMaterials.push_back(nearby.material);
            auto goal=makeGoal(std::clamp(
                0.10+0.26*safeAutonomousPreference(
                    request.personality.curiosity)
                +0.03*(1.0-static_cast<double>(
                    nearby.distanceCells)
                    /std::max(1,request.localRadiusCells)),
                0.0,0.45));
            AutonomousGoalPredicate desire;
            desire.metric=AutonomousGoalMetric::ActorRawMaterialCount;
            desire.material=nearby.material;
            desire.comparison=AutonomousGoalCompare::AtLeast;
            desire.threshold=1.0;
            goal.desiredState.push_back(desire);
            goal.evidence.push_back({
                AutonomousGoalEvidenceKind::ObservedLocalResource,i
            });
            emit(std::move(goal));
        }
    }

    std::vector<AutonomousGoalCandidate> relationshipGoals;
    relationshipGoals.reserve(request.relationships.size());
    for(std::size_t i=0;i<request.relationships.size();++i){
        const auto& relation=request.relationships[i];
        if(!std::isfinite(relation.trust) ||relation.trust<0.0
           ||relation.trust>=0.85 ||relation.target==0
           ||relation.target==request.actor) continue;
        // The generator only sees a person's relationship context. It must
        // not query hidden target vitality to change its reasoning; the
        // authoritative validator independently checks target existence.
        const double sociability=safeAutonomousPreference(
            request.personality.sociability);
        const double empathy=safeAutonomousPreference(
            request.personality.empathy);
        const double priority=0.12
            +0.53*sociability*(1.0-relation.trust)
            +0.12*empathy;

        auto goal=makeGoal(priority);
        AutonomousGoalPredicate desired;
        desired.metric=AutonomousGoalMetric::RelationshipTrust;
        desired.targetResident=relation.target;
        desired.comparison=AutonomousGoalCompare::AtLeast;
        desired.threshold=std::min(0.9,relation.trust+0.15);
        goal.desiredState.push_back(desired);
        goal.evidence.push_back({AutonomousGoalEvidenceKind::Related,i});
        relationshipGoals.push_back(goal);
        emit(std::move(goal));
    }

    // Conjunctions create fresh desired-state combinations instead of
    // inventing a fixed "Build Village" or "Unlock Husbandry" sequence.
    // Avoid packaging social aspirations with an urgent survival pressure.
    if(needGoals.size()>=2){
        std::stable_sort(needGoals.begin(),needGoals.end(),
            [](const auto& a,const auto& b){
                return a.priority>b.priority;
            });
        auto combined=needGoals[0];
        combined.desiredState.push_back(needGoals[1].desiredState.front());
        combined.priority=std::clamp(
            0.58*needGoals[0].priority
            +0.42*needGoals[1].priority-0.03,0.0,1.0);
        emit(std::move(combined));
    }
    if(!needGoals.empty() && !relationshipGoals.empty()
       &&needs[static_cast<std::size_t>(
           needGoals.front().desiredState.front().need)]<0.66){
        auto combined=needGoals.front();
        const auto& social=relationshipGoals.front();
        combined.desiredState.push_back(social.desiredState.front());
        combined.evidence.push_back(social.evidence.front());
        combined.priority=std::clamp(
            0.65*combined.priority+0.35*social.priority,0.0,1.0);
        emit(std::move(combined));
    }

    std::stable_sort(weighted.begin(),weighted.end(),
        [](const WeightedAutonomousGoal& a,const WeightedAutonomousGoal& b){
            if(a.goal.priority!=b.goal.priority)
                return a.goal.priority>b.goal.priority;
            return a.deterministicOrder<b.deterministicOrder;
        });

    for(auto& entry:weighted){
        if(result.size()>=limit) break;
        result.push_back(std::move(entry.goal));
    }
    return result;
}

} // namespace lifelens
