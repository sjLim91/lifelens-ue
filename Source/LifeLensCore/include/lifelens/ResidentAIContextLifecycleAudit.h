#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

#include "lifelens/Simulation.h"
#include "lifelens/ResidentAIExistingAffordance.h"

namespace lifelens {

// A read-only, per-minute observer. A disappearance of a pending Core token
// proves only that the token is no longer pending; it does NOT prove execution,
// success, failure, or cognitive causation. No simulation writes or RNG calls.
inline constexpr std::size_t ObservedOpportunityContextCount=3;
inline constexpr const char* ObservedOpportunityContextNames[]={
    "KnowledgeTeaching","Parenting","Trade"
};
inline std::size_t observedOpportunityContextIndex(ContextActionKind kind)
{
    switch(kind){
        case ContextActionKind::KnowledgeTeaching: return 0;
        case ContextActionKind::Parenting: return 1;
        case ContextActionKind::Trade: return 2;
        default: return ObservedOpportunityContextCount;
    }
}

inline CognitiveIntentKind observedOpportunityExpectedIntent(
    ContextActionKind kind)
{
    switch(kind){
        case ContextActionKind::KnowledgeTeaching:
            return CognitiveIntentKind::TeachKnowledge;
        case ContextActionKind::Parenting:
            return CognitiveIntentKind::CareForDependent;
        case ContextActionKind::Trade:
            return CognitiveIntentKind::TradeWithResident;
        default: return CognitiveIntentKind::None;
    }
}

// A temporal actor+target match is evidence of alignment, NOT proof that an
// AI caused the action. An autonomous Core opportunity could coincide with it.
inline bool opportunityTokenHasMatchingAcceptedThought(
    const World& world,
    const std::vector<AcceptedCognitiveDecision>& journal,
    CharacterId actor,
    const PendingContextActionObservation& pending)
{
    if(!pending.active ||pending.token==0 ||pending.actor!=actor
       ||pending.targetResident==0 ||pending.issuedMinute<0)
        return false;
    const CognitiveIntentKind expected=
        observedOpportunityExpectedIntent(pending.kind);
    if(expected==CognitiveIntentKind::None) return false;
    for(auto it=journal.rbegin();it!=journal.rend();++it){
        if(it->actor!=actor ||it->minute>pending.issuedMinute) continue;
        if(pending.issuedMinute-it->minute>
           ResidentAgencyGoalHorizonMinutes)
            return false;
        // Only the latest preceding accepted thought can own this slot.
        return it->intent==expected
            &&it->target==pending.targetResident
            &&it->priority>=0.55
            &&validAcceptedCognitiveDecision(*it,world);
    }
    return false;
}

struct OpportunityContextLifecycleCounts {
    std::array<std::uint64_t,ObservedOpportunityContextCount> observedStarts{};
    std::array<std::uint64_t,ObservedOpportunityContextCount> temporalIntentTargetMatches{};
    std::array<std::uint64_t,ObservedOpportunityContextCount> noLongerPendingUnclassified{};
    std::array<std::uint64_t,ObservedOpportunityContextCount> replacedByAnotherToken{};
};

class OpportunityContextLifecycleObserver {
public:
    void observe(
        CharacterId actor,
        const PendingContextActionObservation& pending,
        const World& world,
        const std::vector<AcceptedCognitiveDecision>& accepted,
        OpportunityContextLifecycleCounts& counts)
    {
        auto prior=live_.find(actor);
        const bool nowActive=pending.active &&pending.token!=0;
        if(prior!=live_.end()
           &&(!nowActive ||prior->second.token!=pending.token)){
            const auto index=observedOpportunityContextIndex(
                prior->second.kind);
            if(index<ObservedOpportunityContextCount){
                ++counts.noLongerPendingUnclassified[index];
                if(nowActive) ++counts.replacedByAnotherToken[index];
            }
            live_.erase(prior);
        }
        if(!nowActive) return;
        if(live_.find(actor)!=live_.end()) return;
        live_[actor]={pending.token,pending.kind};
        const auto index=observedOpportunityContextIndex(pending.kind);
        if(index>=ObservedOpportunityContextCount) return;
        ++counts.observedStarts[index];
        if(opportunityTokenHasMatchingAcceptedThought(
            world,accepted,actor,pending))
            ++counts.temporalIntentTargetMatches[index];
    }

    void observeSimulation(
        const Simulation& sim,
        OpportunityContextLifecycleCounts& counts)
    {
        for(const Character& actor:sim.world().characters){
            observe(actor.id,
                sim.observePendingContextAction(actor.id),
                sim.world(),sim.acceptedCognitiveDecisions(),counts);
        }
    }

private:
    struct Token {
        std::uint64_t token=0;
        ContextActionKind kind=ContextActionKind::None;
    };
    std::map<CharacterId,Token> live_;
};

} // namespace lifelens
