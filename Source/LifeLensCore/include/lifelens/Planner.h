#pragma once
#include <vector>
#include "UtilityAI.h"
namespace lifelens {
enum class ActionType { FindObject, Reserve, MoveTo, Use, Release, Idle };
struct Action { ActionType type=ActionType::Idle; ObjectId objectId=0; int remainingTicks=0; };
inline std::vector<Action> buildPlan(const World& w,const Character& c,Goal g,GridPos from={}) {
    if(g==Goal::Idle) return {{ActionType::Idle,0,5}};
    const auto kind=objectKindFor(g);
    for(const auto& o:w.objects){
        if(o.kind==kind && (!o.reservedBy || *o.reservedBy==c.id)){
            const int travel=std::max(1,manhattan(from,o.pos));
            return {{ActionType::FindObject,o.id,0},{ActionType::Reserve,o.id,0},{ActionType::MoveTo,o.id,travel},{ActionType::Use,o.id,std::max(1,o.useDurationTicks)},{ActionType::Release,o.id,0}};
        }
    }
    return {};
}
}
