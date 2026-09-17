#include "lifelens/Simulation.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace lifelens {
namespace {
constexpr int CivilizationKnowledgeWitnessRadiusTiles=2;
}

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

    const auto actorRuntime=runtime_.find(actor.id);
    if(actorRuntime==runtime_.end()) return;
    const GridPos demonstrationPos=actorRuntime->second.pos;

    for(auto& observer:world_.characters){
        if(!observer.alive || observer.id==actor.id) continue;

        const auto observerRuntime=runtime_.find(observer.id);
        if(observerRuntime==runtime_.end()
           || !contextActionNearTarget(
                observerRuntime->second.pos,
                demonstrationPos,
                CivilizationKnowledgeWitnessRadiusTiles)){
            continue;
        }

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

void Simulation::advanceCivilizationKnowledgeTeaching()
{
    if(world_.minute<=0 || world_.minute%60!=0) return;

    struct Candidate {
        CharacterId teacher=0;
        CharacterId learner=0;
        TechniqueId technique=TechniqueId::None;
        double score=0.0;
    } best;

    for(const Character& teacher:world_.characters){
        if(!teacher.alive) continue;
        const auto teacherRuntime=runtime_.find(teacher.id);
        if(teacherRuntime==runtime_.end() || teacherRuntime->second.pendingContext.active()) continue;

        for(const TechniqueKnowledge& record:teacher.civilization.knowledge.all()){
            if(static_cast<int>(record.level)<static_cast<int>(KnowledgeLevel::Reproducible)) continue;
            for(const Character& learner:world_.characters){
                if(!learner.alive || learner.id==teacher.id) continue;
                const auto learnerRuntime=runtime_.find(learner.id);
                if(learnerRuntime==runtime_.end() || learnerRuntime->second.pendingContext.active()) continue;

                const KnowledgeLevel learnerLevel=learner.civilization.knowledge.level(record.technique);
                if(static_cast<int>(learnerLevel)>=static_cast<int>(KnowledgeLevel::Reproducible)) continue;
                if(bestTechniqueFactForTeaching(
                    socialKnowledge_,teacher.id,learner.id,record.technique)==nullptr) continue;

                const double trust=learnerTrustInTeacher(
                    relationships_,learner.id,teacher.id);
                const double gap=std::max(0.0,std::min(1.0,
                    static_cast<double>(
                        static_cast<int>(record.level)-static_cast<int>(learnerLevel))/6.0));
                const double score=
                    0.08
                    +0.24*trust
                    +0.18*techniqueMasteryFactor(record.level)
                    +0.18*learner.civilization.learningSkill
                    +0.12*learner.personality.curiosity
                    +0.08*learner.personality.openness
                    +0.12*gap;
                if(score<0.36) continue;

                const bool better=score>best.score+1e-12;
                const bool tied=std::fabs(score-best.score)<=1e-12;
                if(better || (tied &&
                   (best.teacher==0 || teacher.id<best.teacher ||
                    (teacher.id==best.teacher && learner.id<best.learner) ||
                    (teacher.id==best.teacher && learner.id==best.learner &&
                     static_cast<int>(record.technique)<static_cast<int>(best.technique))))){
                    best={teacher.id,learner.id,record.technique,score};
                }
            }
        }
    }

    if(best.teacher==0 || best.learner==0 || best.technique==TechniqueId::None) return;

    Character* teacher=nullptr;
    Character* learner=nullptr;
    for(auto& character:world_.characters){
        if(character.id==best.teacher) teacher=&character;
        if(character.id==best.learner) learner=&character;
    }
    if(teacher==nullptr || learner==nullptr) return;

    auto teacherRuntime=runtime_.find(best.teacher);
    const auto learnerRuntime=runtime_.find(best.learner);
    if(teacherRuntime==runtime_.end() || learnerRuntime==runtime_.end()
       || teacherRuntime->second.pendingContext.active()
       || learnerRuntime->second.pendingContext.active()) return;

    PendingContextAction pending;
    pending.token=nextContextActionToken();
    pending.kind=ContextActionKind::KnowledgeTeaching;
    pending.issuedMinute=world_.minute;
    pending.knowledgeTeachingTarget=best.learner;
    pending.knowledgeTeachingTechnique=best.technique;
    pending.knowledgeTeachingScore=best.score;

    Runtime& runtime=teacherRuntime->second;
    runtime.pendingContext=pending;
    runtime.goal=Goal::Idle;
    runtime.plan.clear();
    runtime.actionIndex=0;
    runtime.announced=false;
    runtime.civilizationActive=false;
    runtime.socialActive=false;
    runtime.socialIntent=SocialIntent::None;
    runtime.socialTarget=0;

    if(!world_.externalPhysicalExecution){
        completeContextAction(
            *teacher,
            runtime,
            pending.token,
            learnerRuntime->second.pos);
    }
}

} // namespace lifelens
