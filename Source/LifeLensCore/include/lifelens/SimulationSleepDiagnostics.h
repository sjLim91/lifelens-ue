#pragma once
#include <array>
#include <cstdint>
#include "Facility.h"
#include "Planner.h"
#include "SleepEnvironment.h"

namespace lifelens {
// Opt-in harness instrumentation only. Never saved, serialized to the Web,
// consulted by AI, or used to change the simulation's decisions.
enum class SleepDiagnosticEnd { Rested, UrgentNeed, Budget, Replan, Failure, Death };
struct SleepDiagnosticCounters {
    std::array<std::uint64_t,3> recoveryMinutes{};
    std::array<std::uint64_t,6> sessionEnds{};
    std::array<std::uint64_t,6> failedPlansByGoal{};
    std::uint64_t sleepSelections=0, shelterRejectedDistance=0, shelterRejectedCapacity=0;
    std::uint64_t shelterRejectedRoute=0, replans=0, backoffRestStarts=0;
    std::uint64_t sessionStarts=0, sessionClosed=0;
    double startNeedSum=0, endNeedSum=0, grossRecoverySum=0, lastGrossRecovery=0;
    bool sessionActive=false;
    FacilityId selectedFacility=0;
    FacilityKind selectedKind=FacilityKind::SleepingPlace;
    GridPos selectedPosition{};
    int selectedDistance=0, lastReplanMinute=-1, lastFailureMinute=-1;
    int lastSessionEnd=-1, lastSessionEndMinute=-1;
    Goal lastFailedGoal=Goal::Idle;
    bool lastFailureWasRoute=false;
};
struct SleepRuntimeDiagnostic {
    Goal goal=Goal::Idle;
    ActionType action=ActionType::Idle;
    bool hasAction=false, hasTarget=false, arrived=false;
    GridPos position{},target{};
    int remainingTicks=0, penaltyUntilMinute=0, consecutiveFailures=0;
    SleepDiagnosticCounters counters;
};
}
