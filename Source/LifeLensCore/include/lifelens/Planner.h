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
        case Goal::Sleep: return {0,0,-0.035,0,0};
        case Goal::UseToilet: return {0,0,0,-0.13,0.012};
        case Goal::Wash: return {0,0,0,0,-0.018};
        case Goal::Idle:
        default: return {};
    }
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
        case Goal::Sleep: return {0,0,-0.055,0,0};
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

    const auto kind=objectKindFor(g);
    bool hasObject=false;
    for(const auto& o:w.objects){
        if(o.kind==kind && (!o.reservedBy || *o.reservedBy==c.id)){
            hasObject=true;
            if(!w.externalPhysicalExecution){
                const int travel=environmentAdjustedTravelTicks(w,from,o.pos);
                return {{ActionType::FindObject,o.id,0},{ActionType::Reserve,o.id,0},{ActionType::MoveTo,o.id,travel},{ActionType::Use,o.id,std::max(1,o.useDurationTicks)},{ActionType::Release,o.id,0}};
            }
            break;
        }
    }

    const bool hasEmergency=emergencyAffordanceAvailableFor(c,g);
    if(!hasObject && !hasEmergency) return {};

    if(w.externalPhysicalExecution){
        // Unreal/another physical executor owns movement and arrival. Keep the
        // authoritative Core intent alive without applying need effects, moving
        // Core position, consuming provisions, or creating residues until the
        // executor explicitly acknowledges completion.
        return {{ActionType::Idle,0,std::numeric_limits<int>::max()}};
    }

    if(hasObject){
        for(const auto& o:w.objects){
            if(o.kind==kind && (!o.reservedBy || *o.reservedBy==c.id)){
                const int travel=environmentAdjustedTravelTicks(w,from,o.pos);
                return {{ActionType::FindObject,o.id,0},{ActionType::Reserve,o.id,0},{ActionType::MoveTo,o.id,travel},{ActionType::Use,o.id,std::max(1,o.useDurationTicks)},{ActionType::Release,o.id,0}};
            }
        }
    }

    // Emergency Eat/Drink must consume a real provision; fallback must never
    // synthesize food or water merely because the need exists.
    if(g==Goal::Eat && !c.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::PlantFood,1)) return {};
    if(g==Goal::Drink && !c.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,1)) return {};

    return {{ActionType::EmergencyUse,0,emergencyUseDurationTicks(g)}};
}
}
