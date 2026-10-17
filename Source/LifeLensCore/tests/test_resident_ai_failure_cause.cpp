#include <cassert>
#include <string>
#include <vector>

#include "lifelens/ResidentCivilizationOutcomeLearning.h"
#include "lifelens/CognitiveAgent.h"

using namespace lifelens;

int main()
{
    World world(606002);
    world.minute=480;
    Character actor;
    actor.id=9;
    actor.alive=true;
    actor.civilization.character=actor.id;
    actor.lifeStage=LifeStage::Adult;

    CivilizationUtilityDecision request;
    request.intent=CivilizationIntent::Gather;
    request.material=MaterialKind::PlantFood;
    request.resourceNode=101;
    request.quantity=2;
    const GridPos camp=world.initialStartRegionCenterGrid();

    // Classification is read-only: it cannot create resources or evidence.
    world.resourceNodes.clear();
    assert(diagnoseResidentGatherFailure(world,actor,request,camp)
        ==ResidentGatherFailureCause::SourceRemoved);
    assert(actor.memory.entries.empty());

    ResourceNode food;
    food.id=request.resourceNode;
    food.material=MaterialKind::PlantFood;
    food.pos=camp;
    food.quantity=0;
    food.maxQuantity=12;
    world.resourceNodes.push_back(food);
    assert(diagnoseResidentGatherFailure(world,actor,request,camp)
        ==ResidentGatherFailureCause::SourceExhausted);
    assert(world.resourceNodes[0].quantity==0);

    // Only an authoritative real attempt records the failure. The proposed
    // action by itself cannot generate this evidence.
    rememberCivilizationAttemptOutcome(actor,request,480,false,
        diagnoseResidentGatherFailure(world,actor,request,camp));
    const std::string key=civilizationActionExperienceKey(request);
    assert(key=="Civilization:Gather:PlantFood");
    const BeliefRecord* belief=actor.beliefs.find(
        0,actionExperienceProposition(key.c_str()));
    assert(belief!=nullptr &&belief->contradictionCount==1);
    bool witnessed=false;
    for(const auto& memory:actor.memory.entries){
        if(memory.tags.size()==3
           &&memory.tags[0]=="action_experience"
           &&memory.tags[1]==key
           &&memory.tags[2]=="failure"){
            assert(memory.source==MemorySource::DirectWitness);
            assert(memory.witnessed);
            assert(memory.what.find("source-exhausted")!=std::string::npos);
            witnessed=true;
        }
    }
    assert(witnessed);
    RelationshipBook relationships;
    const auto cognitive=buildCognitiveRequest(
        actor,relationships,480,CognitiveTriggerKind::Reflection,
        {CognitiveIntentKind::ImproveFoodSecurity},8,8,8);
    bool visible=false;
    for(const auto& memory:cognitive.memories){
        if(memory.source==MemorySource::DirectWitness
           &&memory.what.find("source-exhausted")!=std::string::npos)
            visible=true;
    }
    assert(visible);

    // If water exists but no empty container is held, do not infer absence
    // of water. This is a logistics failure, not world resource depletion.
    world.resourceNodes[0].material=MaterialKind::Water;
    world.resourceNodes[0].quantity=10;
    request.material=MaterialKind::Water;
    assert(diagnoseResidentGatherFailure(world,actor,request,camp)
        ==ResidentGatherFailureCause::ContainerMissing);
    assert(world.resourceNodes[0].quantity==10);
    assert(actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Water)==0);

    // Unknown actor action or a mismatched material cannot be interpreted
    // as a trusted observation of a particular missing resource.
    request.material=MaterialKind::Stone;
    assert(diagnoseResidentGatherFailure(world,actor,request,camp)
        ==ResidentGatherFailureCause::Unknown);
    request.intent=CivilizationIntent::Explore;
    assert(diagnoseResidentGatherFailure(world,actor,request,camp)
        ==ResidentGatherFailureCause::Unknown);
    request.intent=CivilizationIntent::Gather;
    request.material=MaterialKind::PlantFood;
    world.resourceNodes[0].material=MaterialKind::PlantFood;
    assert(diagnoseResidentGatherFailure(
        world,actor,request,{camp.x+999,camp.y+999})
        ==ResidentGatherFailureCause::ResourceAccessUnavailable);

    // Success must not acquire the vocabulary of failure causes.
    rememberCivilizationAttemptOutcome(actor,request,481,true);
    for(const auto& memory:actor.memory.entries){
        if(memory.tags.size()==3 &&memory.tags[2]=="success")
            assert(memory.what.find("원인:")==std::string::npos);
    }
    return 0;
}
