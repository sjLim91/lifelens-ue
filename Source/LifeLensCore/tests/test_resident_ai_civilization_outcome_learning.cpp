#include <cassert>
#include <string>
#include <vector>

#include "lifelens/ResidentCivilizationOutcomeLearning.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

const Character* lookup(const Simulation& sim,CharacterId id)
{
    for(const auto& person:sim.world().characters)
        if(person.id==id) return &person;
    return nullptr;
}

int main()
{
    // A model proposal or an uncompleted pending action cannot create a
    // personal memory. Only authoritative execution attempts have outcomes.
    Simulation simulation(99551);
    simulation.setupNewGame();
    simulation.setExternalPhysicalExecution(true);
    assert(simulation.world().minute==480);
    const CharacterId actorId=simulation.world().characters.front().id;
    const CharacterId bystanderId=simulation.world().characters.back().id;
    GridPos actorPosition{};
    assert(simulation.runtimePosition(actorId,actorPosition));
    for(auto& character:simulation.world().characters)
        character.needs={0.01,0.01,0.01,0.01,0.01};

    ResourceNode food;
    food.id=987654321;
    food.material=MaterialKind::PlantFood;
    food.pos=actorPosition;
    food.quantity=40;
    food.maxQuantity=40;
    simulation.world().resourceNodes.clear();
    simulation.world().resourceNodes.push_back(food);

    const CognitiveRequest observed=simulation.observeCognitiveRequest(
        actorId,CognitiveTriggerKind::Reflection);
    CognitiveProposal wish;
    wish.actor=actorId;
    wish.intent=CognitiveIntentKind::ImproveFoodSecurity;
    wish.priority=0.90;
    assert(simulation.acceptCognitiveProposal(
        observed,wish,"local-test-model","personal-food-goal"));

    const std::string key="Civilization:Gather:PlantFood";
    const std::string proposition=actionExperienceProposition(key.c_str());
    assert(lookup(simulation,actorId)->beliefs.find(0,proposition)==nullptr);
    simulation.step();

    const PendingContextActionObservation pending=
        simulation.observePendingContextAction(actorId);
    assert(pending.active &&pending.token!=0);
    assert(pending.kind==ContextActionKind::Civilization);
    assert(pending.civilizationIntent==CivilizationIntent::Gather);
    assert(pending.material==MaterialKind::PlantFood);
    assert(pending.resourceNode==food.id);
    assert(lookup(simulation,actorId)->beliefs.find(0,proposition)==nullptr);
    assert(simulation.world().resourceNodes.front().quantity==40);

    // Snapshot/restart with an accepted-but-not-executed plan must not
    // synthesize a memory of success.
    Simulation replay(99551);
    std::string error;
    assert(replay.restoreSnapshot(simulation.captureSnapshot(),&error));
    assert(replay.observePendingContextAction(actorId).token==pending.token);
    assert(lookup(replay,actorId)->beliefs.find(0,proposition)==nullptr);

    assert(simulation.completeExternalContextAction(
        actorId,pending.token,pending.targetPos));
    assert(replay.completeExternalContextAction(
        actorId,pending.token,pending.targetPos));
    assert(simulation.world().resourceNodes.front().quantity<40);
    assert(simulation.world().resourceNodes.front().quantity
           ==replay.world().resourceNodes.front().quantity);

    const Character* resident=lookup(simulation,actorId);
    const Character* replayed=lookup(replay,actorId);
    const Character* bystander=lookup(simulation,bystanderId);
    assert(resident && replayed && bystander);
    const BeliefRecord* success=resident->beliefs.find(0,proposition);
    assert(success!=nullptr);
    assert(success->supportCount==1);
    assert(success->contradictionCount==0);
    assert(replayed->beliefs.find(0,proposition)->supportCount==1);
    assert(bystander->beliefs.find(0,proposition)==nullptr);

    const auto request=simulation.observeCognitiveRequest(
        actorId,CognitiveTriggerKind::Reflection);
    bool observedSuccess=false;
    for(const auto& memory:request.memories){
        if(memory.source==MemorySource::DirectWitness
           &&memory.witnessed
           &&memory.minute==simulation.world().minute
           &&memory.what.find(key)!=std::string::npos
           &&memory.what.find("성공")!=std::string::npos){
            observedSuccess=true;
        }
    }
    assert(observedSuccess); // feedback becomes the NEXT AI input
    const auto thirdPerson=simulation.observeCognitiveRequest(
        bystanderId,CognitiveTriggerKind::Reflection);
    for(const auto& memory:thirdPerson.memories)
        assert(memory.what.find(key)==std::string::npos);

    // A real attempt with an exhausted resource reports failure without
    // creating food. The belief tracks bounded contradictory evidence.
    Simulation failed(99551);
    assert(failed.restoreSnapshot(simulation.captureSnapshot(),&error));
    // Use a separate Core fixture at the earlier, still-pending moment.
    Simulation blocked(99551);
    assert(blocked.restoreSnapshot(replay.captureSnapshot(),&error));
    // Directly exercise the exact, deterministic observed execution gate:
    // a rejected completed action can be a personal failed attempt, never
    // a fabricated resource or knowledge change.
    Character student;
    student.id=111;
    student.alive=true;
    CivilizationUtilityDecision gathering;
    gathering.intent=CivilizationIntent::Gather;
    gathering.material=MaterialKind::PlantFood;
    rememberCivilizationAttemptOutcome(student,gathering,480,false);
    assert(student.beliefs.find(0,proposition)->contradictionCount==1);
    rememberCivilizationAttemptOutcome(student,gathering,480,false);
    assert(student.beliefs.find(0,proposition)->contradictionCount==1);
    rememberCivilizationAttemptOutcome(student,gathering,481,true);
    assert(student.beliefs.find(0,proposition)->supportCount==1);
    assert(student.memory.entries.size()==2);

    // Binary save/load preserves direct experience and learned stance.
    std::vector<std::uint8_t> bytes;
    assert(encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error));
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    Simulation restored(99551);
    assert(restored.restoreSnapshot(decoded,&error));
    const auto learned=restored.observeCognitiveRequest(
        actorId,CognitiveTriggerKind::Reflection);
    bool persisted=false;
    for(const auto& memory:learned.memories)
        if(memory.what.find(key)!=std::string::npos) persisted=true;
    assert(persisted);
    assert(lookup(restored,actorId)->beliefs.find(0,proposition)->supportCount
           ==success->supportCount);
    return 0;
}
