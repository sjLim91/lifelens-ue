#include <cassert>
#include <limits>
#include "lifelens/ResidentAIExistingAffordance.h"
#include "lifelens/ResidentAICompletedPlanGuard.h"
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

    thought.intent=CognitiveIntentKind::CraftUsefulTools;
    // Without technology/ingredients, a thought is NOT a crafted item.
    const auto craft=chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought);
    if(craft) {
        // A personally chosen tool may legitimately start by gathering
        // actual ingredients or experimenting with an existing Core recipe.
        assert(craft->intent==CivilizationIntent::Craft
            ||craft->intent==CivilizationIntent::Gather
            ||craft->intent==CivilizationIntent::Experiment);
        if(craft->intent==CivilizationIntent::Craft){
            assert(craft->facilityAction==FacilityBuildAction::None);
            assert(craft->technique!=TechniqueId::None);
        }
        if(craft->intent==CivilizationIntent::Gather)
            assert(craft->resourceNode!=0);
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
    world.minute=ResidentAgencyGoalHorizonMinutes+1;
    assert(!chooseResidentAIExistingCivilizationAffordance(
        world,actor,{0,0},thought));
    world.minute=480;

    // A water request cannot harvest water without a free vessel; if the
    // resident already knows the SimpleContainer craft and holds real clay,
    // the SAME accepted AI intent can become a valid Core crafting plan.
    World waterWorld(606002);
    waterWorld.minute=480;
    waterWorld.resourceNodes.clear();
    ResourceNode waterSupply;
    waterSupply.id=409;
    waterSupply.material=MaterialKind::Water;
    waterSupply.quantity=20;
    waterSupply.maxQuantity=20;
    waterSupply.pos=waterWorld.initialStartRegionCenterGrid();
    waterWorld.resourceNodes.push_back(waterSupply);
    Character potter;
    potter.id=77;
    potter.alive=true;
    potter.lifeStage=LifeStage::Adult;
    potter.civilization.character=potter.id;
    potter.needs.thirst=0.75;
    potter.civilization.knowledge.learn(
        TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible,1.0);
    potter.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Clay,3,0.5,1.0
    });
    AcceptedCognitiveDecision waterThought;
    waterThought.actor=potter.id;
    waterThought.intent=CognitiveIntentKind::ImproveWaterSecurity;
    waterThought.minute=480;
    waterThought.priority=0.90;
    const GridPos waterPosition=waterSupply.pos;

    assert(emptySimpleContainerCount(potter.civilization.inventory)==0);
    const auto unavailableWater=bestGatherDecisionAtPosition(
        waterWorld,potter,waterPosition,nullptr,MaterialKind::Water);
    assert(unavailableWater.intent==CivilizationIntent::None);
    const auto realVessel=chooseResidentAIExistingCivilizationAffordance(
        waterWorld,potter,waterPosition,waterThought);
    assert(realVessel);
    assert(realVessel->intent==CivilizationIntent::Craft);
    assert(realVessel->technique==TechniqueId::SimpleContainer);
    assert(realVessel->facilityAction==FacilityBuildAction::None);
    assert(waterWorld.resourceNodes.front().quantity==20);
    assert(potter.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Clay)==3);

    // A filter left unset must leave the original craft chooser unchanged.
    const auto craftDefault=bestCraftDecisionAtPosition(
        waterWorld,potter,waterPosition);
    const auto craftUnfiltered=bestCraftDecisionAtPosition(
        waterWorld,potter,waterPosition,nullptr,TechniqueId::None);
    assert(craftDefault.intent==craftUnfiltered.intent);
    assert(craftDefault.technique==craftUnfiltered.technique);
    assert(craftDefault.utility==craftUnfiltered.utility);

    Character noClay=potter;
    noClay.civilization.inventory=Inventory{};
    assert(!chooseResidentAIExistingCivilizationAffordance(
        waterWorld,noClay,waterPosition,waterThought));

    // Real multi-step goal: water needs a vessel; a known vessel recipe
    // needs real clay, so gather only an actual reachable clay node.
    World withClay=waterWorld;
    ResourceNode clayNode;
    clayNode.id=410;
    clayNode.material=MaterialKind::Clay;
    clayNode.pos=waterPosition;
    clayNode.quantity=12;
    clayNode.maxQuantity=12;
    withClay.resourceNodes.push_back(clayNode);
    const auto obtainClay=chooseResidentAIExistingCivilizationAffordance(
        withClay,noClay,waterPosition,waterThought);
    assert(obtainClay);
    assert(obtainClay->intent==CivilizationIntent::Gather);
    assert(obtainClay->material==MaterialKind::Clay);
    assert(obtainClay->resourceNode==clayNode.id);
    assert(obtainClay->quantity>0);
    assert(noClay.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Clay)==0);
    assert(withClay.resourceNodes.back().quantity==12);

    // A three-stage chain uses three separate real-world states, never a
    // single model-issued "create container and water" side effect.
    Character withIngredients=noClay;
    withIngredients.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Clay,3,0.5,1.0
    });
    const auto nowCraft=chooseResidentAIExistingCivilizationAffordance(
        withClay,withIngredients,waterPosition,waterThought);
    assert(nowCraft &&nowCraft->intent==CivilizationIntent::Craft);
    assert(nowCraft->technique==TechniqueId::SimpleContainer);
    assert(emptySimpleContainerCount(withIngredients.civilization.inventory)==0);

    Character withActualVessel=noClay;
    withActualVessel.civilization.inventory.add({
        ItemKind::SimpleContainer,MaterialKind::Clay,1,0.5,1.0
    });
    assert(emptySimpleContainerCount(withActualVessel.civilization.inventory)==1);
    const auto nowGatherWater=chooseResidentAIExistingCivilizationAffordance(
        withClay,withActualVessel,waterPosition,waterThought);
    assert(nowGatherWater &&nowGatherWater->intent==CivilizationIntent::Gather);
    assert(nowGatherWater->material==MaterialKind::Water);
    assert(nowGatherWater->resourceNode==waterSupply.id);
    assert(withClay.resourceNodes.front().quantity==20);

    // One accepted strategy persists across many physical activity slots.
    // Real clay -> real known recipe -> actual transport vessel -> real water.
    // No additional model turn, imaginary material, or inventory mutation.
    World later=withClay;
    later.minute=1200; // 12 hours after the same personal thought
    const auto continuedCraft=chooseResidentAIExistingCivilizationAffordance(
        later,withIngredients,waterPosition,waterThought);
    assert(continuedCraft &&continuedCraft->intent==CivilizationIntent::Craft);
    assert(continuedCraft->technique==TechniqueId::SimpleContainer);
    const auto continuedWater=chooseResidentAIExistingCivilizationAffordance(
        later,withActualVessel,waterPosition,waterThought);
    assert(continuedWater &&continuedWater->intent==CivilizationIntent::Gather);
    assert(continuedWater->material==MaterialKind::Water);
    assert(later.resourceNodes.front().quantity==20);
    // Stale cognition never remains a perpetual command.
    later.minute=waterThought.minute+ResidentAgencyGoalHorizonMinutes+1;
    assert(!chooseResidentAIExistingCivilizationAffordance(
        later,withActualVessel,waterPosition,waterThought));

    // A four-stage Core-only dependency chain also works when the
    // resident has NOT learned to craft a water vessel yet:
    // real clay -> probabilistic ShapeClay experiment -> known recipe craft
    // -> real water gathered with the resulting empty vessel.
    Character noTechNoClay=potter;
    noTechNoClay.civilization.knowledge=KnowledgeState{};
    noTechNoClay.civilization.inventory=Inventory{};
    const auto collectExperimentClay=chooseResidentAIExistingCivilizationAffordance(
        withClay,noTechNoClay,waterPosition,waterThought);
    assert(collectExperimentClay);
    assert(collectExperimentClay->intent==CivilizationIntent::Gather);
    assert(collectExperimentClay->material==MaterialKind::Clay);
    assert(collectExperimentClay->resourceNode==clayNode.id);
    assert(noTechNoClay.civilization.knowledge.level(
        TechniqueId::SimpleContainer)==KnowledgeLevel::Unknown);

    Character noviceWithClay=noTechNoClay;
    noviceWithClay.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Clay,3,0.5,1.0
    });
    const auto actualExperiment=chooseResidentAIExistingCivilizationAffordance(
        withClay,noviceWithClay,waterPosition,waterThought);
    assert(actualExperiment);
    assert(actualExperiment->intent==CivilizationIntent::Experiment);
    assert(actualExperiment->experiment==ExperimentKind::ShapeClay);
    assert(actualExperiment->technique==TechniqueId::SimpleContainer);
    assert(noviceWithClay.civilization.knowledge.level(
        TechniqueId::SimpleContainer)==KnowledgeLevel::Unknown);
    assert(noviceWithClay.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Clay)==3);
    // Optional filtering must preserve the original unfiltered chooser.
    const auto experimentDefault=bestExperimentDecisionAtPosition(
        withClay,noviceWithClay,waterPosition);
    const auto experimentUnfiltered=bestExperimentDecisionAtPosition(
        withClay,noviceWithClay,waterPosition,nullptr,TechniqueId::None);
    assert(experimentDefault.intent==experimentUnfiltered.intent);
    assert(experimentDefault.experiment==experimentUnfiltered.experiment);
    assert(experimentDefault.utility==experimentUnfiltered.utility);

    World noWater=withClay;
    noWater.resourceNodes.front().quantity=0;
    assert(!chooseResidentAIExistingCivilizationAffordance(
        noWater,noClay,waterPosition,waterThought));
    assert(!chooseResidentAIExistingCivilizationAffordance(
        noWater,noTechNoClay,waterPosition,waterThought));
    assert(!chooseResidentAIExistingCivilizationAffordance(
        noWater,noviceWithClay,waterPosition,waterThought));

    Character untrained=potter;
    untrained.civilization.knowledge=KnowledgeState{};
    assert(!chooseResidentAIExistingCivilizationAffordance(
        waterWorld,noTechNoClay,waterPosition,waterThought));
    const auto learnedExperiment=chooseResidentAIExistingCivilizationAffordance(
        waterWorld,untrained,waterPosition,waterThought);
    assert(learnedExperiment);
    assert(learnedExperiment->intent==CivilizationIntent::Experiment);
    assert(learnedExperiment->experiment==ExperimentKind::ShapeClay);
    assert(learnedExperiment->technique==TechniqueId::SimpleContainer);

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
    const CivilizationUtilityDecision executedDecision=sim.captureSnapshot()
        .runtime.at(self).pendingContext.civilization;
    const std::string gatherKey="Civilization:Gather:PlantFood";
    const auto hasFirsthandOutcome=[&](const Character& resident,
                                       const char* outcome){
        for(const MemoryRecord& memory:resident.memory.entries){
            if(memory.tags.size()==3 &&memory.tags[0]=="action_experience"
               &&memory.tags[1]==gatherKey &&memory.tags[2]==outcome
               &&memory.source==MemorySource::DirectWitness
               &&memory.witnessed &&memory.sourceCharacter==resident.id)
                return true;
        }
        return false;
    };
    assert(!hasFirsthandOutcome(sim.world().characters.front(),"success"));
    assert(!hasFirsthandOutcome(sim.world().characters.front(),"failure"));
    const auto& accepted=sim.acceptedCognitiveDecisions().back();
    assert(!residentAIPlanAlreadyAttempted(
        sim.world().characters.front(),accepted,executedDecision,
        sim.world().minute));

    // The authorized AI choice alone is not an action: only completing the
    // pending Core token can alter the resource and firsthand evidence.
    // This fixture intentionally uses an injected test resource, so it is
    // not a valid production snapshot world and must not be restore-tested.
    const int foodBefore=sim.world().characters.front().civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood);
    sim.setExternalPhysicalExecution(true);
    assert(sim.completeExternalContextAction(
        self,pending.token,pending.targetPos));

    // Success is a Core Gather result, not a claim in the local-model text.
    const Character& succeeded=sim.world().characters.front();
    assert(sim.world().resourceNodes.front().quantity<40);
    assert(succeeded.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)>foodBefore);
    assert(hasFirsthandOutcome(succeeded,"success"));
    const BeliefRecord* successBelief=succeeded.beliefs.find(
        0,actionExperienceProposition(gatherKey.c_str()));
    assert(successBelief!=nullptr &&successBelief->supportCount>0);
    assert(residentAIPlanAlreadyAttempted(
        succeeded,accepted,executedDecision,sim.world().minute));
    // A subsequent independent inference may legitimately choose Gather again.
    AcceptedCognitiveDecision fresh=accepted;
    fresh.minute=sim.world().minute;
    assert(!residentAIPlanAlreadyAttempted(
        succeeded,fresh,executedDecision,sim.world().minute));

    // Snapshot persistence is separately covered by the native live AI and
    // snapshot-codec tests using an unmodified, valid NEW GAME world.
    const CognitiveRequest nextRequest=sim.observeCognitiveRequest(
        self,CognitiveTriggerKind::Reflection);
    bool modelCanReadSuccess=false;
    for(const auto& memory:nextRequest.memories){
        if(memory.tags.size()==3 &&memory.tags[0]=="action_experience"
           &&memory.tags[1]==gatherKey &&memory.tags[2]=="success"
           &&memory.source==MemorySource::DirectWitness &&memory.witnessed)
            modelCanReadSuccess=true;
    }
    assert(modelCanReadSuccess);

    // Independent NEW GAME under the same seed and physical fixture:
    // if another process exhausts the node AFTER planning but BEFORE work,
    // Core must not invent food and must record the firsthand failure.
    Simulation exhausted(99551);
    exhausted.setupNewGame();
    assert(exhausted.world().characters.front().id==self);
    GridPos failedPosition{};
    assert(exhausted.runtimePosition(self,failedPosition));
    for(Character& resident:exhausted.world().characters)
        resident.needs={0.01,0.01,0.01,0.01,0.01};
    ResourceNode exhaustedNode=actualFood;
    exhaustedNode.pos=failedPosition;
    exhausted.world().resourceNodes.clear();
    exhausted.world().resourceNodes.push_back(exhaustedNode);
    const CognitiveRequest failedPlanRequest=exhausted.observeCognitiveRequest(
        self,CognitiveTriggerKind::Reflection);
    assert(exhausted.acceptCognitiveProposal(
        failedPlanRequest,plan,"local-test-model","resource-001"));
    exhausted.step();
    const auto failingPending=exhausted.observePendingContextAction(self);
    assert(failingPending.active);
    assert(failingPending.kind==ContextActionKind::Civilization);
    assert(failingPending.civilizationIntent==CivilizationIntent::Gather);
    assert(failingPending.resourceNode==actualFood.id);
    const CivilizationUtilityDecision failedDecision=exhausted.captureSnapshot()
        .runtime.at(self).pendingContext.civilization;
    assert(exhausted.world().resourceNodes.front().quantity==40);
    exhausted.world().resourceNodes.front().quantity=0;
    exhausted.setExternalPhysicalExecution(true);
    assert(!exhausted.completeExternalContextAction(
        self,failingPending.token,failingPending.targetPos));
    const Character& failed=exhausted.world().characters.front();
    assert(exhausted.world().resourceNodes.front().quantity==0);
    assert(failed.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==foodBefore);
    assert(!hasFirsthandOutcome(failed,"success"));
    assert(hasFirsthandOutcome(failed,"failure"));
    const BeliefRecord* failureBelief=failed.beliefs.find(
        0,actionExperienceProposition(gatherKey.c_str()));
    assert(failureBelief!=nullptr &&failureBelief->contradictionCount>0);
    bool modelCanReadFailureCause=false;
    const CognitiveRequest failedRequest=exhausted.observeCognitiveRequest(
        self,CognitiveTriggerKind::Reflection);
    for(const auto& memory:failedRequest.memories){
        if(memory.tags.size()==3 &&memory.tags[0]=="action_experience"
           &&memory.tags[1]==gatherKey &&memory.tags[2]=="failure"
           &&memory.source==MemorySource::DirectWitness &&memory.witnessed
           &&memory.what.find("source-exhausted")!=std::string::npos)
            modelCanReadFailureCause=true;
    }
    assert(modelCanReadFailureCause);
    assert(residentAIPlanAlreadyAttempted(
        failed,exhausted.acceptedCognitiveDecisions().back(),
        failedDecision,exhausted.world().minute));
    return 0;
}
