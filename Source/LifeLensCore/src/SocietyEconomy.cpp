#include "lifelens/Simulation.h"
#include "lifelens/SocietyEconomy.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace lifelens {

void Simulation::advanceSocietyExchange()
{
    if(world_.minute<=0 || world_.minute%60!=0) return;

    SocietyExchangePlan best;
    for(std::size_t i=0;i<world_.characters.size();++i){
        Character& first=world_.characters[i];
        if(!first.alive || requiresDirectCare(first.lifeStage)) continue;
        const auto firstRuntime=runtime_.find(first.id);
        if(firstRuntime==runtime_.end()
           || firstRuntime->second.pendingContext.active()
           || firstRuntime->second.socialActive) continue;

        for(std::size_t j=i+1;j<world_.characters.size();++j){
            Character& second=world_.characters[j];
            if(!second.alive || requiresDirectCare(second.lifeStage)) continue;
            const auto secondRuntime=runtime_.find(second.id);
            if(secondRuntime==runtime_.end()
               || secondRuntime->second.pendingContext.active()
               || secondRuntime->second.socialActive) continue;

            const int distance=
                std::abs(firstRuntime->second.pos.x-secondRuntime->second.pos.x)
                +std::abs(firstRuntime->second.pos.y-secondRuntime->second.pos.y);
            if(distance>1) continue;

            SocietyExchangePlan candidate=
                bestMutualExchangePlan(first,second,relationships_);
            if(candidate.score>best.score+1e-12){
                best=candidate;
            }
        }
    }

    constexpr double ExchangeThreshold=0.66;
    if(!best.valid() || best.score<ExchangeThreshold) return;

    Character* first=nullptr;
    Character* second=nullptr;
    for(auto& resident:world_.characters){
        if(resident.id==best.first) first=&resident;
        if(resident.id==best.second) second=&resident;
    }
    if(first==nullptr || second==nullptr) return;
    if(!executeMutualExchange(*first,*second,best)) return;

    registerSocietyExchangeFact(
        socialKnowledge_,*first,*second,best,world_.minute,world_.seed);

    relationships_.getOrCreate(first->id,second->id).apply(
        relationshipDeltaFor(RelationshipEvent::SharedPositiveExperience,0.35));
    relationships_.getOrCreate(second->id,first->id).apply(
        relationshipDeltaFor(RelationshipEvent::SharedPositiveExperience,0.35));

    std::ostringstream log;
    log<<first->name<<" exchanged "<<materialName(best.firstGives)
       <<" with "<<second->name<<" for "<<materialName(best.secondGives);
    emit(log.str());
}

} // namespace lifelens
