#include <cassert>
#include <cstdint>
#include "lifelens/Civilization.h"

using namespace lifelens;

static std::uint64_t findAttempt(
    ExperimentContext context,
    bool wantSuccess)
{
    const double base=experimentBaseChance(context.kind,context.material);
    const double chance=clampCivilization01(
        base
        +clampCivilization01(context.learningSkill)*0.22
        +clampCivilization01(context.curiosity)*0.12
        +clampCivilization01(context.patience)*0.08);
    for(std::uint64_t attempt=0;attempt<10000;++attempt){
        context.attemptIndex=attempt;
        const bool success=civilizationRoll(context)<chance;
        if(success==wantSuccess) return attempt;
    }
    assert(false && "deterministic experiment roll search failed");
    return 0;
}

int main()
{
    // Material properties are physical affordances, not tech unlocks.
    const MaterialProperties flint=materialProperties(MaterialKind::Flint);
    const MaterialProperties wood=materialProperties(MaterialKind::Wood);
    assert(flint.sharpnessPotential>0.9);
    assert(wood.combustibility>0.8);

    // Finite gathering: resource quantity moves into the individual's inventory.
    IndividualCivilizationState a;
    a.character=1;
    a.gatheringSkill=0.5;
    ResourceNode flintNode{10,MaterialKind::Flint,5,5,false,0};
    const CivilizationEvent gathered=gatherResource(a,flintNode,3);
    assert(gathered.type==CivilizationEventType::Gathered);
    assert(gathered.actor==1);
    assert(gathered.quantity==3);
    assert(flintNode.quantity==2);
    assert(a.inventory.count(ItemKind::RawMaterial,MaterialKind::Flint)==3);

    // Renewable nodes cannot exceed their max quantity.
    ResourceNode fiberNode{11,MaterialKind::Fiber,2,5,true,2};
    fiberNode.regenerateDay();
    assert(fiberNode.quantity==4);
    fiberNode.regenerateDay();
    assert(fiberNode.quantity==5);

    // Storage is a real material transfer, not a UI-only counter.
    StorageSite cache;
    cache.id=100;
    const CivilizationEvent stored=storeItems(
        a,cache,ItemKind::RawMaterial,MaterialKind::Flint,1);
    assert(stored.quantity==1);
    assert(a.inventory.count(ItemKind::RawMaterial,MaterialKind::Flint)==2);
    assert(cache.inventory.count(ItemKind::RawMaterial,MaterialKind::Flint)==1);

    // Personal knowledge: one person's discovery never globally unlocks another.
    IndividualCivilizationState b;
    b.character=2;
    assert(a.knowledge.level(TechniqueId::SharpFlake)==KnowledgeLevel::Unknown);
    assert(b.knowledge.level(TechniqueId::SharpFlake)==KnowledgeLevel::Unknown);

    ExperimentContext sharpContext;
    sharpContext.worldSeed=424242;
    sharpContext.actor=a.character;
    sharpContext.kind=ExperimentKind::StrikeStone;
    sharpContext.material=MaterialKind::Flint;
    sharpContext.learningSkill=0.62;
    sharpContext.curiosity=0.78;
    sharpContext.patience=0.66;
    sharpContext.attemptIndex=findAttempt(sharpContext,true);

    // The same inputs produce the same deterministic outcome.
    const double firstRoll=civilizationRoll(sharpContext);
    const double secondRoll=civilizationRoll(sharpContext);
    assert(firstRoll==secondRoll);

    // We stored one flint, so return enough raw material for an experiment.
    assert(cache.inventory.transferTo(
        a.inventory,ItemKind::RawMaterial,MaterialKind::Flint,1));
    assert(a.inventory.count(ItemKind::RawMaterial,MaterialKind::Flint)==3);

    const ExperimentResult sharpDiscovery=attemptExperiment(
        sharpContext,a.inventory,a.knowledge);
    assert(sharpDiscovery.attempted);
    assert(sharpDiscovery.success);
    assert(sharpDiscovery.discovered==TechniqueId::SharpFlake);
    assert(sharpDiscovery.event.type==CivilizationEventType::Discovered);
    assert(sharpDiscovery.producedItem);
    assert(sharpDiscovery.output.kind==ItemKind::SharpFlake);
    assert(a.knowledge.knowsAtLeast(
        TechniqueId::SharpFlake,KnowledgeLevel::Reproducible));
    assert(b.knowledge.level(TechniqueId::SharpFlake)==KnowledgeLevel::Unknown);

    // A learned technique can be intentionally reproduced, but B cannot use it yet.
    a.inventory.add({ItemKind::RawMaterial,MaterialKind::Flint,2,0.5,1.0});
    const CraftResult aReproduces=reproduceTechnique(
        a.character,TechniqueId::SharpFlake,a.inventory,a.knowledge,0.60);
    assert(aReproduces.success);
    assert(a.inventory.count(ItemKind::SharpFlake,MaterialKind::Flint)>=2);

    b.inventory.add({ItemKind::RawMaterial,MaterialKind::Flint,2,0.5,1.0});
    const CraftResult bBlocked=reproduceTechnique(
        b.character,TechniqueId::SharpFlake,b.inventory,b.knowledge,0.95);
    assert(!bBlocked.success);
    assert(b.inventory.count(ItemKind::RawMaterial,MaterialKind::Flint)==2);

    // Failure consumes experiment resources and creates only a hypothesis, not mastery.
    IndividualCivilizationState c;
    c.character=3;
    c.inventory.add({ItemKind::RawMaterial,MaterialKind::Fiber,2,0.5,1.0});
    ExperimentContext cordageContext;
    cordageContext.worldSeed=424242;
    cordageContext.actor=c.character;
    cordageContext.kind=ExperimentKind::TwistFiber;
    cordageContext.material=MaterialKind::Fiber;
    cordageContext.learningSkill=0.15;
    cordageContext.curiosity=0.30;
    cordageContext.patience=0.25;
    cordageContext.attemptIndex=findAttempt(cordageContext,false);
    const ExperimentResult failedCordage=attemptExperiment(
        cordageContext,c.inventory,c.knowledge);
    assert(failedCordage.attempted);
    assert(!failedCordage.success);
    assert(failedCordage.event.type==CivilizationEventType::ExperimentFailed);
    assert(c.inventory.count(ItemKind::RawMaterial,MaterialKind::Fiber)==0);
    assert(c.knowledge.level(TechniqueId::FiberCordage)==KnowledgeLevel::Hypothesized);

    // Prerequisites create an emergent chain: a hafted tool cannot be discovered
    // before the individual actually understands sharp flakes.
    IndividualCivilizationState d;
    d.character=4;
    d.inventory.add({ItemKind::SharpFlake,MaterialKind::Flint,1,0.6,1.0});
    d.inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,1,0.5,1.0});
    ExperimentContext haftBlocked;
    haftBlocked.worldSeed=9;
    haftBlocked.actor=d.character;
    haftBlocked.kind=ExperimentKind::HaftSharpFlake;
    haftBlocked.material=MaterialKind::Wood;
    const ExperimentResult blockedExperiment=attemptExperiment(
        haftBlocked,d.inventory,d.knowledge);
    assert(!blockedExperiment.attempted);
    assert(d.inventory.count(ItemKind::SharpFlake,MaterialKind::Flint)==1);
    assert(d.inventory.count(ItemKind::RawMaterial,MaterialKind::Wood)==1);

    // Once the prerequisite is personally known, the next discovery becomes possible.
    a.inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,1,0.5,1.0});
    ExperimentContext haftContext;
    haftContext.worldSeed=424242;
    haftContext.actor=a.character;
    haftContext.kind=ExperimentKind::HaftSharpFlake;
    haftContext.material=MaterialKind::Wood;
    haftContext.learningSkill=0.72;
    haftContext.curiosity=0.76;
    haftContext.patience=0.70;
    haftContext.attemptIndex=findAttempt(haftContext,true);

    const ExperimentResult toolDiscovery=attemptExperiment(
        haftContext,a.inventory,a.knowledge);
    assert(toolDiscovery.attempted);
    assert(toolDiscovery.success);
    assert(toolDiscovery.discovered==TechniqueId::ChippedStoneTool);
    assert(toolDiscovery.output.kind==ItemKind::StoneCuttingTool);
    assert(itemCapability(toolDiscovery.output.kind)==ToolCapability::Chop);
    assert(a.knowledge.knowsAtLeast(
        TechniqueId::ChippedStoneTool,KnowledgeLevel::Reproducible));
    assert(b.knowledge.level(TechniqueId::ChippedStoneTool)==KnowledgeLevel::Unknown);

    // Repeated successful reproduction turns knowledge into practiced/mastered skill.
    KnowledgeState practice;
    practice.learn(TechniqueId::FiberCordage,KnowledgeLevel::Reproducible,0.7);
    practice.recordSuccessfulUse(TechniqueId::FiberCordage);
    practice.recordSuccessfulUse(TechniqueId::FiberCordage);
    practice.recordSuccessfulUse(TechniqueId::FiberCordage);
    assert(practice.level(TechniqueId::FiberCordage)==KnowledgeLevel::Practiced);
    for(int i=0;i<9;++i) practice.recordSuccessfulUse(TechniqueId::FiberCordage);
    assert(practice.level(TechniqueId::FiberCordage)==KnowledgeLevel::Mastered);

    return 0;
}
