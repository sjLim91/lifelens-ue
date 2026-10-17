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

    // An accepted AI food request after three actual failed attempts must
    // not blindly force the same gather candidate over ordinary Core plans.
    // When a genuine exploration affordance exists for the SAME resource,
    // the resident seeks alternatives. Otherwise ordinary Core decides.
    const auto originalFood=chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought);
    assert(originalFood.has_value());
    assert(originalFood->intent==CivilizationIntent::Gather);
    for(int tick=477;tick<=479;++tick)
        rememberCivilizationAttemptOutcome(actor,*originalFood,tick,false);
    assert(residentAIHasRecentRepeatedFailure(
        actor,*originalFood,world.minute));
    const auto attemptedAgain=chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought);
    const auto trueExploration=bestResourceExplorationDecisionAtPosition(
        world,actor,{0,0},nullptr,MaterialKind::PlantFood);
    if(trueExploration.intent==CivilizationIntent::Explore
       &&trueExploration.utility>=0.14){
        assert(attemptedAgain.has_value());
        assert(attemptedAgain->intent==CivilizationIntent::Explore);
        assert(attemptedAgain->material==MaterialKind::PlantFood);
    }else{
        assert(!attemptedAgain.has_value());
    }
    // Another resident cannot inherit someone else's failed history.
    Character fresh=actor;
    fresh.id=10;
    fresh.civilization.character=fresh.id;
    fresh.memory=MemoryState{};
    fresh.beliefs=BeliefState{};
    AcceptedCognitiveDecision freshThought=thought;
    freshThought.actor=fresh.id;
    const auto independent=chooseResidentAIExistingCivilizationAffordance(
        world,fresh,{0,0},freshThought);
    assert(independent.has_value()
           &&independent->intent==CivilizationIntent::Gather);

    // A truly successful retry supersedes stale failures immediately.
    rememberCivilizationAttemptOutcome(actor,*originalFood,480,true);
    assert(!residentAIHasRecentRepeatedFailure(
        actor,*originalFood,world.minute));
    const auto recovered=chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought);
    assert(recovered.has_value()
           &&recovered->intent==CivilizationIntent::Gather);
    // The changed choice is learned from existing snapshots, never a new
    // ephemeral actor state or hidden RNG decision.
    World tomorrow=world;
    tomorrow.minute=2000;
    assert(!residentAIHasRecentRepeatedFailure(
        actor,*originalFood,tomorrow.minute));

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
