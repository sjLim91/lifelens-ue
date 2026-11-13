#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

#include "SimulationSnapshot.h"

namespace lifelens {

// Append-only optional v8 tail after the existing Gather-goal extension.
// Old saves without this marker remain readable and have no claimed origin.
inline constexpr char ResidentCognitiveActionLinkExtensionMagic[]=
    {'L','L','C','O','G','A','C','1'};
inline constexpr std::uint32_t ResidentCognitiveActionLinkExtensionVersion=2;

inline bool validAttachedResidentCognitiveActionLink(
    CharacterId actor,
    const PendingContextAction& pending,
    const std::vector<AcceptedCognitiveDecision>& journal,
    int currentMinute)
{
    const auto& link=pending.cognitiveGoal;
    if(!link.active()) return link.actor==0
        &&link.contextFingerprint==0 &&link.thoughtMinute==-1
        &&link.issuedMinute==-1;
    if(!validResidentCognitiveActionLink(link)
       ||actor!=link.actor
       ||!pending.active()
       ||pending.kind!=ContextActionKind::Civilization
       ||pending.civilization.intent==CivilizationIntent::None
       ||link.token!=pending.token
       ||link.issuedMinute!=pending.issuedMinute
       ||link.issuedMinute>currentMinute)
        return false;
    return std::any_of(journal.begin(),journal.end(),
        [&](const AcceptedCognitiveDecision& thought){
            return thought.actor==link.actor
                &&thought.intent==CognitiveIntentKind::CraftUsefulTools
                &&thought.minute==link.thoughtMinute
                &&thought.contextFingerprint==link.contextFingerprint
                &&thought.minute<=link.issuedMinute;
        });
}

// The suspended identity is the PREEMPTED, expired physical token. It
// cannot itself execute. A later real Core token can inherit its goal only
// after urgent Needs resolve and the same resident's live agency revalidates.
inline bool validAttachedResidentSuspendedCognitiveLink(
    CharacterId actor,
    const ResidentSuspendedGatherGoal& suspended,
    const std::vector<AcceptedCognitiveDecision>& journal,
    int currentMinute)
{
    const auto& link=suspended.cognitiveGoal;
    if(!link.active()) return link.actor==0
        &&link.contextFingerprint==0 &&link.thoughtMinute==-1
        &&link.issuedMinute==-1;
    if(!suspended.active()
       ||!validResidentCognitiveActionLink(link)
       ||link.actor!=actor
       ||link.token!=suspended.goal.token
       ||link.issuedMinute!=suspended.goal.issuedMinute
       ||link.issuedMinute>currentMinute)
        return false;
    return std::any_of(journal.begin(),journal.end(),
        [&](const AcceptedCognitiveDecision& thought){
            return thought.actor==actor
                &&thought.intent==CognitiveIntentKind::CraftUsefulTools
                &&thought.minute==link.thoughtMinute
                &&thought.contextFingerprint==link.contextFingerprint;
        });
}

template<typename W>
void writeResidentCognitiveActionLinkExtension(
    W& w,const SimulationStateSnapshot& snapshot)
{
    w.raw(ResidentCognitiveActionLinkExtensionMagic,
        sizeof(ResidentCognitiveActionLinkExtensionMagic));
    std::vector<CharacterId> suspendedActors;
    for(const auto& entry:snapshot.runtime)
        if(entry.second.suspendedGather.cognitiveGoal.active())
            suspendedActors.push_back(entry.first);
    std::sort(suspendedActors.begin(),suspendedActors.end());
    // No suspended links => preserve exact version-1 binary representation.
    w.u32(suspendedActors.empty()?1:ResidentCognitiveActionLinkExtensionVersion);
    std::vector<CharacterId> actors;
    for(const auto& entry:snapshot.runtime)
        if(entry.second.pendingContext.cognitiveGoal.active())
            actors.push_back(entry.first);
    std::sort(actors.begin(),actors.end());
    w.u32(static_cast<std::uint32_t>(actors.size()));
    for(const CharacterId actor:actors){
        const auto& link=snapshot.runtime.at(actor).pendingContext.cognitiveGoal;
        w.u64(actor);
        w.u64(link.token);
        w.u64(link.contextFingerprint);
        w.i32(link.thoughtMinute);
        w.i32(link.issuedMinute);
    }
    if(!suspendedActors.empty()){
        w.u32(static_cast<std::uint32_t>(suspendedActors.size()));
        for(const CharacterId actor:suspendedActors){
            const auto& link=snapshot.runtime.at(actor)
                .suspendedGather.cognitiveGoal;
            w.u64(actor);
            w.u64(link.token);
            w.u64(link.contextFingerprint);
            w.i32(link.thoughtMinute);
            w.i32(link.issuedMinute);
        }
    }
}

template<typename R>
bool readResidentCognitiveActionLinkExtension(
    R& r,SimulationStateSnapshot& snapshot)
{
    char magic[sizeof(ResidentCognitiveActionLinkExtensionMagic)]{};
    std::uint32_t version=0,count=0;
    if(!r.raw(magic,sizeof(magic))
       ||!std::equal(magic,magic+sizeof(magic),
            ResidentCognitiveActionLinkExtensionMagic)
       ||!r.u32(version)
       ||(version!=1 &&version!=ResidentCognitiveActionLinkExtensionVersion)
       ||!r.count(count)
       ||count>snapshot.runtime.size()) return false;

    std::unordered_set<CharacterId> seen;
    std::vector<std::pair<CharacterId,ResidentCognitiveActionLink>> staged;
    staged.reserve(count);
    for(std::uint32_t i=0;i<count;++i){
        CharacterId actor=0;
        ResidentCognitiveActionLink link;
        if(!r.u64(actor)
           ||actor==0
           ||!seen.insert(actor).second
           ||!r.u64(link.token)
           ||!r.u64(link.contextFingerprint)
           ||!r.i32(link.thoughtMinute)
           ||!r.i32(link.issuedMinute))
            return false;
        link.actor=actor;
        const auto it=snapshot.runtime.find(actor);
        if(it==snapshot.runtime.end()
           ||it->second.pendingContext.cognitiveGoal.active())
            return false;
        const auto& pending=it->second.pendingContext;
        if(!validResidentCognitiveActionLink(link)
           ||!pending.active()
           ||pending.kind!=ContextActionKind::Civilization
           ||pending.civilization.intent==CivilizationIntent::None
           ||pending.token!=link.token
           ||pending.issuedMinute!=link.issuedMinute
           ||link.issuedMinute>snapshot.world.minute
           ||!std::any_of(
                 snapshot.acceptedCognitiveDecisions.begin(),
                 snapshot.acceptedCognitiveDecisions.end(),
                 [&](const AcceptedCognitiveDecision& thought){
                     return thought.actor==actor
                         &&thought.intent==CognitiveIntentKind::CraftUsefulTools
                         &&thought.minute==link.thoughtMinute
                         &&thought.contextFingerprint==link.contextFingerprint;
                 }))
            return false;
        staged.emplace_back(actor,link);
    }
    std::vector<std::pair<CharacterId,ResidentCognitiveActionLink>> suspendedStaged;
    if(version>=2){
        std::uint32_t suspendedCount=0;
        if(!r.count(suspendedCount) ||suspendedCount>snapshot.runtime.size())
            return false;
        std::unordered_set<CharacterId> suspendedSeen;
        suspendedStaged.reserve(suspendedCount);
        for(std::uint32_t i=0;i<suspendedCount;++i){
            CharacterId actor=0;
            ResidentCognitiveActionLink link;
            if(!r.u64(actor) ||actor==0
               ||!suspendedSeen.insert(actor).second
               ||!r.u64(link.token)
               ||!r.u64(link.contextFingerprint)
               ||!r.i32(link.thoughtMinute)
               ||!r.i32(link.issuedMinute)) return false;
            link.actor=actor;
            const auto it=snapshot.runtime.find(actor);
            if(it==snapshot.runtime.end()) return false;
            auto candidate=it->second.suspendedGather;
            if(candidate.cognitiveGoal.active()) return false;
            candidate.cognitiveGoal=link;
            if(!validAttachedResidentSuspendedCognitiveLink(
                   actor,candidate,snapshot.acceptedCognitiveDecisions,
                   snapshot.world.minute))
                return false;
            suspendedStaged.emplace_back(actor,link);
        }
    }
    // Atomic restore: only attach any row after both active and suspended
    // groups have been fully validated against authoritative Core state.
    for(const auto& item:staged)
        snapshot.runtime.at(item.first).pendingContext.cognitiveGoal=item.second;
    for(const auto& item:suspendedStaged)
        snapshot.runtime.at(item.first).suspendedGather.cognitiveGoal=item.second;
    return true;
}

} // namespace lifelens
