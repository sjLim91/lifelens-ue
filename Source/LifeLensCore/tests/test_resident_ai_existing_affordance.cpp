#include <cassert>
#include <limits>
#include "lifelens/ResidentAIExistingAffordance.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

int main()
{
    World world(993);
    world.minute=480;
    Character actor;
    actor.id=9;
    actor.alive=true;
    actor.civilization.character=actor.id;
    actor.needs={0.10,0.10,0.10,0.10,0.10};

    ResourceNode stone;
    stone.id=100;
    stone.material=MaterialKind::Stone;
    stone.quantity=20;
    stone.maxQuantity=20;
    stone.pos={0,0};

    ResourceNode plantFood=stone;
    plantFood.id=101;
    plantFood.material=MaterialKind::PlantFood;

    world.resourceNodes={stone,plantFood};
    AcceptedCognitiveDecision thought;
    thought.actor=9;
    thought.minute=480;
    thought.priority=0.80;
    thought.intent=CognitiveIntentKind::ImproveFoodSecurity;

    const auto food=chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought);
    assert(food.has_value());
    assert(food->intent==CivilizationIntent::Gather);
    assert(food->material==MaterialKind::PlantFood);
    assert(food->resourceNode==101);
    assert(food->quantity>0);
    // The underlying unfiltered Core chooser is unchanged.
    const auto baseline=bestGatherDecisionAtPosition(world,actor,{0,0});
    const auto defaultFilter=bestGatherDecisionAtPosition(
        world,actor,{0,0},nullptr,MaterialKind::Unknown);
    assert(baseline.intent==defaultFilter.intent);
    assert(baseline.material==defaultFilter.material);
    assert(baseline.resourceNode==defaultFilter.resourceNode);
    assert(baseline.utility==defaultFilter.utility);

    thought.intent=CognitiveIntentKind::ImproveWaterSecurity;
    assert(!chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought));
    ResourceNode water=stone;
    water.id=102;
    water.material=MaterialKind::Water;
    world.resourceNodes.push_back(water);
    // No portable container => not even a candidate, never free water.
    assert(!chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought));

    thought.intent=CognitiveIntentKind::AcquireMaterials;
    const auto anyMaterial=chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought);
    assert(anyMaterial.has_value());
    assert(anyMaterial->intent==CivilizationIntent::Gather);
    assert(anyMaterial->resourceNode!=0);

    // A model cannot force the same FAILED gathering method forever.
    // Before REAL attempts are observed, the model's choice stays intact.
    thought.intent=CognitiveIntentKind::ImproveFoodSecurity;
    world.minute=510;
    thought.minute=510;
    assert(chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought)->intent==CivilizationIntent::Gather);
    CivilizationUtilityDecision failedFood;
    failedFood.intent=CivilizationIntent::Gather;
    failedFood.material=MaterialKind::PlantFood;
    rememberCivilizationAttemptOutcome(actor,failedFood,501,false);
    // One failure is not enough to conclude a strategy is unproductive.
    assert(chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought)->intent==CivilizationIntent::Gather);
    rememberCivilizationAttemptOutcome(actor,failedFood,502,false);
    assert(residentAIRecentRepeatedActionFailure(actor,failedFood,510));
    const auto reconsidered=chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought);
    // The only permitted substitute is a REAL Core exploration candidate;
    // when exploration is impossible, the normal planner owns the next slot.
    assert(!reconsidered
           ||reconsidered->intent==CivilizationIntent::Explore);
    assert(!reconsidered
           ||reconsidered->resourceNode!=plantFood.id);

    // Other residents do not inherit the first actor's private failures.
    Character newcomer=actor;
    newcomer.id=77;
    newcomer.civilization.character=newcomer.id;
    newcomer.memory=MemoryState{};
    newcomer.beliefs=BeliefState{};
    AcceptedCognitiveDecision newThought=thought;
    newThought.actor=newcomer.id;
    const auto independent=chooseResidentAIExistingCivilizationAffordance(
        world,newcomer,{0,0},newThought);
    assert(independent &&independent->intent==CivilizationIntent::Gather);
    assert(independent->material==MaterialKind::PlantFood);

    // A verified success can reverse the soft deterrent, while neither a
    // fabricated recollection nor a stale failure persists forever.
    rememberCivilizationAttemptOutcome(actor,failedFood,503,true);
    assert(!residentAIRecentRepeatedActionFailure(actor,failedFood,510));
    const auto recovered=chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought);
    assert(recovered &&recovered->intent==CivilizationIntent::Gather);
    world.minute=660;
    thought.minute=660;
    assert(!residentAIRecentRepeatedActionFailure(actor,failedFood,660));
    const auto afterTime=chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought);
    assert(afterTime &&afterTime->intent==CivilizationIntent::Gather);

    // Restore a neutral test baseline for the existing contract assertions.
    world.minute=480;
    thought.minute=480;
    actor.memory=MemoryState{};
    actor.beliefs=BeliefState{};

    thought.intent=CognitiveIntentKind::CraftUsefulTools;
    // Without technology/ingredients, a thought is NOT a crafted item.
    const auto craft=chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought);
    if(craft) {
        assert(craft->intent==CivilizationIntent::Craft);
        assert(craft->facilityAction==FacilityBuildAction::None);
        assert(craft->technique!=TechniqueId::None);
    }

    thought.intent=CognitiveIntentKind::ImproveFoodSecurity;
    thought.priority=0.1;
    assert(!chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought));
    thought.priority=std::numeric_limits<double>::quiet_NaN();
    assert(!chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought));
    thought.priority=std::numeric_limits<double>::infinity();
    assert(!chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought));
    thought.priority=0.9;
    thought.actor=1234;
    assert(!chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought));
    thought.actor=actor.id;
    actor.alive=false;
    assert(!chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought));
    actor.alive=true;
    thought.minute=0;
    assert(!chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought));

    // True NEW GAME Core execution: a resident thought cannot mutate the
    // resource or invent a harvest, but it can select a legal gathering
    // context in the ordinary 15-minute planning slot.
    Simulation sim(99551);
    sim.setupNewGame();
    assert(sim.world().minute==480);
    assert(!sim.world().characters.empty());
    const CharacterId self=sim.world().characters.front().id;
    GridPos actorPosition{};
    assert(sim.runtimePosition(self,actorPosition));
    for(Character& resident:sim.world().characters)
        resident.needs={0.01,0.01,0.01,0.01,0.01};
    ResourceNode actualFood;
    actualFood.id=987654321;
    actualFood.material=MaterialKind::PlantFood;
    actualFood.pos=actorPosition;
    actualFood.quantity=40;
    actualFood.maxQuantity=40;
    sim.world().resourceNodes.clear();
    sim.world().resourceNodes.push_back(actualFood);
    const auto request=sim.observeCognitiveRequest(
        self,CognitiveTriggerKind::Reflection);
    CognitiveProposal plan;
    plan.actor=self;
    plan.intent=CognitiveIntentKind::ImproveFoodSecurity;
    plan.priority=0.90;
    assert(sim.acceptCognitiveProposal(
        request,plan,"local-test-model","resource-001"));
    sim.step();
    const auto pending=sim.observePendingContextAction(self);
    assert(pending.active);
    assert(pending.kind==ContextActionKind::Civilization);
    assert(pending.civilizationIntent==CivilizationIntent::Gather);
    assert(pending.resourceNode==actualFood.id);
    assert(pending.material==MaterialKind::PlantFood);
    // Issuing a thought/plan is NOT a successful action commit.
    assert(sim.world().resourceNodes.front().quantity==40);
    return 0;
}
