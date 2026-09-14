#pragma once
#include <vector>
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

inline std::vector<Action> buildPlan(const World& w,Character& c,Goal g,GridPos from={}) {
    if(g==Goal::Idle) return {{ActionType::Idle,0,5}};
    const auto kind=objectKindFor(g);
    for(const auto& o:w.objects){
        if(o.kind==kind && (!o.reservedBy || *o.reservedBy==c.id)){
            const int travel=std::max(1,manhattan(from,o.pos));
            return {{ActionType::FindObject,o.id,0},{ActionType::Reserve,o.id,0},{ActionType::MoveTo,o.id,travel},{ActionType::Use,o.id,std::max(1,o.useDurationTicks)},{ActionType::Release,o.id,0}};
        }
    }
    if(!emergencyAffordanceAvailableFor(c,g)) return {};

    // Emergency Eat/Drink must consume a real provision; fallback must never
    // synthesize food or water merely because the need exists.
    if(g==Goal::Eat && !c.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::PlantFood,1)) return {};
    if(g==Goal::Drink && !c.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,1)) return {};

    return {{ActionType::EmergencyUse,0,emergencyUseDurationTicks(g)}};
}
}
