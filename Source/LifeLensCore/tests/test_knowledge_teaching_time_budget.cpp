#include <cassert>
#include <iostream>
#include <string>

#include "lifelens/CivilizationKnowledgeTransmission.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

namespace {

bool teachingLogSeen(const Simulation& simulation,const std::string& teacher)
{
    for(const auto& line:simulation.logs()){
        if(line.find(teacher+" taught ")!=std::string::npos
           || line.find(teacher+" tried teaching ")!=std::string::npos){
            return true;
        }
    }
    return false;
}

}

int main()
{
    SimulationRuleset rules=DefaultSimulationRuleset;
    rules.needs.hungerPerMinute=0.0;
    rules.needs.thirstPerMinute=0.0;
    rules.needs.sleepPerMinute=0.0;
    rules.needs.bladderPerMinute=0.0;
    rules.needs.hygienePerMinute=0.0;

    Simulation simulation(
        95101,
        0,
        CurrentWorldGenerationVersion,
        rules);
    simulation.setupNewGame();
    assert(simulation.world().characters.size()>=2);
    // Isolate one teacher/learner pair so the deterministic scheduler cannot
    // legitimately choose a different founder with an equivalent opportunity.
    simulation.world().characters.resize(2);

    Character& teacher=simulation.world().characters[0];
    Character& learner=simulation.world().characters[1];
    const CharacterId teacherId=teacher.id;

    teacher.civilization.knowledge.learn(
        TechniqueId::SharpFlake,
        KnowledgeLevel::Mastered,
        0.99);
    learner.civilization.learningSkill=1.0;
    learner.personality.curiosity=1.0;
    learner.personality.openness=1.0;

    const KnowledgeReceipt* origin=registerTechniqueOrigin(
        simulation.socialKnowledge(),
        teacher,
        TechniqueId::SharpFlake,
        simulation.world().minute,
        CivilizationEventType::Crafted,
        simulation.world().seed);
    assert(origin!=nullptr);
    const SocialFact* fact=
        simulation.socialKnowledge().findFact(origin->factId);
    assert(fact!=nullptr);
    assert(applyTechniqueWitness(
        simulation.socialKnowledge(),
        *fact,
        teacher,
        learner,
        simulation.world().seed,
        simulation.world().minute).receiptAccepted);

    Relationship& learnerToTeacher=
        simulation.relationships().getOrCreate(learner.id,teacher.id);
    learnerToTeacher.trust=1.0;
    learnerToTeacher.respect=1.0;
    learnerToTeacher.familiarity=1.0;
    learnerToTeacher.comfort=1.0;

    // The simulation starts at minute 480. Keep the teacher in a real,
    // long-running sleep plan across the minute-540 teaching scheduler tick.
    teacher.needs={0.05,0.05,1.0,0.05,0.05};
    learner.needs={0.05,0.05,0.05,0.05,0.05};

    simulation.runMinutes(60);
    assert(simulation.world().minute==540);

    const ResidentPresentationObservation duringSleep=
        simulation.observeResidentPresentation(teacherId);
    assert(duringSleep.active);
    assert(duringSleep.kind==PresentationActionKind::Physical);
    assert(duringSleep.physicalGoal==Goal::Sleep);
    assert(!teachingLogSeen(simulation,teacher.name));

    // Once the physical commitment is complete, the same authoritative
    // knowledge opportunity remains eligible at a later scheduler boundary.
    teacher.needs.sleep=0.10;
    simulation.step();
    assert(simulation.world().minute==541);

    simulation.runMinutes(59);
    assert(simulation.world().minute==600);

    const ResidentPresentationObservation teaching=
        simulation.observeResidentPresentation(teacherId);
    assert(teaching.active);
    assert(teaching.kind==PresentationActionKind::KnowledgeTeaching);
    assert(teaching.knowledgeTeachingTechnique==TechniqueId::SharpFlake);
    assert(teaching.targetResidentId==learner.id);

    std::cout<<"knowledge teaching respects active life-action time budget PASS\n";
    return 0;
}
