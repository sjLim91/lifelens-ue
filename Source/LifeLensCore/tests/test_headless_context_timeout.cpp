#include <cassert>
#include <iostream>
#include <string>

#include "lifelens/Simulation.h"

using namespace lifelens;

namespace {

Simulation makeSimulation()
{
    SimulationRuleset rules=DefaultSimulationRuleset;
    rules.needs.hungerPerMinute=0.0;
    rules.needs.thirstPerMinute=0.0;
    rules.needs.sleepPerMinute=0.0;
    rules.needs.bladderPerMinute=0.0;
    rules.needs.hygienePerMinute=0.0;

    Simulation simulation(
        881122,
        0,
        CurrentWorldGenerationVersion,
        rules);
    simulation.setupNewGame();
    return simulation;
}

bool timeoutLogSeen(const Simulation& simulation,const std::string& name)
{
    for(const auto& line:simulation.logs()){
        if(line.find(name+" context action timed out")!=std::string::npos){
            return true;
        }
    }
    return false;
}

void installUnresolvableTeaching(
    Simulation& simulation,
    int issuedMinute)
{
    SimulationStateSnapshot snapshot=simulation.captureSnapshot();
    assert(!snapshot.world.characters.empty());

    const CharacterId teacher=snapshot.world.characters.front().id;
    auto runtime=snapshot.runtime.find(teacher);
    assert(runtime!=snapshot.runtime.end());

    runtime->second.plan.clear();
    runtime->second.actionIndex=0;
    runtime->second.pendingContext.clear();
    runtime->second.pendingContext.token=777;
    runtime->second.pendingContext.kind=ContextActionKind::KnowledgeTeaching;
    runtime->second.pendingContext.issuedMinute=issuedMinute;
    runtime->second.pendingContext.knowledgeTeachingTarget=999999;
    runtime->second.pendingContext.knowledgeTeachingTechnique=TechniqueId::SharpFlake;
    runtime->second.pendingContext.knowledgeTeachingScore=0.8;

    std::string error;
    assert(simulation.restoreSnapshot(snapshot,&error));
}

}

int main()
{
    {
        Simulation simulation=makeSimulation();
        SimulationStateSnapshot snapshot=simulation.captureSnapshot();
        const CharacterId teacher=snapshot.world.characters.front().id;
        const std::string teacherName=snapshot.world.characters.front().name;
        snapshot.world.minute=600;
        std::string error;
        assert(simulation.restoreSnapshot(snapshot,&error));

        installUnresolvableTeaching(
            simulation,
            600-contextActionTimeoutMinutes(
                ContextActionKind::KnowledgeTeaching));

        simulation.step();

        const SimulationStateSnapshot after=simulation.captureSnapshot();
        const auto runtime=after.runtime.find(teacher);
        assert(runtime!=after.runtime.end());
        assert(!runtime->second.pendingContext.active());
        assert(timeoutLogSeen(simulation,teacherName));
    }

    {
        Simulation simulation=makeSimulation();
        SimulationStateSnapshot snapshot=simulation.captureSnapshot();
        const CharacterId teacher=snapshot.world.characters.front().id;
        snapshot.world.minute=600;
        std::string error;
        assert(simulation.restoreSnapshot(snapshot,&error));

        installUnresolvableTeaching(
            simulation,
            600-contextActionTimeoutMinutes(
                ContextActionKind::KnowledgeTeaching)+1);

        simulation.step();

        const SimulationStateSnapshot after=simulation.captureSnapshot();
        const auto runtime=after.runtime.find(teacher);
        assert(runtime!=after.runtime.end());
        assert(runtime->second.pendingContext.active());
    }

    std::cout<<"headless context timeout contract PASS\n";
    return 0;
}
