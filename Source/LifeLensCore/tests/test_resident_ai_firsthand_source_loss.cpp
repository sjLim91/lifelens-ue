#include <cassert>
#include <string>

#include "lifelens/ResidentAIExistingAffordance.h"

using namespace lifelens;

int main()
{
    World world(606002);
    world.minute=480;
    const GridPos camp=world.initialStartRegionCenterGrid();
    world.resourceNodes.clear();

    Character actor;
    actor.id=9;
    actor.alive=true;
    actor.lifeStage=LifeStage::Adult;
    actor.civilization.character=actor.id;
    actor.needs={0.72,0.10,0.10,0.10,0.10};

    ResourceNode food;
    food.id=101;
    food.pos=camp;
    food.material=MaterialKind::PlantFood;
    food.quantity=20;
    food.maxQuantity=20;
    world.resourceNodes.push_back(food);

    AcceptedCognitiveDecision wish;
    wish.actor=actor.id;
    wish.minute=world.minute;
    wish.priority=0.9;
    wish.intent=CognitiveIntentKind::ImproveFoodSecurity;
    const auto first=chooseResidentAIExistingCivilizationAffordance(
        world,actor,camp,wish);
    assert(first &&first->intent==CivilizationIntent::Gather);
    assert(first->resourceNode==food.id);

    // The candidate's real node is lost before physical completion. This
    // simulated outcome uses the same authoritative diagnostic and memory
    // writer that ContextActionRuntime invokes only after a real attempt.
    const CivilizationUtilityDecision attempted=*first;
    world.resourceNodes.front().quantity=0;
    const auto cause=diagnoseResidentGatherFailure(
        world,actor,attempted,camp);
    assert(cause==ResidentGatherFailureCause::SourceExhausted);
    rememberCivilizationAttemptOutcome(actor,attempted,480,false,cause);
    assert(actor.beliefs.find(
        0,actionExperienceProposition("Civilization:Gather:PlantFood"))
            ->contradictionCount==1);
    world.minute=481;
    wish.minute=481;
    assert(!residentAIHasRecentRepeatedGatherFailure(
        actor,MaterialKind::PlantFood,world.minute));
    assert(residentAIHasConfirmedLostGatherSource(
        actor,MaterialKind::PlantFood,world.minute));
    const auto retarget=chooseResidentAIExistingCivilizationAffordance(
        world,actor,camp,wish);
    assert(retarget &&retarget->intent==CivilizationIntent::Explore);
    assert(retarget->material==MaterialKind::PlantFood);
    assert(retarget->resourceNode==0);
    assert(world.resourceNodes.front().quantity==0); // no creation

    // If another real food source becomes viable, use that Gather. One
    // exhausted source does not imply all food in the world disappeared.
    ResourceNode alternate=food;
    alternate.id=102;
    alternate.quantity=20;
    world.resourceNodes.push_back(alternate);
    const auto available=chooseResidentAIExistingCivilizationAffordance(
        world,actor,camp,wish);
    assert(available &&available->intent==CivilizationIntent::Gather);
    assert(available->resourceNode==alternate.id);

    // Private direct evidence is not transferable between residents.
    Character bystander=actor;
    bystander.id=10;
    bystander.civilization.character=10;
    assert(!residentAIHasConfirmedLostGatherSource(
        bystander,MaterialKind::PlantFood,world.minute));
    Character hearsay=actor;
    for(auto& memory:hearsay.memory.entries){
        memory.source=MemorySource::ToldByOther;
        memory.witnessed=false;
    }
    assert(!residentAIHasConfirmedLostGatherSource(
        hearsay,MaterialKind::PlantFood,world.minute));
    Character textForgery=actor;
    for(auto& memory:textForgery.memory.entries)
        memory.what="source-exhausted"; // non-Core authored text
    assert(!residentAIHasConfirmedLostGatherSource(
        textForgery,MaterialKind::PlantFood,world.minute));

    // A missing water vessel cannot be read as a depleted water source.
    ResourceNode water=food;
    water.id=103;
    water.material=MaterialKind::Water;
    water.quantity=20;
    CivilizationUtilityDecision waterAttempt=attempted;
    waterAttempt.material=MaterialKind::Water;
    waterAttempt.resourceNode=water.id;
    world.resourceNodes.push_back(water);
    assert(diagnoseResidentGatherFailure(
        world,actor,waterAttempt,camp)
        ==ResidentGatherFailureCause::ContainerMissing);
    rememberCivilizationAttemptOutcome(actor,waterAttempt,481,false,
        ResidentGatherFailureCause::ContainerMissing);
    assert(!residentAIHasConfirmedLostGatherSource(
        actor,MaterialKind::Water,world.minute));

    // Source removal is also direct evidence, but unknown or access
    // problems are NOT certain evidence of depletion.
    Character removed=actor;
    removed.memory=MemoryState{};
    removed.beliefs=BeliefState{};
    world.resourceNodes.clear();
    assert(diagnoseResidentGatherFailure(
        world,removed,attempted,camp)
        ==ResidentGatherFailureCause::SourceRemoved);
    rememberCivilizationAttemptOutcome(removed,attempted,481,false,
        ResidentGatherFailureCause::SourceRemoved);
    assert(residentAIHasConfirmedLostGatherSource(
        removed,MaterialKind::PlantFood,481));
    Character unknown=actor;
    unknown.memory=MemoryState{};
    unknown.beliefs=BeliefState{};
    rememberCivilizationAttemptOutcome(unknown,attempted,481,false);
    assert(!residentAIHasConfirmedLostGatherSource(
        unknown,MaterialKind::PlantFood,481));

    // Fresh firsthand success reverses the negative prior. The outcome
    // memory remains persisted through existing Character storage.
    rememberCivilizationAttemptOutcome(actor,attempted,482,true);
    assert(!residentAIHasConfirmedLostGatherSource(
        actor,MaterialKind::PlantFood,482));
    assert(!residentAIHasConfirmedLostGatherSource(
        removed,MaterialKind::PlantFood,481+1441));
    return 0;
}
