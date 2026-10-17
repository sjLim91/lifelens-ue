#include <cassert>
#include <string>
#include <vector>

#include "lifelens/ResidentAIExistingAffordance.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

int main()
{
    // Use a known traversable seeded region with a real frontier. An empty
    // synthetic ocean-adjacent world does not guarantee an Explore option.
    World world(606002);
    world.minute=480;
    const GridPos camp=world.initialStartRegionCenterGrid();
    world.resourceNodes.clear();
    Character resident;
    resident.id=9;
    resident.alive=true;
    resident.civilization.character=9;
    resident.lifeStage=LifeStage::Adult;
    resident.needs={0.72,0.10,0.10,0.10,0.10};

    ResourceNode food;
    food.id=101;
    food.material=MaterialKind::PlantFood;
    food.quantity=20;
    food.maxQuantity=20;
    food.pos=camp;
    world.resourceNodes.push_back(food);

    AcceptedCognitiveDecision thought;
    thought.actor=9;
    thought.intent=CognitiveIntentKind::ImproveFoodSecurity;
    thought.minute=480;
    thought.priority=0.9;

    const auto original=chooseResidentAIExistingCivilizationAffordance(
        world,resident,camp,thought);
    assert(original && original->intent==CivilizationIntent::Gather);
    assert(original->material==MaterialKind::PlantFood);
    assert(!residentAIHasRecentRepeatedGatherFailure(
        resident,MaterialKind::PlantFood,world.minute));

    CivilizationUtilityDecision gather;
    gather.intent=CivilizationIntent::Gather;
    gather.material=MaterialKind::PlantFood;
    const std::string key=civilizationActionExperienceKey(gather);
    assert(key=="Civilization:Gather:PlantFood");

    for(int i=0;i<2;++i)
        rememberCivilizationAttemptOutcome(resident,gather,480+i,false);
    world.minute=482;
    thought.minute=482;
    assert(!residentAIHasRecentRepeatedGatherFailure(
        resident,MaterialKind::PlantFood,world.minute));
    // Two attempts do not suppress a perfectly real nearby food source.
    const auto twoFailures=chooseResidentAIExistingCivilizationAffordance(
        world,resident,camp,thought);
    assert(twoFailures && twoFailures->intent==CivilizationIntent::Gather);

    rememberCivilizationAttemptOutcome(resident,gather,482,false);
    assert(residentAIHasRecentRepeatedGatherFailure(
        resident,MaterialKind::PlantFood,world.minute));
    // Actual food is STILL present. Core exploration pressure is zero; do
    // not invent a new frontier or block an accessible gather just because
    // prior attempts were bad.
    const auto stillAvailable=chooseResidentAIExistingCivilizationAffordance(
        world,resident,camp,thought);
    assert(stillAvailable &&stillAvailable->intent==CivilizationIntent::Gather);

    // Exhaustion of real nearby food changes the afforded strategy.
    world.resourceNodes.front().quantity=0;
    const auto unfiltered=bestResourceExplorationDecisionAtPosition(
        world,resident,camp);
    const auto explicitlyUnfiltered=bestResourceExplorationDecisionAtPosition(
        world,resident,camp,nullptr,MaterialKind::Unknown);
    assert(unfiltered.intent==explicitlyUnfiltered.intent);
    assert(unfiltered.material==explicitlyUnfiltered.material);
    assert(unfiltered.utility==explicitlyUnfiltered.utility);
    const auto alternative=bestResourceExplorationDecisionAtPosition(
        world,resident,camp,nullptr,MaterialKind::PlantFood);
    assert(alternative.intent==CivilizationIntent::Explore);
    assert(alternative.material==MaterialKind::PlantFood);
    const auto retry=chooseResidentAIExistingCivilizationAffordance(
        world,resident,camp,thought);
    assert(retry &&retry->intent==CivilizationIntent::Explore);
    assert(retry->material==MaterialKind::PlantFood);
    assert(retry->resourceNode==0);

    // A second resident has no private evidence. Hearsay alone does not
    // mutate their Core belief; they cannot acquire the first actor's bias.
    Character bystander=resident;
    bystander.id=10;
    bystander.civilization.character=10;
    bystander.memory=MemoryState{};
    bystander.beliefs=BeliefState{};
    assert(!residentAIHasRecentRepeatedGatherFailure(
        bystander,MaterialKind::PlantFood,world.minute));
    const auto noLearning=chooseResidentAIExistingCivilizationAffordance(
        world,bystander,camp,AcceptedCognitiveDecision{
            10,482,CognitiveTriggerKind::Reflection,1,"local",
            CognitiveIntentKind::ImproveFoodSecurity,0.9,0,"digest"});
    assert(!noLearning.has_value()); // depleted resource, no evidence

    // New firsthand success reverses the repetition penalty.
    rememberCivilizationAttemptOutcome(resident,gather,483,true);
    world.minute=483;
    thought.minute=483;
    assert(!residentAIHasRecentRepeatedGatherFailure(
        resident,MaterialKind::PlantFood,world.minute));

    // An old failure must not drive exploration forever.
    world.minute=1924;
    thought.minute=1924;
    assert(!residentAIHasRecentRepeatedGatherFailure(
        resident,MaterialKind::PlantFood,world.minute));

    // A forged/told-by-other failure without a matching firsthand memory
    // cannot trigger the alternate strategy even with negative belief.
    Character rumor=resident;
    for(auto& m:rumor.memory.entries)
        if(m.tags.size()==3 &&m.tags[2]=="failure"){
            m.source=MemorySource::ToldByOther;
            m.witnessed=false;
        }
    world.minute=483;
    assert(!residentAIHasRecentRepeatedGatherFailure(
        rumor,MaterialKind::PlantFood,world.minute));

    // Snapshot retains the actor's evidence without any new schema.
    Simulation simulation(1442);
    simulation.setupNewGame();
    simulation.runMinutes(3); // latest witnessed outcome is minute 483
    assert(simulation.world().minute==483);
    const auto id=simulation.world().characters.front().id;
    for(auto& c:simulation.world().characters)
        if(c.id==id){
            c.beliefs=resident.beliefs;
            c.memory=resident.memory;
            // The fixture's direct witness (id=9) becomes the actual
            // founder in this authoritative persistence scenario.
            for(auto& memory:c.memory.entries)
                if(memory.sourceCharacter==resident.id)
                    memory.sourceCharacter=id;
        }
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error));
    SimulationStateSnapshot restored;
    assert(decodeSimulationSnapshot(bytes,restored,&error));
    const Character* persisted=nullptr;
    for(const auto& c:restored.world.characters)
        if(c.id==id){persisted=&c;break;}
    assert(persisted);
    assert(persisted->beliefs.find(
        0,actionExperienceProposition(key.c_str()))!=nullptr);
    return 0;
}
