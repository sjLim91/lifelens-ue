#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "AutonomousGoalContract.h"

namespace lifelens {

// COG-B4c-3: read-only comparison of verified WORLD outcomes for an arbitrary
// conjunctive goal. "Reached since baseline" is a temporal observation,
// NEVER proof that a given action caused the change. The comparison may use
// privileged Core truth, so do not inject these observations into the
// resident's Memory/Belief without separate evidence of personal perception.
enum class AutonomousGoalTruthDeltaStatus {
    InvalidContext,
    Unverifiable,
    Expired,
    AlreadySatisfied,
    ReachedSinceBaseline,
    ProgressButUnmet,
    MixedChanges,
    Regressed,
    Stalled
};

inline const char* autonomousGoalTruthDeltaStatusName(
    AutonomousGoalTruthDeltaStatus status)
{
    switch(status){
        case AutonomousGoalTruthDeltaStatus::InvalidContext: return "invalid-context";
        case AutonomousGoalTruthDeltaStatus::Unverifiable: return "unverifiable";
        case AutonomousGoalTruthDeltaStatus::Expired: return "deadline-expired";
        case AutonomousGoalTruthDeltaStatus::AlreadySatisfied: return "already-satisfied";
        case AutonomousGoalTruthDeltaStatus::ReachedSinceBaseline:
            return "reached-since-baseline-not-attributed";
        case AutonomousGoalTruthDeltaStatus::ProgressButUnmet: return "progress-but-unmet";
        case AutonomousGoalTruthDeltaStatus::MixedChanges: return "mixed-changes";
        case AutonomousGoalTruthDeltaStatus::Regressed: return "regressed";
        case AutonomousGoalTruthDeltaStatus::Stalled: return "stalled";
        default: return "invalid-context";
    }
}

struct AutonomousGoalPredicateTruthDelta {
    AutonomousGoalPredicate predicate{};
    double before=0.0;
    double after=0.0;
    double normalizedGapBefore=0.0;
    double normalizedGapAfter=0.0;
    bool satisfiedBefore=false;
    bool satisfiedAfter=false;
};

struct AutonomousGoalTruthDelta {
    AutonomousGoalTruthDeltaStatus status=
        AutonomousGoalTruthDeltaStatus::InvalidContext;
    CharacterId actor=0;
    int baselineMinute=-1;
    int observedMinute=-1;
    std::vector<AutonomousGoalPredicateTruthDelta> predicates;
    double gapBefore=0.0;
    double gapAfter=0.0;
    bool satisfiedBefore=false;
    bool satisfiedAfter=false;
};

// Fail-closed typed shape checks. The caller is separately responsible for
// the original personal-context validation at goal issue time; checking a
// predicate's shape is NOT authorizing a model to see arbitrary World truth.
inline bool validCoreGoalComparisonPredicate(
    const AutonomousGoalPredicate& p,
    CharacterId actor)
{
    if(!std::isfinite(p.threshold)
       ||(p.comparison!=AutonomousGoalCompare::AtMost
          &&p.comparison!=AutonomousGoalCompare::AtLeast))
        return false;

    switch(p.metric){
        case AutonomousGoalMetric::ActorNeed:
            return autonomousGoalNeedValid(p.need)
                &&p.targetResident==0
                &&p.material==MaterialKind::Unknown
                &&p.threshold>=0.0 &&p.threshold<=1.0;
        case AutonomousGoalMetric::ActorRawMaterialCount:
            return p.targetResident==0
                &&static_cast<int>(p.material)>=static_cast<int>(MaterialKind::Stone)
                &&static_cast<int>(p.material)<=static_cast<int>(MaterialKind::Bronze)
                &&p.threshold>=0.0 &&p.threshold<=100000.0
                &&std::floor(p.threshold)==p.threshold;
        case AutonomousGoalMetric::RelationshipTrust:
            return p.targetResident!=0 &&p.targetResident!=actor
                &&p.material==MaterialKind::Unknown
                &&p.threshold>=0.0 &&p.threshold<=1.0;
        case AutonomousGoalMetric::KnownResidentAlive:
            return p.targetResident!=0 &&p.targetResident!=actor
                &&p.material==MaterialKind::Unknown
                &&p.comparison==AutonomousGoalCompare::AtLeast
                &&p.threshold==1.0;
        default: return false;
    }
}

inline double normalizedCoreGoalGap(
    const AutonomousGoalPredicate& p,
    double observed)
{
    const double raw=p.comparison==AutonomousGoalCompare::AtMost
        ?std::max(0.0,observed-p.threshold)
        :std::max(0.0,p.threshold-observed);
    if(p.metric==AutonomousGoalMetric::ActorRawMaterialCount)
        return raw/std::max(1.0,p.threshold);
    return raw; // Needs, trust and alive are all in [0,1].
}

inline AutonomousGoalTruthDelta compareAutonomousGoalCoreTruth(
    const World& baseline,
    const RelationshipBook& baselineRelationships,
    const World& observed,
    const RelationshipBook& observedRelationships,
    const AutonomousGoalCandidate& goal)
{
    AutonomousGoalTruthDelta result;
    result.actor=goal.actor;
    result.baselineMinute=baseline.minute;
    result.observedMinute=observed.minute;

    // A goal cannot be compared across different universes, a reversed
    // timeline, or a fabricated empty/oversized desired-state conjunction.
    if(goal.actor==0
       ||baseline.seed==0
       ||baseline.seed!=observed.seed
       ||baseline.populationSeed!=observed.populationSeed
       ||baseline.generationVersion!=observed.generationVersion
       ||baseline.minute<0
       ||observed.minute<baseline.minute
       ||goal.createdMinute<0
       ||goal.createdMinute>baseline.minute
       ||goal.expiresMinute<goal.createdMinute
       ||static_cast<std::int64_t>(goal.expiresMinute)-goal.createdMinute
           >MaxAutonomousGoalHorizonMinutes
       ||goal.desiredState.empty()
       ||goal.desiredState.size()>MaxAutonomousGoalPredicates
       ||cognitiveCharacter(baseline,goal.actor)==nullptr
       ||cognitiveCharacter(observed,goal.actor)==nullptr)
        return result;

    result.satisfiedBefore=true;
    result.satisfiedAfter=true;
    bool improved=false;
    bool worsened=false;
    result.predicates.reserve(goal.desiredState.size());
    for(const auto& predicate:goal.desiredState){
        if(!validCoreGoalComparisonPredicate(predicate,goal.actor)){
            result.status=AutonomousGoalTruthDeltaStatus::InvalidContext;
            result.predicates.clear();
            return result;
        }

        if(predicate.metric==AutonomousGoalMetric::RelationshipTrust
           ||predicate.metric==AutonomousGoalMetric::KnownResidentAlive){
            const Character* priorTarget=
                cognitiveCharacter(baseline,predicate.targetResident);
            const Character* currentTarget=
                cognitiveCharacter(observed,predicate.targetResident);
            // A relationship trust objective cannot claim progress once its
            // counterpart is unavailable. A known-alive objective can
            // explicitly measure a death, but not a missing GUID.
            if(priorTarget==nullptr ||currentTarget==nullptr
               ||(predicate.metric==AutonomousGoalMetric::RelationshipTrust
                  &&(!priorTarget->alive ||!currentTarget->alive))){
                result.status=AutonomousGoalTruthDeltaStatus::Unverifiable;
                result.predicates.clear();
                return result;
            }
        }

        const auto initial=autonomousGoalTruthValue(
            baseline,baselineRelationships,goal.actor,predicate);
        const auto current=autonomousGoalTruthValue(
            observed,observedRelationships,goal.actor,predicate);
        if(!initial ||!current ||!std::isfinite(*initial)
           ||!std::isfinite(*current)){
            result.status=AutonomousGoalTruthDeltaStatus::Unverifiable;
            result.predicates.clear();
            return result;
        }

        AutonomousGoalPredicateTruthDelta part;
        part.predicate=predicate;
        part.before=*initial;
        part.after=*current;
        part.normalizedGapBefore=normalizedCoreGoalGap(predicate,*initial);
        part.normalizedGapAfter=normalizedCoreGoalGap(predicate,*current);
        part.satisfiedBefore=part.normalizedGapBefore==0.0;
        part.satisfiedAfter=part.normalizedGapAfter==0.0;
        result.gapBefore+=part.normalizedGapBefore;
        result.gapAfter+=part.normalizedGapAfter;
        result.satisfiedBefore=result.satisfiedBefore &&part.satisfiedBefore;
        result.satisfiedAfter=result.satisfiedAfter &&part.satisfiedAfter;
        if(part.normalizedGapAfter<part.normalizedGapBefore-1e-12)
            improved=true;
        if(part.normalizedGapAfter>part.normalizedGapBefore+1e-12)
            worsened=true;
        result.predicates.push_back(part);
    }

    // A deadline lapse is not a success claim; still report actual gaps.
    if(observed.minute>goal.expiresMinute)
        result.status=AutonomousGoalTruthDeltaStatus::Expired;
    else if(result.satisfiedBefore)
        result.status=AutonomousGoalTruthDeltaStatus::AlreadySatisfied;
    else if(result.satisfiedAfter)
        result.status=AutonomousGoalTruthDeltaStatus::ReachedSinceBaseline;
    else if(improved &&worsened)
        result.status=AutonomousGoalTruthDeltaStatus::MixedChanges;
    else if(improved)
        result.status=AutonomousGoalTruthDeltaStatus::ProgressButUnmet;
    else if(worsened)
        result.status=AutonomousGoalTruthDeltaStatus::Regressed;
    else result.status=AutonomousGoalTruthDeltaStatus::Stalled;

    return result;
}

} // namespace lifelens
