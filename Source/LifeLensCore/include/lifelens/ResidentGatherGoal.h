#pragma once

#include <cstdint>
#include "Civilization.h"

namespace lifelens {

// The material-acquisition predicate of the existing single Core decision.
// A goal survives bounded alternative steps; every step still has its own
// physical ContextAction token. This is neither an executor nor a reward.
struct ResidentGatherGoalLink {
    std::uint64_t goalId=0;
    std::uint64_t token=0;
    CharacterId actor=0;
    MaterialKind material=MaterialKind::Unknown;
    int createdMinute=-1;
    int issuedMinute=-1;
    int expiresMinute=-1;
    int inventoryAtIssue=0;
    int targetQuantity=0;
    std::uint64_t previousToken=0;
    int attempt=0;
    bool composed=false;

    bool active() const { return goalId!=0; }
};

inline constexpr int ResidentGatherGoalMaxAttempts=3;
inline constexpr int ResidentGatherGoalMaxMinutes=120;

inline bool validResidentGatherGoal(const ResidentGatherGoalLink& goal)
{
    return goal.active() &&goal.token!=0 &&goal.actor!=0
        &&goal.material>=MaterialKind::Stone &&goal.material<=MaterialKind::Bronze
        &&goal.createdMinute>=0 &&goal.issuedMinute>=goal.createdMinute
        &&goal.expiresMinute>=goal.issuedMinute
        &&static_cast<std::int64_t>(goal.expiresMinute)-goal.createdMinute
            <=ResidentGatherGoalMaxMinutes
        &&goal.inventoryAtIssue>=0 &&goal.targetQuantity>goal.inventoryAtIssue
        &&goal.targetQuantity<=100000
        &&goal.attempt>=1 &&goal.attempt<=ResidentGatherGoalMaxAttempts
        &&(goal.attempt==1
            ? goal.goalId==goal.token &&goal.previousToken==0
            : goal.previousToken!=0 &&goal.previousToken!=goal.token
                &&goal.goalId!=goal.token);
}

} // namespace lifelens
