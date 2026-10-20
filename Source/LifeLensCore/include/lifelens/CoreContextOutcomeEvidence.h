#pragma once

#include <cstdint>

#include "ContextAction.h"

namespace lifelens {

// An unsaved observation of an ACTUAL Core execution result, never a
// model proposal, a pending token, or an invented Web action. Subscribers
// only observe evidence; the authoritative executor may separately record
// firsthand experience in the actor's existing saved memory/belief state.
// This event introduces no new binary snapshot fields.
enum class CoreContextOutcomeCode {
    TeachingAdvanced,
    TeachingComprehensionFailed,
    TeachingClarityFailed,
    TeachingRejected,
    ParentingPerformed,
    ParentingRejected,
    TradeExchanged,
    TradeExchangeRejected,
    TradeReturned
};

struct CoreContextOutcomeEvent {
    CharacterId actor=0;
    CharacterId target=0;
    std::uint64_t token=0;
    int minute=-1;
    ContextActionKind kind=ContextActionKind::None;
    CoreContextOutcomeCode code=CoreContextOutcomeCode::TeachingRejected;
};

inline bool coreContextOutcomeActualSuccess(CoreContextOutcomeCode code)
{
    return code==CoreContextOutcomeCode::TeachingAdvanced
        ||code==CoreContextOutcomeCode::ParentingPerformed
        ||code==CoreContextOutcomeCode::TradeExchanged;
}

inline bool coreContextOutcomeActualFailure(CoreContextOutcomeCode code)
{
    switch(code){
        case CoreContextOutcomeCode::TeachingComprehensionFailed:
        case CoreContextOutcomeCode::TeachingClarityFailed:
        case CoreContextOutcomeCode::TeachingRejected:
        case CoreContextOutcomeCode::ParentingRejected:
        case CoreContextOutcomeCode::TradeExchangeRejected: return true;
        default: return false;
    }
}

inline const char* coreContextOutcomeCodeName(CoreContextOutcomeCode code)
{
    switch(code){
        case CoreContextOutcomeCode::TeachingAdvanced: return "teaching-advanced";
        case CoreContextOutcomeCode::TeachingComprehensionFailed: return "comprehension-failed";
        case CoreContextOutcomeCode::TeachingClarityFailed: return "clarity-or-trust-failed";
        case CoreContextOutcomeCode::TeachingRejected: return "teaching-core-rejected";
        case CoreContextOutcomeCode::ParentingPerformed: return "parenting-performed";
        case CoreContextOutcomeCode::ParentingRejected: return "parenting-core-rejected";
        case CoreContextOutcomeCode::TradeExchanged: return "trade-exchanged";
        case CoreContextOutcomeCode::TradeExchangeRejected: return "trade-core-rejected";
        case CoreContextOutcomeCode::TradeReturned: return "trade-returned";
    }
    return "unknown";
}

} // namespace lifelens
