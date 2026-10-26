#include <cassert>
#include <cmath>
#include <iostream>
#include "lifelens/CivilizationDecision.h"

using namespace lifelens;

static Character inventor(MaterialKind head)
{
    Character c;
    c.id=17;
    c.alive=true;
    c.lifeStage=LifeStage::Adult;
    c.civilization.character=c.id;
    c.personality.curiosity=0.95;
    c.personality.openness=0.85;
    c.needs={0.10,0.10,0.10,0.10,0.10};
    c.civilization.inventory.add({ItemKind::RawMaterial,head,2,0.8,1.0});
    c.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,2,0.8,1.0});
    c.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Fiber,2,0.8,1.0});
    return c;
}

int main()
{
    // No technique is predefined for a material combination. Both designs
    // use the same generic Core hypothesis and raw-material accounting.
    Character stone=inventor(MaterialKind::Stone);
    Character flint=inventor(MaterialKind::Flint);
    const auto blunt=proposeMaterialComposition(stone.civilization.inventory);
    const auto sharp=proposeMaterialComposition(flint.civilization.inventory);
    assert(blunt &&sharp);
    assert(blunt->head==MaterialKind::Stone);
    assert(sharp->head==MaterialKind::Flint);
    assert(blunt->resultingTool==ItemKind::StoneHammer);
    assert(sharp->resultingTool==ItemKind::StoneCuttingTool);
    assert(sharp->testedPerformance>blunt->testedPerformance);
    assert(blunt->testedDurability>0.0 &&sharp->testedDurability>0.0);

    World world(5151);
    world.minute=720;
    world.resourceNodes.clear();
    const auto sourceWorld=world;
    CivilizationUtilityDecision decision;
    decision.intent=CivilizationIntent::Experiment;
    decision.experiment=ExperimentKind::ComposeMaterials;
    decision.material=MaterialKind::Flint;
    decision.technique=TechniqueId::None;
    decision.item=ItemKind::StoneCuttingTool;
    decision.utility=0.55;
    Character copy=flint;
    const auto attempt=executeCivilizationDecisionAtPosition(
        world,flint,decision,{0,0});
    auto secondWorld=sourceWorld;
    const auto replay=executeCivilizationDecisionAtPosition(
        secondWorld,copy,decision,{0,0});
    assert(attempt.executed &&replay.executed);
    assert(attempt.success==replay.success);
    assert(attempt.experiment.roll==replay.experiment.roll);
    assert(world.resourceNodes.empty() &&secondWorld.resourceNodes.empty());
    assert(flint.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Flint)==1);
    assert(flint.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood)==1);
    assert(flint.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Fiber)==1);
    assert(!flint.civilization.knowledge.knowsAtLeast(
        TechniqueId::ChippedStoneTool,KnowledgeLevel::Reproducible));
    if(attempt.success){
        assert(attempt.experiment.producedItem);
        assert(flint.civilization.inventory.count(
            ItemKind::StoneCuttingTool,MaterialKind::Flint)==1);
        assert(attempt.experiment.output.quality==sharp->testedPerformance);
        assert(attempt.experiment.output.durability==sharp->testedDurability);
    }else{
        assert(!attempt.experiment.producedItem);
        assert(flint.civilization.inventory.count(
            ItemKind::StoneCuttingTool,MaterialKind::Flint)==0);
    }

    // A proposed material can be forged after the inventor's observation,
    // but the real Core inventory / head choice never honors invented inputs.
    Character forged=inventor(MaterialKind::Stone);
    auto mismatched=decision;
    const auto rejected=executeCivilizationDecisionAtPosition(
        world,forged,mismatched,{0,0});
    assert(!rejected.executed &&!rejected.success);
    assert(forged.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Stone)==2);

    Character missing=inventor(MaterialKind::Flint);
    missing.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Fiber,2);
    assert(!proposeMaterialComposition(missing.civilization.inventory));
    const auto emptyTrial=executeCivilizationDecisionAtPosition(
        world,missing,decision,{0,0});
    assert(!emptyTrial.executed &&!emptyTrial.success);
    assert(missing.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Flint)==2);

    // The automatic utility selector genuinely proposes the physical trial
    // only if the resident owns every required material and is not starving.
    Character autonomous=inventor(MaterialKind::Flint);
    for(const auto known:{
        TechniqueId::SharpFlake,TechniqueId::ChippedStoneTool,
        TechniqueId::FireMaking,TechniqueId::FiberCordage,
        TechniqueId::DiggingStick,TechniqueId::SimpleContainer}){
        autonomous.civilization.knowledge.learn(
            known,KnowledgeLevel::Reproducible,1.0);
    }
    const auto selected=bestExperimentDecisionAtPosition(
        world,autonomous,{0,0});
    assert(selected.intent==CivilizationIntent::Experiment
        &&selected.experiment==ExperimentKind::ComposeMaterials);
    autonomous.needs.thirst=0.90;
    const auto urgent=bestExperimentDecisionAtPosition(
        world,autonomous,{0,0});
    assert(urgent.experiment!=ExperimentKind::ComposeMaterials);
    assert(world.resourceNodes.empty());
    std::cout<<"material-based physical tool trial with resource cost and Core replay PASS\n";
}
