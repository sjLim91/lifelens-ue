#include <cassert>
#include <cmath>
#include <iostream>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/ContextAction.h"
#include "lifelens/ToolEffectiveness.h"

using namespace lifelens;

static ConstructedFacility operationalFixture(
    FacilityId id,
    FacilityKind kind,
    GridPos pos,
    CharacterId builder,
    int minute)
{
    ConstructedFacility facility=
        makeFacilityConstructionSite(id,kind,pos,builder,minute);
    assert(facility.id!=0);
    for(auto& requirement:facility.requirements){
        requirement.delivered=requirement.required;
    }
    facility.constructionWork=facility.requiredWork;
    assert(activateConstructedFacility(facility,0,minute));
    assert(facilityOperationalAndActive(facility));
    return facility;
}

static void learnBronzePrerequisites(Character& resident)
{
    resident.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Reproducible,0.9);
    resident.civilization.knowledge.learn(
        TechniqueId::ChippedStoneTool,KnowledgeLevel::Reproducible,0.9);
    resident.civilization.knowledge.learn(
        TechniqueId::FiberCordage,KnowledgeLevel::Reproducible,0.9);
    resident.civilization.knowledge.learn(
        TechniqueId::FireMaking,KnowledgeLevel::Reproducible,0.9);
    resident.civilization.knowledge.learn(
        TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible,0.9);
    resident.civilization.knowledge.learn(
        TechniqueId::StoneHammer,KnowledgeLevel::Reproducible,0.9);
    resident.civilization.knowledge.learn(
        TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible,0.9);
}

int main()
{
    World world(8675309);
    world.facilities.clear();
    world.storageSites.clear();

    Character resident;
    resident.id=1;
    resident.name="BronzeWorker";
    resident.alive=true;
    resident.civilization.character=resident.id;
    resident.personality.curiosity=0.9;
    resident.personality.openness=0.8;
    resident.personality.patience=0.8;
    resident.personality.conscientiousness=0.8;
    resident.civilization.learningSkill=0.9;
    resident.civilization.craftingSkill=0.8;
    learnBronzePrerequisites(resident);

    const GridPos furnacePos{8,0};
    const GridPos workPos{0,8};
    world.facilities.push_back(
        operationalFixture(100,FacilityKind::Furnace,furnacePos,resident.id,0));
    world.facilities.push_back(
        operationalFixture(101,FacilityKind::WorkSurface,workPos,resident.id,0));

    // Bronze must begin from actual refined copper, tin-bearing ore, charcoal,
    // and an idle operational furnace. It is not a timer/era unlock.
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::CopperMetal,160,0.6,1.0});
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::TinOre,80,0.5,1.0});
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Charcoal,80,0.55,1.0});
    assert(bronzeAlloyingOpportunityAvailable(world,resident));

    CivilizationUtilityDecision alloyExperiment;
    alloyExperiment.intent=CivilizationIntent::Experiment;
    alloyExperiment.experiment=ExperimentKind::AlloyBronze;
    alloyExperiment.material=MaterialKind::TinOre;
    alloyExperiment.technique=TechniqueId::BronzeAlloying;

    // Public execution boundary also enforces the physical furnace location.
    CivilizationExecutionResult remote=
        executeCivilizationDecisionAtPosition(
            world,resident,alloyExperiment,{0,0},nullptr);
    assert(!remote.executed);

    bool alloyDiscovered=false;
    for(int attempt=0;attempt<50 && !alloyDiscovered;++attempt){
        world.minute=attempt+1;
        const CivilizationExecutionResult result=
            executeCivilizationDecisionAtPosition(
                world,resident,alloyExperiment,furnacePos,nullptr);
        assert(result.executed);
        alloyDiscovered=result.success;
    }
    assert(alloyDiscovered);
    assert(resident.civilization.knowledge.knowsAtLeast(
        TechniqueId::BronzeAlloying,KnowledgeLevel::Reproducible));
    assert(resident.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::BronzeMetal)>=2);

    // Once known, repeat production is a real furnace action and consumes the
    // physical copper/tin/charcoal recipe rather than spawning an age reward.
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::CopperMetal,2,0.6,1.0});
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::TinOre,1,0.5,1.0});
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Charcoal,1,0.55,1.0});
    const int bronzeBefore=resident.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::BronzeMetal);
    const CivilizationUtilityDecision alloyRepeat=
        bestPrimitiveFurnaceDecision(world,resident,furnacePos);
    assert(alloyRepeat.intent==CivilizationIntent::Craft);
    assert(alloyRepeat.technique==TechniqueId::BronzeAlloying);
    assert(alloyRepeat.facilityAction==FacilityBuildAction::AlloyBronze);
    assert(alloyRepeat.facility==100);
    const CivilizationExecutionResult alloyed=
        executeCivilizationDecisionAtPosition(
            world,resident,alloyRepeat,furnacePos,nullptr);
    assert(alloyed.executed && alloyed.success);
    assert(resident.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::BronzeMetal)==bronzeBefore+2);

    // A bronze edge is a separate discovery and requires a real WorkSurface.
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::BronzeMetal,64,0.65,1.0});
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Wood,64,0.5,1.0});
    resident.civilization.inventory.add({
        ItemKind::Cordage,MaterialKind::Fiber,64,0.7,1.0});

    CivilizationUtilityDecision edgeExperiment;
    edgeExperiment.intent=CivilizationIntent::Experiment;
    edgeExperiment.experiment=ExperimentKind::ForgeBronzeEdge;
    edgeExperiment.material=MaterialKind::BronzeMetal;
    edgeExperiment.technique=TechniqueId::BronzeEdgeToolmaking;

    assert(civilizationContextRequiresSpatialTarget(edgeExperiment));
    GridPos resolved{};
    SanitationSiteId sanitation=0;
    assert(resolveCivilizationContextTarget(
        world,resident,edgeExperiment,{0,0},resolved,sanitation,nullptr));
    assert(resolved.x==workPos.x && resolved.y==workPos.y);

    const CivilizationExecutionResult wrongSurface=
        executeCivilizationDecisionAtPosition(
            world,resident,edgeExperiment,furnacePos,nullptr);
    assert(!wrongSurface.executed);

    bool edgeDiscovered=false;
    for(int attempt=0;attempt<50 && !edgeDiscovered;++attempt){
        world.minute=100+attempt;
        const CivilizationExecutionResult result=
            executeCivilizationDecisionAtPosition(
                world,resident,edgeExperiment,workPos,nullptr);
        assert(result.executed);
        edgeDiscovered=result.success;
    }
    assert(edgeDiscovered);
    assert(resident.civilization.knowledge.knowsAtLeast(
        TechniqueId::BronzeEdgeToolmaking,KnowledgeLevel::Reproducible));
    assert(resident.civilization.inventory.count(
        ItemKind::BronzeEdgeTool,MaterialKind::BronzeMetal)>=1);

    // Reproduction keeps the same material prerequisites.
    while(resident.civilization.inventory.remove(
        ItemKind::BronzeEdgeTool,MaterialKind::BronzeMetal,1)) {}
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::BronzeMetal,1,0.65,1.0});
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Wood,1,0.5,1.0});
    resident.civilization.inventory.add({
        ItemKind::Cordage,MaterialKind::Fiber,1,0.7,1.0});
    const CraftResult reproduced=reproduceTechnique(
        resident.id,TechniqueId::BronzeEdgeToolmaking,
        resident.civilization.inventory,resident.civilization.knowledge,
        resident.civilization.craftingSkill);
    assert(reproduced.success);
    assert(reproduced.output.kind==ItemKind::BronzeEdgeTool);
    assert(reproduced.output.material==MaterialKind::BronzeMetal);

    // A bronze edge must actually improve work, not be a cosmetic technology.
    Inventory stoneTools;
    stoneTools.add({
        ItemKind::StoneCuttingTool,MaterialKind::Flint,1,0.80,1.0});
    Inventory bronzeTools;
    bronzeTools.add({
        ItemKind::BronzeEdgeTool,MaterialKind::BronzeMetal,1,0.80,1.0});
    const GatherToolUseProfile stone=
        inspectGatherTool(stoneTools,MaterialKind::Wood);
    const GatherToolUseProfile bronze=
        inspectGatherTool(bronzeTools,MaterialKind::Wood);
    assert(stone.available && bronze.available);
    assert(bronze.tool.kind==ItemKind::BronzeEdgeTool);
    assert(bronze.quantityMultiplier>stone.quantityMultiplier);
    assert(bronze.wear<stone.wear);
    assert(gatheringQuantityWithTool(
        bronzeTools,MaterialKind::Wood,3)
        > gatheringQuantityWithTool(
            stoneTools,MaterialKind::Wood,3));

    // New appended values are valid persisted civilization data.
    assert(validMaterialKind(MaterialKind::BronzeMetal));
    assert(validItemKind(ItemKind::BronzeEdgeTool));
    assert(validTechniqueId(TechniqueId::BronzeAlloying));
    assert(validTechniqueId(TechniqueId::BronzeEdgeToolmaking));

    std::cout<<"bronze progression passed\n";
    return 0;
}
