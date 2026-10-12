#include <cassert>
#include <cmath>
#include <string>
#include "lifelens/ActionExperienceLearning.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/UtilityAI.h"

using namespace lifelens;

int main()
{
    World world(99);
    Character actor;
    actor.id=77;
    actor.needs.sleep=0.55;
    actor.needs.bladder=0.55;

    const double before=scoreGoal(world,actor,Goal::Sleep);
    assert(std::abs(learnedActionDisposition(actor,"Sleep")-1.0)<1e-12);

    for(int attempt=1;attempt<=9;++attempt){
        rememberActionOutcome(actor,"Sleep",attempt,false);
    }
    const double discouraged=scoreGoal(world,actor,Goal::Sleep);
    assert(discouraged<before);
    assert(discouraged>0.0);

    const BeliefRecord* experience=actor.beliefs.find(
        0,actionExperienceProposition("Sleep"));
    assert(experience && experience->contradictionCount==9);
    assert(actor.memory.entries.size()==1);
    // Duplicate feedback during the same tick cannot forge repeated learning.
    rememberActionOutcome(actor,"Sleep",9,false);
    assert(experience->contradictionCount==9);

    for(int attempt=10;attempt<=38;++attempt){
        rememberActionOutcome(actor,"Sleep",attempt,true);
    }
    const double recovered=scoreGoal(world,actor,Goal::Sleep);
    assert(recovered>discouraged);
    assert(actor.memory.entries.size()==2);
    assert(experience->supportWeight+experience->contradictionWeight<15.0);

    // Critical physical needs must dominate both success and failure bias.
    actor.needs.sleep=DefaultSimulationRuleset.utilityAI.urgentThreshold+0.01;
    const double criticalWithHistory=scoreGoal(world,actor,Goal::Sleep);
    Character newcomer=actor;
    newcomer.beliefs=BeliefState{};
    newcomer.memory=MemoryState{};
    const double criticalNeutral=scoreGoal(world,newcomer,Goal::Sleep);
    assert(std::abs(criticalWithHistory-criticalNeutral)<1e-12);

    // One resident's learning does not magically transfer to another.
    assert(std::abs(learnedActionDisposition(newcomer,"Sleep")-1.0)<1e-12);
    assert(learnedActionDisposition(actor,"Sleep")>1.0);

    // Belief and the memory of an actual attempt survive authoritative
    // save/load, with no new binary schema or external model dependency.
    Simulation simulation(15151);
    simulation.setupNewGame();
    const CharacterId founder=simulation.world().characters.front().id;
    for(Character& individual:simulation.world().characters){
        if(individual.id==founder){
            individual.beliefs=actor.beliefs;
            individual.memory=actor.memory;
        }
    }
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error));
    SimulationStateSnapshot restored;
    assert(decodeSimulationSnapshot(bytes,restored,&error));
    const Character* remembered=nullptr;
    for(const Character& individual:restored.world.characters){
        if(individual.id==founder){remembered=&individual;break;}
    }
    assert(remembered!=nullptr);
    assert(remembered->memory.entries.size()==2);
    assert(std::abs(learnedActionDisposition(*remembered,"Sleep")
                    -learnedActionDisposition(actor,"Sleep"))<1e-12);
    return 0;
}
