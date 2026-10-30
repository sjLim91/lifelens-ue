#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

#include "SimulationSnapshot.h"

namespace lifelens {

// Optional append-only v8 tail. An older in-flight Gather has no recorded
// objective; loading it must not invent one or change its physical command.
inline constexpr char ResidentGatherGoalExtensionMagic[]=
    {'L','L','G','G','O','A','L','1'};
inline constexpr std::uint32_t ResidentGatherGoalExtensionVersion=1;

inline void hashGatherGoalInteger(std::uint64_t& hash,std::uint64_t value)
{
    for(int byte=0;byte<8;++byte){
        hash^=value&0xffU;
        hash*=1099511628211ULL;
        value>>=8;
    }
}

inline void hashGatherGoal(std::uint64_t& hash,const ResidentGatherGoalLink& goal)
{
    for(const auto value:{goal.actor,goal.goalId,goal.token,
        static_cast<std::uint64_t>(goal.material),
        static_cast<std::uint64_t>(goal.createdMinute),
        static_cast<std::uint64_t>(goal.issuedMinute),
        static_cast<std::uint64_t>(goal.expiresMinute),
        static_cast<std::uint64_t>(goal.inventoryAtIssue),
        static_cast<std::uint64_t>(goal.targetQuantity),goal.previousToken,
        static_cast<std::uint64_t>(goal.attempt),
        static_cast<std::uint64_t>(goal.composed)})
        hashGatherGoalInteger(hash,value);
}

inline bool validAttachedResidentGatherGoal(
    CharacterId actor,const PendingContextAction& pending,int currentMinute)
{
    const auto& goal=pending.gatherGoal;
    if(!goal.active()) return goal.token==0;
    return validResidentGatherGoal(goal)
        &&pending.active()
        &&pending.kind==ContextActionKind::Civilization
        &&pending.civilization.intent==CivilizationIntent::Gather
        &&pending.civilization.resourceNode!=0
        &&pending.civilization.quantity>0
        &&goal.actor==actor
        &&goal.token==pending.token
        &&goal.material==pending.civilization.material
        &&goal.issuedMinute==pending.issuedMinute
        &&goal.issuedMinute<=currentMinute;
}

template<typename W>
void writeResidentGatherGoalExtension(
    W& w,const SimulationStateSnapshot& snapshot)
{
    w.raw(ResidentGatherGoalExtensionMagic,sizeof(ResidentGatherGoalExtensionMagic));
    w.u32(ResidentGatherGoalExtensionVersion);
    std::vector<CharacterId> actors;
    for(const auto& entry:snapshot.runtime)
        if(entry.second.pendingContext.gatherGoal.active())
            actors.push_back(entry.first);
    std::sort(actors.begin(),actors.end());
    w.u32(static_cast<std::uint32_t>(actors.size()));
    std::uint64_t checksum=14695981039346656037ULL;
    hashGatherGoalInteger(checksum,actors.size());
    for(CharacterId actor:actors){
        const auto& goal=snapshot.runtime.at(actor).pendingContext.gatherGoal;
        w.u64(actor);
        w.u64(goal.goalId);
        w.u64(goal.token);
        w.enumeration(goal.material);
        w.i32(goal.createdMinute);
        w.i32(goal.issuedMinute);
        w.i32(goal.expiresMinute);
        w.i32(goal.inventoryAtIssue);
        w.i32(goal.targetQuantity);
        w.u64(goal.previousToken);
        w.i32(goal.attempt);
        w.boolean(goal.composed);
        hashGatherGoal(checksum,goal);
    }
    w.u64(checksum);
}

template<typename R>
bool readResidentGatherGoalExtension(R& r,SimulationStateSnapshot& snapshot)
{
    char magic[sizeof(ResidentGatherGoalExtensionMagic)]{};
    std::uint32_t version=0,count=0;
    if(!r.raw(magic,sizeof(magic))
       ||!std::equal(magic,magic+sizeof(magic),ResidentGatherGoalExtensionMagic)
       ||!r.u32(version) ||version!=ResidentGatherGoalExtensionVersion
       ||!r.count(count) ||count>snapshot.runtime.size()) return false;

    std::unordered_set<CharacterId> seen;
    std::vector<std::pair<CharacterId,ResidentGatherGoalLink>> staged;
    staged.reserve(count);
    std::uint64_t checksum=14695981039346656037ULL;
    hashGatherGoalInteger(checksum,count);
    for(std::uint32_t i=0;i<count;++i){
        ResidentGatherGoalLink goal;
        if(!r.u64(goal.actor) ||goal.actor==0
           ||!seen.insert(goal.actor).second
           ||!r.u64(goal.goalId) ||!r.u64(goal.token)
           ||!r.enumeration(goal.material)
           ||!r.i32(goal.createdMinute) ||!r.i32(goal.issuedMinute)
           ||!r.i32(goal.expiresMinute) ||!r.i32(goal.inventoryAtIssue)
           ||!r.i32(goal.targetQuantity) ||!r.u64(goal.previousToken)
           ||!r.i32(goal.attempt) ||!r.boolean(goal.composed)) return false;
        const auto found=snapshot.runtime.find(goal.actor);
        if(found==snapshot.runtime.end()
           ||found->second.pendingContext.gatherGoal.active()) return false;
        PendingContextAction pending=found->second.pendingContext;
        pending.gatherGoal=goal;
        if(!goal.active()
           ||!validAttachedResidentGatherGoal(goal.actor,pending,snapshot.world.minute))
            return false;
        staged.emplace_back(goal.actor,goal);
        hashGatherGoal(checksum,goal);
    }
    std::uint64_t expected=0;
    if(!r.u64(expected) ||expected!=checksum) return false;
    // A malformed later actor must never leave earlier links attached.
    for(const auto& [actor,goal]:staged)
        snapshot.runtime.at(actor).pendingContext.gatherGoal=goal;
    return true;
}

} // namespace lifelens
