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

// Read-only relationship between an *observed* Core pending token, the
// latest accepted actor/intent/target at issuance, and the actual terminal
// Core outcome callback. A temporal match is NOT proof of model causation;
// missing evidence stays explicitly unknown instead of claiming completion.
struct OpportunityAcceptedOutcomeCounts {
    std::array<std::uint64_t,ObservedOpportunityContextCount> matchedSuccesses{};
    std::array<std::uint64_t,ObservedOpportunityContextCount> matchedFailures{};
    std::array<std::uint64_t,ObservedOpportunityContextCount> observedWithoutMatch{};
    std::array<std::uint64_t,ObservedOpportunityContextCount> noObservedToken{};
};

class OpportunityAcceptedOutcomeObserver {
public:
    void observePending(
        CharacterId actor,const PendingContextActionObservation& pending,
        const World& world,
        const std::vector<AcceptedCognitiveDecision>& accepted)
    {
        if(!pending.active ||pending.token==0 ||pending.actor!=actor){
            live_.erase(actor);
            return;
        }
        const auto previous=live_.find(actor);
        if(previous!=live_.end() &&previous->second.token==pending.token)
            return;
        live_[actor]={
            pending.token,pending.targetResident,pending.kind,
            opportunityTokenHasMatchingAcceptedThought(
                world,accepted,actor,pending)
        };
    }

    void observeSimulation(const Simulation& sim)
    {
        for(const auto& resident:sim.world().characters)
            observePending(resident.id,
                sim.observePendingContextAction(resident.id),
                sim.world(),sim.acceptedCognitiveDecisions());
    }

    void observeOutcome(const CoreContextOutcomeEvent& event,
                        OpportunityAcceptedOutcomeCounts& counts)
    {
        const auto index=observedOpportunityContextIndex(event.kind);
        if(index>=ObservedOpportunityContextCount ||event.actor==0
           ||event.token==0 ||event.minute<0
           ||(!coreContextOutcomeActualSuccess(event.code)
               &&!coreContextOutcomeActualFailure(event.code)))
            return; // trade return is NOT a second exchange
        const auto it=live_.find(event.actor);
        if(it==live_.end() ||it->second.token!=event.token
           ||it->second.kind!=event.kind
           ||it->second.target!=event.target){
            ++counts.noObservedToken[index];
            return;
        }
        if(it->second.matched){
            if(coreContextOutcomeActualSuccess(event.code))
                ++counts.matchedSuccesses[index];
            else ++counts.matchedFailures[index];
        }else{
            ++counts.observedWithoutMatch[index];
        }
        live_.erase(it); // One observed terminal outcome per token
    }
private:
    struct Live {
        std::uint64_t token=0;
        CharacterId target=0;
        ContextActionKind kind=ContextActionKind::None;
        bool matched=false;
    };
    std::map<CharacterId,Live> live_;
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
