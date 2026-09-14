#include "lifelens/Simulation.h"

#include <algorithm>
#include <cstdint>
#include <sstream>

namespace lifelens {

void Simulation::processCivilizationKnowledgeEvent(
    Character& actor,
    const CivilizationEvent& event)
{
    if((event.type!=CivilizationEventType::Discovered &&
        event.type!=CivilizationEventType::Crafted) ||
       event.technique==TechniqueId::None || actor.id==0) return;

    const KnowledgeReceipt* origin=registerTechniqueOrigin(
        socialKnowledge_,actor,event.technique,world_.minute,event.type,world_.seed);
    if(origin==nullptr) return;

    const SocialFact* fact=socialKnowledge_.findFact(origin->factId);
    if(fact==nullptr) return;

    for(auto& observer:world_.characters){
        if(!observer.alive || observer.id==actor.id) continue;

        const Relationship* relation=relationships_.find(observer.id,actor.id);
        const double familiarity=relation ? relation->familiarity : 0.0;
        const double trust=relation ? relation->trust : 0.0;
        const double witnessChance=std::max(0.0,std::min(0.90,
            0.16
            +0.22*observer.personality.curiosity
            +0.12*observer.personality.sociability
            +0.12*familiarity
            +0.06*trust
            +(event.type==CivilizationEventType::Discovered ? 0.10 : 0.04)));
        const double witnessRoll=deterministicKnowledgeUnit(
            world_.seed+7919ULL,
            fact->id,actor.id,observer.id,
            static_cast<std::uint64_t>(std::max(0,world_.minute/15)));
        if(witnessRoll>=witnessChance) continue;

        const KnowledgeLevel before=observer.civilization.knowledge.level(event.technique);
        const TechniqueTransmissionOutcome outcome=applyTechniqueWitness(
            socialKnowledge_,*fact,actor,observer,world_.seed,world_.minute);
        if(!outcome.receiptAccepted) continue;

        std::ostringstream s;
        s<<observer.name<<" witnessed "<<actor.name<<" demonstrate "
         <<techniqueName(event.technique);
        if(static_cast<int>(outcome.after)>static_cast<int>(before)){
            s<<" -> knowledge "<<static_cast<int>(before)
             <<"->"<<static_cast<int>(outcome.after);
        }
        emit(s.str());
    }
}

} // namespace lifelens
