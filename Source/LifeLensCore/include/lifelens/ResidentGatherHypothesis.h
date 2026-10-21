#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "ResidentResourceOutcomePreference.h"
#include "ResidentLocalPerception.h"
#include "CivilizationDecision.h"

namespace lifelens {

// M2-04: resident-specific, read-only possible explanations grounded in
// firsthand Core outcomes. They are neither verified world facts nor
// executable actions. A later real success replaces earlier failure evidence.
enum class ResidentGatherHypothesisCause {
    Unknown,
    SourceDepletedOrRemoved,
    RouteOrAccessBlocked,
    ContainerPrerequisiteMissing,
    AttemptFailedWithoutKnownCause,
    RecoveredByRealSuccess
};

enum class ResidentGatherHypothesisNextEvidence {
    None,
    ObserveSourceStockAgain,
    ObserveActualAccessAgain,
    ObserveContainerAvailability,
    RepeatVerifiedCoreOutcome
};

struct ResidentGatherHypothesis {
    bool available=false;
    CharacterId actor=0;
    std::uint64_t resourceNode=0;
    std::string actionKey;
    int evidenceMinute=-1;
    ResidentGatherHypothesisCause cause=ResidentGatherHypothesisCause::Unknown;
    ResidentGatherHypothesisNextEvidence nextEvidence=
        ResidentGatherHypothesisNextEvidence::None;
    // Bounded preference magnitude is a recency-weighted evidence proxy,
    // NOT a calibrated probability of truth or AI confidence in the world.
    double evidenceStrengthProxy=0.0;
};

inline ResidentGatherHypothesis describeResidentGatherHypothesis(
    const ResidentResourceOutcomeIndex& personal,
    CharacterId actor,int now,std::uint64_t source,
    const std::string& actionKey)
{
    ResidentGatherHypothesis h;
    if(actor==0 ||personal.actor!=actor ||personal.currentMinute!=now
       ||now<0 ||source==0 ||actionKey.rfind("Civilization:Gather:",0)!=0
       ||actionKey.size()<=20) return h;
    const auto observed=personal.find(source,actionKey);
    if(!observed.hasEvidence ||observed.minute<0
       ||observed.minute>now
       ||static_cast<std::int64_t>(now)-observed.minute
          >ResidentResourceOutcomeHorizonMinutes
       ||!std::isfinite(observed.utilityAdjustment)) return h;
    h.available=true;
    h.actor=actor;
    h.resourceNode=source;
    h.actionKey=actionKey;
    h.evidenceMinute=observed.minute;
    h.evidenceStrengthProxy=std::abs(observed.utilityAdjustment);
    switch(observed.reason){
        case ResidentResourceOutcomeReason::SourceUnavailable:
            h.cause=ResidentGatherHypothesisCause::SourceDepletedOrRemoved;
            h.nextEvidence=ResidentGatherHypothesisNextEvidence::ObserveSourceStockAgain;
            break;
        case ResidentResourceOutcomeReason::AccessUnavailable:
            h.cause=ResidentGatherHypothesisCause::RouteOrAccessBlocked;
            h.nextEvidence=ResidentGatherHypothesisNextEvidence::ObserveActualAccessAgain;
            break;
        case ResidentResourceOutcomeReason::ContainerMissing:
            h.cause=ResidentGatherHypothesisCause::ContainerPrerequisiteMissing;
            h.nextEvidence=ResidentGatherHypothesisNextEvidence::ObserveContainerAvailability;
            break;
        case ResidentResourceOutcomeReason::UnspecifiedFailure:
            h.cause=ResidentGatherHypothesisCause::AttemptFailedWithoutKnownCause;
            h.nextEvidence=ResidentGatherHypothesisNextEvidence::RepeatVerifiedCoreOutcome;
            break;
        case ResidentResourceOutcomeReason::Success:
            h.cause=ResidentGatherHypothesisCause::RecoveredByRealSuccess;
            h.nextEvidence=ResidentGatherHypothesisNextEvidence::None;
            break;
        default:
            return {};
    }
    return h;
}


enum class ResidentGatherHypothesisRecheck {
    InvalidContext,
    StillUnconfirmed,
    FreshCoreStockContradictsDepletion
};

// Re-observe a resident's OWN location against actual current Core resources.
// A replenished node refutes the narrow "still depleted" explanation; it
// does not prove the next gather attempt will succeed, access is open, or the
// resident acquired a missing tool. Missing from a bounded scan is UNKNOWN,
// never evidence that the whole world ran out of the material.
inline ResidentGatherHypothesisRecheck reviewResidentGatherHypothesisFromCore(
    const World& world,const Character& actor,GridPos actualPosition,
    const ResidentGatherHypothesis& hypothesis)
{
    if(!hypothesis.available ||!actor.alive ||actor.id==0
       ||hypothesis.actor!=actor.id
       ||cognitiveCharacter(world,actor.id)!=&actor
       ||world.minute<hypothesis.evidenceMinute
       ||world.minute<0 ||hypothesis.resourceNode==0
       ||static_cast<std::int64_t>(world.minute)-hypothesis.evidenceMinute
            >ResidentResourceOutcomeHorizonMinutes)
        return ResidentGatherHypothesisRecheck::InvalidContext;
    if(hypothesis.cause!=ResidentGatherHypothesisCause::SourceDepletedOrRemoved)
        return ResidentGatherHypothesisRecheck::StillUnconfirmed;
    const auto local=observeLocalResourceProximity(
        world,actor.id,actualPosition);
    for(const auto& fact:local.nearbyResources){
        if(fact.nodeId==hypothesis.resourceNode
           &&hypothesis.actionKey
               ==std::string("Civilization:Gather:")+materialName(fact.material))
            return ResidentGatherHypothesisRecheck::FreshCoreStockContradictsDepletion;
    }
    return ResidentGatherHypothesisRecheck::StillUnconfirmed;
}

} // namespace lifelens
