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
inline constexpr std::uint32_t ResidentCognitiveActionLinkExtensionVersion=1;

inline bool validAttachedResidentCognitiveActionLink(
    CharacterId actor,
    const PendingContextAction& pending,
    const std::vector<AcceptedCognitiveDecision>& journal,
    int currentMinute)
{
    const auto& link=pending.cognitiveGoal;
    if(!link.active()) return link.token==0;
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

template<typename W>
void writeResidentCognitiveActionLinkExtension(
    W& w,const SimulationStateSnapshot& snapshot)
{
    w.raw(ResidentCognitiveActionLinkExtensionMagic,
        sizeof(ResidentCognitiveActionLinkExtensionMagic));
    w.u32(ResidentCognitiveActionLinkExtensionVersion);
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
       ||version!=ResidentCognitiveActionLinkExtensionVersion
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
    for(const auto& item:staged)
        snapshot.runtime.at(item.first).pendingContext.cognitiveGoal=item.second;
    return true;
}

} // namespace lifelens
