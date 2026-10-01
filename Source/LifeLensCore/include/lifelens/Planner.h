#pragma once
#include <cmath>
#include <limits>
#include <vector>
#include "EnvironmentalConsequences.h"
#include "EnvironmentalExposure.h"
#include "UtilityAI.h"
namespace lifelens {
enum class ActionType { FindObject, Reserve, MoveTo, Use, Release, Idle, EmergencyUse };
struct Action { ActionType type=ActionType::Idle; ObjectId objectId=0; int remainingTicks=0; };

inline int emergencyUseDurationTicks(Goal g)
{
    switch(g){
        case Goal::Eat: return 1;
        case Goal::Drink: return 1;
        case Goal::Sleep: return 8;
        case Goal::UseToilet: return 2;
        case Goal::Wash: return 4;
        case Goal::Idle:
        default: return 1;
    }
}

inline NeedsDelta emergencyUseEffectPerTick(Goal g)
{
    switch(g){
        case Goal::Eat: return {-0.28,0,0,0,0};
        case Goal::Drink: return {0,-0.32,0,0,0};
        case Goal::Sleep: return {0,0,-0.00150,0,0};
        case Goal::UseToilet: return {0,0,0,-0.13,0.012};
        case Goal::Wash: return {0,0,0,0,-0.018};
        case Goal::Idle:
        default: return {};
    }
}

inline constexpr double RestedSleepNeedTarget = 0.12;
inline constexpr int MinimumSleepSessionMinutes = 30;
inline constexpr int MaximumSleepSessionMinutes = 10 * 60;

inline constexpr double SleepUrgentNeedWakeThreshold = 0.72;
inline constexpr double CriticalSurvivalDominanceMargin = 0.05;

inline bool sleepInterruptedByUrgentNeed(const Character& character)
{
    // Sleep yields as soon as a survival need enters the urgent utility band.
    // A much higher wake threshold lets long, low-quality outdoor sleep pin
    // hunger/thirst near the hard clamp before the resident can re-plan.
    return character.needs.thirst>=SleepUrgentNeedWakeThreshold
        || character.needs.bladder>=SleepUrgentNeedWakeThreshold
        || character.needs.hunger>=SleepUrgentNeedWakeThreshold;
}

inline int sleepDurationMinutesForNeed(
    const Character& character,
    double recoveryPerMinute,
    const NeedsRuleset& rules=DefaultSimulationRuleset.needs)
{
    const double fatigueToRecover=std::max(
        0.0,
        character.needs.sleep-RestedSleepNeedTarget);
    if(fatigueToRecover<=1e-9) return MinimumSleepSessionMinutes;

    // Need decay continues while asleep, so only recovery above the resident's
    // own fatigue accrual counts as net rest. Duration is therefore a real
    // consequence of both current fatigue and sleep quality.
    const double fatigueAccrual=
        rules.sleepPerMinute*std::max(0.0,character.sleepTendency);
    const double netRecovery=std::max(
        0.00010,
        recoveryPerMinute-fatigueAccrual);
    return std::clamp(
        static_cast<int>(std::ceil(fatigueToRecover/netRecovery)),
        MinimumSleepSessionMinutes,
        MaximumSleepSessionMinutes);
}

inline int facilityUseDurationTicks(Goal g)
{
    switch(g){
        case Goal::Eat: return 9;
        case Goal::Drink: return 8;
        case Goal::Sleep: return 16;
        case Goal::UseToilet: return 7;
        case Goal::Wash: return 8;
        case Goal::Idle:
        default: return 1;
    }
}

inline NeedsDelta facilityUseEffectPerTick(Goal g)
{
    switch(g){
        case Goal::Eat: return {-0.075,0,0,0,0};
        case Goal::Drink: return {0,-0.085,0,0,0};
        case Goal::Sleep: return {0,0,-0.00220,0,0};
        case Goal::UseToilet: return {0,0,0,-0.12,0};
        case Goal::Wash: return {0,0,0,0,-0.055};
        case Goal::Idle:
        default: return {};
    }
}

inline int environmentAdjustedTravelTicks(const World& world,GridPos from,GridPos to)
{
    const int baseTravel=std::max(1,manhattan(from,to));
    const DynamicEnvironmentObservation environment=deriveDynamicEnvironment(
        world.genesisIdentity(),chunkCoordForGrid(from),world.minute);
    const EnvironmentalConsequenceProfile consequence=deriveEnvironmentalConsequences(environment);
    const double multiplier=1.0+1.50*consequence.travelFriction01;
    return std::max(baseTravel,static_cast<int>(std::ceil(static_cast<double>(baseTravel)*multiplier)));
}

inline std::vector<Action> buildPlan(const World& w,Character& c,Goal g,GridPos from={}) {
    // Environmental perception is sampled at authoritative planning boundaries.
    // This keeps the feedback loop in Core (not presentation) while avoiding a
    // second per-frame simulation authority in Unreal.
    perceiveEnvironmentalContamination(c,w.environmentalResidues,from,w.minute);

    if(g==Goal::Idle) return {{ActionType::Idle,0,5}};
    // SmartObjects never synthesize provisions. Food requires carried stock;
    // water goals may instead travel to an authoritative natural freshwater
    // source and use it directly before portable containers exist.
    const bool carriedProvision=physicalProvisionAvailableFor(c,g);
    const bool directNaturalWater=canUseNaturalWaterDirectly(w,c,g);
    if(!carriedProvision && !directNaturalWater) return {};

    const auto kind=objectKindFor(g);
    bool hasObject=false;
    if(carriedProvision) for(const auto& o:w.objects){
        if(o.kind==kind && (!o.reservedBy || *o.reservedBy==c.id)){
            hasObject=true;
            if(!w.externalPhysicalExecution){
                const int travel=environmentAdjustedTravelTicks(w,from,o.pos);
                const int useDuration=g==Goal::Sleep
                    ? sleepDurationMinutesForNeed(
                        c,
                        -facilityUseEffectPerTick(Goal::Sleep).sleep)
                    : std::max(1,o.useDurationTicks);
                return {{ActionType::FindObject,o.id,0},{ActionType::Reserve,o.id,0},{ActionType::MoveTo,o.id,travel},{ActionType::Use,o.id,useDuration},{ActionType::Release,o.id,0}};
            }
            break;
        }
    }

    const bool hasEmergency=
        emergencyAffordanceAvailableFor(c,g)
        || directNaturalWater;
    if(!hasObject && !hasEmergency) return {};

    if(w.externalPhysicalExecution){
        // Unreal/another physical executor owns movement and arrival. Keep the
        // authoritative Core intent alive without applying need effects, moving
        // Core position, consuming provisions, or creating residues until the
        // executor explicitly acknowledges completion.
        return {{ActionType::Idle,0,std::numeric_limits<int>::max()}};
    }

    if(hasObject && carriedProvision){
        for(const auto& o:w.objects){
            if(o.kind==kind && (!o.reservedBy || *o.reservedBy==c.id)){
                const int travel=environmentAdjustedTravelTicks(w,from,o.pos);
                const int useDuration=g==Goal::Sleep
                    ? sleepDurationMinutesForNeed(
                        c,
                        -facilityUseEffectPerTick(Goal::Sleep).sleep)
                    : std::max(1,o.useDurationTicks);
                return {{ActionType::FindObject,o.id,0},{ActionType::Reserve,o.id,0},{ActionType::MoveTo,o.id,travel},{ActionType::Use,o.id,useDuration},{ActionType::Release,o.id,0}};
            }
        }
    }

    // Provision consumption happens when the action actually begins, not while
    // merely planning. This keeps interrupted movement from deleting supplies.
    const int emergencyDuration=g==Goal::Sleep
        ? sleepDurationMinutesForNeed(
            c,
            -emergencyUseEffectPerTick(Goal::Sleep).sleep)
        : emergencyUseDurationTicks(g);
    return {{ActionType::EmergencyUse,0,emergencyDuration}};
}
}
