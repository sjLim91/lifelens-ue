#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>

#include "lifelens/CivilizationKnowledgeTransmission.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

namespace {

Simulation makeTeachingSimulation(int learnerDistance)
{
    SimulationRuleset rules=DefaultSimulationRuleset;
    rules.needs.hungerPerMinute=0.0;
    rules.needs.thirstPerMinute=0.0;
    rules.needs.sleepPerMinute=0.0;
    rules.needs.bladderPerMinute=0.0;
    rules.needs.hygienePerMinute=0.0;

    Simulation simulation(
        991701,
        0,
        CurrentWorldGenerationVersion,
        rules);
    simulation.setupNewGame();
    assert(simulation.world().characters.size()>=2);
    simulation.world().characters.resize(2);

    Character& teacher=simulation.world().characters[0];
    Character& learner=simulation.world().characters[1];

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

    Relationship& learnerToTeacher=
        simulation.relationships().getOrCreate(learner.id,teacher.id);
    learnerToTeacher.trust=1.0;
    learnerToTeacher.respect=1.0;
    learnerToTeacher.familiarity=1.0;
    learnerToTeacher.comfort=1.0;

    SimulationStateSnapshot snapshot=simulation.captureSnapshot();
    snapshot.world.minute=539;

    const CharacterId teacherId=teacher.id;
    const CharacterId learnerId=learner.id;
    for(auto it=snapshot.runtime.begin();it!=snapshot.runtime.end();){
        if(it->first!=teacherId && it->first!=learnerId){
            it=snapshot.runtime.erase(it);
        }else{
            ++it;
        }
    }

    snapshot.runtime[teacherId]=SimulationRuntimeSnapshot{};
    snapshot.runtime[teacherId].pos={0,0};
    snapshot.runtime[learnerId]=SimulationRuntimeSnapshot{};
    snapshot.runtime[learnerId].pos={learnerDistance,0};

    std::string error;
    assert(simulation.restoreSnapshot(snapshot,&error));
    return simulation;
}

bool teachingLogSeen(const Simulation& simulation)
{
    for(const auto& line:simulation.logs()){
        if(line.find(" taught ")!=std::string::npos
           || line.find(" tried teaching ")!=std::string::npos){
            return true;
        }
    }
    return false;
}

}

int main()
{
    {
        Simulation nearby=makeTeachingSimulation(2);
        const CharacterId teacherId=nearby.world().characters[0].id;

        nearby.step();

        const SimulationStateSnapshot scheduled=nearby.captureSnapshot();
        const auto runtime=scheduled.runtime.find(teacherId);
        assert(runtime!=scheduled.runtime.end());
        assert(runtime->second.pendingContext.active());
        assert(runtime->second.pendingContext.kind==
            ContextActionKind::KnowledgeTeaching);

        nearby.step();
        assert(teachingLogSeen(nearby));
    }

    {
        Simulation distant=makeTeachingSimulation(3);
        const CharacterId teacherId=distant.world().characters[0].id;

        distant.step();

        const SimulationStateSnapshot after=distant.captureSnapshot();
        const auto runtime=after.runtime.find(teacherId);
        assert(runtime!=after.runtime.end());
        assert(!runtime->second.pendingContext.active());
        assert(!teachingLogSeen(distant));
    }

    std::cout<<"knowledge teaching locality contract PASS\n";
    return 0;
}
