#pragma once
#include "ResidentProductionPlan.h"
#include <ostream>
namespace lifelens {
inline void appendProductionPlanJson(std::ostream& out,const ResidentProductionPlan* p) {
    if(!p ||!p->id){out<<"null";return;}
    out<<"{\"id\":\""<<p->id<<"\",\"target\":\""<<techniqueName(p->target)
       <<"\",\"createdMinute\":"<<p->createdMinute<<",\"expiresMinute\":"<<p->expiresMinute
       <<",\"finishedMinute\":"<<p->finishedMinute<<",\"targetStock\":"<<p->targetStock
       <<",\"cognitiveOrigin\":"<<(p->cognitiveFingerprint?"true":"false")
       <<",\"state\":"<<static_cast<int>(p->state)<<",\"reason\":"<<static_cast<int>(p->reason)<<",\"steps\":[";
    bool comma=false;for(const auto& s:p->steps) {
        if(comma)out<<",";comma=true;
        out<<"{\"token\":\""<<s.token<<"\",\"issuedMinute\":"<<s.issuedMinute<<",\"finishedMinute\":"<<s.finishedMinute
           <<",\"action\":\""<<civilizationIntentName(s.intent)<<"\",\"technique\":\""<<techniqueName(s.technique)
           <<"\",\"material\":\""<<materialName(s.material)<<"\",\"source\":\""<<s.source<<"\",\"result\":"<<static_cast<int>(s.result)
           <<",\"reason\":"<<static_cast<int>(s.reason)<<",\"stockBefore\":"<<s.stockBefore<<",\"stockAfter\":"<<s.stockAfter
           <<",\"from\":{\"x\":"<<s.from.x<<",\"y\":"<<s.from.y<<"},\"to\":{\"x\":"<<s.to.x<<",\"y\":"<<s.to.y
           <<"},\"attempted\":"<<(s.attempted?"true":"false")<<"}";
    }out<<"]}";
}
} // namespace lifelens
