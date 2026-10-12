#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "CognitiveAgent.h"
#include "Civilization.h"
#include "Relationship.h"
#include "World.h"

namespace lifelens {

// B3 SHADOW-ONLY contract: actors may compose NEW desired-state predicates
// without following a fixed civilization/technology progression. This layer
// cannot execute, authorize or persist an action. A later Core gateway must
// reconstruct its CognitiveRequest and validate provenance again.
inline constexpr std::size_t MaxAutonomousGoalPredicates=6;
inline constexpr std::size_t MaxAutonomousGoalEvidence=8;
inline constexpr int MaxAutonomousGoalHorizonMinutes=30*1440;

enum class AutonomousGoalMetric {
    ActorNeed,
    ActorRawMaterialCount,
    RelationshipTrust,
    KnownResidentAlive
};

enum class AutonomousNeedKind {
    Hunger,
    Thirst,
    Sleep,
    Bladder,
    Hygiene
};

enum class AutonomousGoalCompare {
    AtMost,
    AtLeast
};

enum class AutonomousGoalEvidenceKind {
    ActorNeed,
    Remembered,
    Believed,
    Related
};

// These are references into a bounded individual CognitiveRequest, not proof
// that a belief/report is factually true. The current CognitiveMemoryEvidence
// omits provenance (direct/reported/inferred); do not infer it from this type.
struct AutonomousGoalEvidenceRef {
    AutonomousGoalEvidenceKind kind=AutonomousGoalEvidenceKind::ActorNeed;
    std::size_t index=0;
};

struct AutonomousGoalPredicate {
    AutonomousGoalMetric metric=AutonomousGoalMetric::ActorNeed;
    AutonomousNeedKind need=AutonomousNeedKind::Hunger;
    MaterialKind material=MaterialKind::Unknown;
    CharacterId targetResident=0;
    AutonomousGoalCompare comparison=AutonomousGoalCompare::AtMost;
    double threshold=0.0;
};

struct AutonomousGoalCandidate {
    CharacterId actor=0;
    int createdMinute=0;
    int expiresMinute=0;
    double priority=0.0;
    std::vector<AutonomousGoalPredicate> desiredState;
    std::vector<AutonomousGoalEvidenceRef> evidence;
};

struct AutonomousHypothesisCandidate {
    CharacterId actor=0;
    int createdMinute=0;
    double probability=0.0;
    double estimatedCost=0.0;
    std::vector<AutonomousGoalPredicate> predictedChanges;
    std::vector<AutonomousGoalEvidenceRef> evidence;
};

enum class AutonomousGoalIssue {
    None,
    InvalidActor,
    InvalidContext,
    InvalidTime,
    InvalidPriority,
    InvalidShape,
    InvalidEvidence,
    InvalidPredicate
};

struct AutonomousGoalValidation {
    bool accepted=false;
    AutonomousGoalIssue issue=AutonomousGoalIssue::InvalidShape;
};

inline bool autonomousGoalNeedValid(AutonomousNeedKind need)
{
    switch(need){
        case AutonomousNeedKind::Hunger:
        case AutonomousNeedKind::Thirst:
        case AutonomousNeedKind::Sleep:
        case AutonomousNeedKind::Bladder:
        case AutonomousNeedKind::Hygiene: return true;
        default: return false;
    }
}

inline bool autonomousGoalEvidenceValid(
    const CognitiveRequest& request,
    const AutonomousGoalEvidenceRef& ref)
{
    switch(ref.kind){
        case AutonomousGoalEvidenceKind::ActorNeed:
            return ref.index==0;
        case AutonomousGoalEvidenceKind::Remembered:
            return ref.index<request.memories.size()
                && request.memories[ref.index].minute<=request.minute;
        case AutonomousGoalEvidenceKind::Believed:
            return ref.index<request.beliefs.size()
                && request.beliefs[ref.index].lastUpdatedMinute<=request.minute;
        case AutonomousGoalEvidenceKind::Related:
            return ref.index<request.relationships.size();
        default: return false;
    }
}

inline bool autonomousGoalPredicateValid(
    const World& world,
    const CognitiveRequest& request,
    const AutonomousGoalPredicate& predicate)
{
    if(!std::isfinite(predicate.threshold)) return false;
    if(predicate.comparison!=AutonomousGoalCompare::AtMost
       &&predicate.comparison!=AutonomousGoalCompare::AtLeast) return false;

    switch(predicate.metric){
        case AutonomousGoalMetric::ActorNeed:
            return autonomousGoalNeedValid(predicate.need)
                &&predicate.targetResident==0
                &&predicate.material==MaterialKind::Unknown
                &&predicate.threshold>=0.0 &&predicate.threshold<=1.0;
        case AutonomousGoalMetric::ActorRawMaterialCount:
            return predicate.targetResident==0
                &&static_cast<int>(predicate.material)
                    >=static_cast<int>(MaterialKind::Stone)
                &&static_cast<int>(predicate.material)
                    <=static_cast<int>(MaterialKind::Bronze)
                &&predicate.threshold>=0.0
                &&predicate.threshold<=100000.0
                &&std::floor(predicate.threshold)==predicate.threshold;
        case AutonomousGoalMetric::RelationshipTrust:
        case AutonomousGoalMetric::KnownResidentAlive: {
            if(predicate.targetResident==0
               ||predicate.targetResident==request.actor
               ||predicate.material!=MaterialKind::Unknown) return false;
            const Character* target=
                cognitiveCharacter(world,predicate.targetResident);
            if(target==nullptr || !target->alive) return false;
            const bool salient=std::any_of(
                request.relationships.begin(),request.relationships.end(),
                [&](const CognitiveRelationshipEvidence& evidence){
                    return evidence.target==predicate.targetResident;
                });
            if(!salient) return false; // do not leak arbitrary residents
            if(predicate.metric==AutonomousGoalMetric::KnownResidentAlive)
                return predicate.threshold==1.0
                    &&predicate.comparison==AutonomousGoalCompare::AtLeast;
            return predicate.threshold>=0.0 &&predicate.threshold<=1.0;
        }
        default: return false;
    }
}

inline bool autonomousGoalEvidenceSetValid(
    const CognitiveRequest& request,
    const std::vector<AutonomousGoalEvidenceRef>& evidence)
{
    if(evidence.empty() || evidence.size()>MaxAutonomousGoalEvidence)
        return false;
    for(std::size_t i=0;i<evidence.size();++i){
        if(!autonomousGoalEvidenceValid(request,evidence[i])) return false;
        for(std::size_t j=0;j<i;++j){
            if(evidence[j].kind==evidence[i].kind
               &&evidence[j].index==evidence[i].index) return false;
        }
    }
    return true;
}

inline bool autonomousGoalPredicatesValid(
    const World& world,
    const CognitiveRequest& request,
    const std::vector<AutonomousGoalPredicate>& predicates)
{
    if(predicates.empty() ||predicates.size()>MaxAutonomousGoalPredicates)
        return false;
    for(std::size_t i=0;i<predicates.size();++i){
        const auto& p=predicates[i];
        if(!autonomousGoalPredicateValid(world,request,p)) return false;
        for(std::size_t j=0;j<i;++j){
            const auto& previous=predicates[j];
            if(p.metric==previous.metric &&p.need==previous.need
               &&p.material==previous.material
               &&p.targetResident==previous.targetResident
               &&p.comparison==previous.comparison
               &&p.threshold==previous.threshold) return false;
        }
    }
    return true;
}

inline AutonomousGoalValidation validateAutonomousGoalCandidate(
    const World& world,
    const CognitiveRequest& request,
    const AutonomousGoalCandidate& candidate)
{
    const auto reject=[](AutonomousGoalIssue issue){
        return AutonomousGoalValidation{false,issue};
    };
    const Character* actor=cognitiveCharacter(world,request.actor);
    if(candidate.actor==0 ||candidate.actor!=request.actor
       ||actor==nullptr ||!actor->alive
       ||requiresDirectCare(actor->lifeStage))
        return reject(AutonomousGoalIssue::InvalidActor);
    if(request.minute!=world.minute
       ||request.trigger==CognitiveTriggerKind::None)
        return reject(AutonomousGoalIssue::InvalidContext);
    if(candidate.createdMinute!=request.minute
       ||candidate.expiresMinute<candidate.createdMinute
       ||static_cast<std::int64_t>(candidate.expiresMinute)
            -candidate.createdMinute>MaxAutonomousGoalHorizonMinutes)
        return reject(AutonomousGoalIssue::InvalidTime);
    if(!std::isfinite(candidate.priority)
       ||candidate.priority<0.0 ||candidate.priority>1.0)
        return reject(AutonomousGoalIssue::InvalidPriority);
    if(!autonomousGoalEvidenceSetValid(request,candidate.evidence))
        return reject(AutonomousGoalIssue::InvalidEvidence);
    if(!autonomousGoalPredicatesValid(
           world,request,candidate.desiredState))
        return reject(AutonomousGoalIssue::InvalidPredicate);
    return {true,AutonomousGoalIssue::None};
}

inline AutonomousGoalValidation validateAutonomousHypothesisCandidate(
    const World& world,
    const CognitiveRequest& request,
    const AutonomousGoalCandidate& goal,
    const AutonomousHypothesisCandidate& hypothesis)
{
    const auto validated=validateAutonomousGoalCandidate(world,request,goal);
    if(!validated.accepted) return validated;
    if(hypothesis.actor!=goal.actor
       ||hypothesis.createdMinute!=goal.createdMinute)
        return {false,AutonomousGoalIssue::InvalidActor};
    if(!std::isfinite(hypothesis.probability)
       ||hypothesis.probability<0.0 ||hypothesis.probability>1.0
       ||!std::isfinite(hypothesis.estimatedCost)
       ||hypothesis.estimatedCost<0.0
       ||hypothesis.estimatedCost>100000.0)
        return {false,AutonomousGoalIssue::InvalidPriority};
    if(!autonomousGoalEvidenceSetValid(request,hypothesis.evidence))
        return {false,AutonomousGoalIssue::InvalidEvidence};
    if(!autonomousGoalPredicatesValid(
           world,request,hypothesis.predictedChanges))
        return {false,AutonomousGoalIssue::InvalidPredicate};
    return {true,AutonomousGoalIssue::None};
}

// Only the authoritative Core can evaluate whether a typed goal is actually
// satisfied. This is a read-only truth check; it must NOT be passed as an
// omniscient fact into a resident's CognitiveRequest.
inline std::optional<double> autonomousGoalTruthValue(
    const World& world,
    const RelationshipBook& relationships,
    CharacterId actorId,
    const AutonomousGoalPredicate& predicate)
{
    const Character* actor=cognitiveCharacter(world,actorId);
    if(actor==nullptr) return std::nullopt;
    switch(predicate.metric){
        case AutonomousGoalMetric::ActorNeed:
            switch(predicate.need){
                case AutonomousNeedKind::Hunger: return actor->needs.hunger;
                case AutonomousNeedKind::Thirst: return actor->needs.thirst;
                case AutonomousNeedKind::Sleep: return actor->needs.sleep;
                case AutonomousNeedKind::Bladder: return actor->needs.bladder;
                case AutonomousNeedKind::Hygiene: return actor->needs.hygiene;
                default: return std::nullopt;
            }
        case AutonomousGoalMetric::ActorRawMaterialCount:
            return static_cast<double>(actor->civilization.inventory.count(
                ItemKind::RawMaterial,predicate.material));
        case AutonomousGoalMetric::RelationshipTrust: {
            const Relationship* relation=
                relationships.find(actorId,predicate.targetResident);
            if(relation==nullptr) return std::nullopt;
            return relation->trust;
        }
        case AutonomousGoalMetric::KnownResidentAlive: {
            const Character* target=
                cognitiveCharacter(world,predicate.targetResident);
            if(target==nullptr) return std::nullopt;
            return target->alive ? 1.0 : 0.0;
        }
        default: return std::nullopt;
    }
}

inline std::optional<bool> autonomousGoalSatisfiedByTruth(
    const World& world,
    const RelationshipBook& relationships,
    const AutonomousGoalCandidate& goal)
{
    if(goal.actor==0
       ||goal.desiredState.empty()
       ||goal.desiredState.size()>MaxAutonomousGoalPredicates)
        return std::nullopt;
    for(const auto& predicate:goal.desiredState){
        if(!std::isfinite(predicate.threshold)
           ||(predicate.comparison!=AutonomousGoalCompare::AtMost
              &&predicate.comparison!=AutonomousGoalCompare::AtLeast))
            return std::nullopt;
        const auto value=autonomousGoalTruthValue(
            world,relationships,goal.actor,predicate);
        if(!value ||!std::isfinite(*value)) return std::nullopt;
        if(predicate.comparison==AutonomousGoalCompare::AtMost){
            if(*value>predicate.threshold) return false;
        }else if(predicate.comparison==AutonomousGoalCompare::AtLeast){
            if(*value<predicate.threshold) return false;
        }else return std::nullopt;
    }
    return true;
}

} // namespace lifelens
