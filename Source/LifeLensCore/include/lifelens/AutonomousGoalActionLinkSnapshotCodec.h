#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <vector>

#include "SimulationSnapshot.h"

namespace lifelens {

// Optional append-only v8 tail. Older snapshots have no links; after a
// restart from such a save the Core NEVER invents an attribution for a
// social action issued before this feature existed.
inline constexpr char AutonomousGoalActionLinkExtensionMagic[]=
    {'L','L','G','A','C','T','0','1'};
inline constexpr std::uint32_t AutonomousGoalActionLinkExtensionVersion=1;

inline bool validAttachedAutonomousGoalLink(
    CharacterId actor,
    const PendingContextAction& pending,
    int currentMinute)
{
    const AutonomousGoalActionLink& link=pending.autonomousGoal;
    if(!link.active()) return true;
    return validAutonomousGoalActionLink(link)
        &&pending.active()
        &&pending.kind==ContextActionKind::Social
        &&link.actor==actor
        &&link.token==pending.token
        &&link.target==pending.social.target
        &&link.intent==pending.social.intent
        &&link.issuedMinute==pending.issuedMinute
        &&link.issuedMinute<=currentMinute;
}

template<typename W>
void writeAutonomousGoalActionLinkExtension(
    W& w,const SimulationStateSnapshot& snapshot)
{
    w.raw(AutonomousGoalActionLinkExtensionMagic,
          sizeof(AutonomousGoalActionLinkExtensionMagic));
    w.u32(AutonomousGoalActionLinkExtensionVersion);
    std::vector<CharacterId> actors;
    for(const auto& entry:snapshot.runtime){
        if(entry.second.pendingContext.autonomousGoal.active())
            actors.push_back(entry.first);
    }
    std::sort(actors.begin(),actors.end());
    w.u32(static_cast<std::uint32_t>(actors.size()));
    for(CharacterId actor:actors){
        const auto& link=snapshot.runtime.at(actor)
            .pendingContext.autonomousGoal;
        w.u64(actor);
        w.u64(link.token);
        w.u64(link.target);
        w.enumeration(link.intent);
        w.i32(link.issuedMinute);
        w.u64(link.contextFingerprint);
        w.real(link.trustThreshold);
        w.real(link.trustAtIssue);
    }
}

template<typename R>
bool readAutonomousGoalActionLinkExtension(
    R& r,SimulationStateSnapshot& snapshot)
{
    char magic[sizeof(AutonomousGoalActionLinkExtensionMagic)]{};
    std::uint32_t version=0,count=0;
    if(!r.raw(magic,sizeof(magic))
       ||!std::equal(
           magic,magic+sizeof(magic),AutonomousGoalActionLinkExtensionMagic)
       ||!r.u32(version)
       ||version!=AutonomousGoalActionLinkExtensionVersion
       ||!r.count(count)
       ||count>snapshot.runtime.size())
        return false;

    std::unordered_set<CharacterId> seen;
    for(std::uint32_t i=0;i<count;++i){
        CharacterId actor=0;
        AutonomousGoalActionLink link;
        if(!r.u64(actor)
           ||actor==0
           ||!seen.insert(actor).second
           ||!r.u64(link.token)
           ||!r.u64(link.target)
           ||!r.enumeration(link.intent)
           ||!r.i32(link.issuedMinute)
           ||!r.u64(link.contextFingerprint)
           ||!r.real(link.trustThreshold)
           ||!r.real(link.trustAtIssue))
            return false;
        link.actor=actor;
        const auto runtime=snapshot.runtime.find(actor);
        if(runtime==snapshot.runtime.end()) return false;
        const PendingContextAction& pending=runtime->second.pendingContext;
        if(!validAutonomousGoalActionLink(link)
           ||pending.autonomousGoal.active()
           ||link.token!=pending.token
           ||link.target!=pending.social.target
           ||link.intent!=pending.social.intent
           ||link.issuedMinute!=pending.issuedMinute
           ||pending.kind!=ContextActionKind::Social
           ||link.issuedMinute>snapshot.world.minute)
            return false;
        runtime->second.pendingContext.autonomousGoal=link;
    }
    return true;
}

} // namespace lifelens
