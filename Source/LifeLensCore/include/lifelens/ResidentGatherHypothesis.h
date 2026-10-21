#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "ResidentResourceOutcomePreference.h"

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

} // namespace lifelens
