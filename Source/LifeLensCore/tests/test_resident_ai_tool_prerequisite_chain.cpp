#include <cassert>
#include <cmath>
#include "lifelens/ResidentAIExistingAffordance.h"
#include "lifelens/ResidentUnifiedAgency.h"

using namespace lifelens;

static Character makeResident()
{
    Character person;
    person.id=15;
    person.alive=true;
    person.lifeStage=LifeStage::Adult;
    person.civilization.character=person.id;
    person.needs={0.12,0.12,0.12,0.12,0.12};
    return person;
}

static AcceptedCognitiveDecision personalToolGoal()
{
    AcceptedCognitiveDecision decision;
    decision.actor=15;
    decision.minute=480;
    decision.priority=0.91;
    decision.intent=CognitiveIntentKind::CraftUsefulTools;
    // An accepted strategic thought is bound to its original Core context.
    // The one-resident decision slot rejects unbound synthetic input.
    decision.contextFingerprint=0x4466ULL;
    return decision;
}

int main()
{
    World world(4466);
    world.minute=480;
    world.resourceNodes.clear();
    ResourceNode wood;
    wood.id=101;
    wood.pos={0,0};
    wood.material=MaterialKind::Wood;
    wood.quantity=15;
    wood.maxQuantity=15;
    world.resourceNodes.push_back(wood);

    Character resident=makeResident();
    resident.civilization.knowledge.learn(
        TechniqueId::DiggingStick,KnowledgeLevel::Reproducible,1.0);
    const auto thought=personalToolGoal();

    // No technique ingredient may appear from nowhere. The resident can
    // still pursue their chosen tool goal by gathering REAL nearby wood.
    const auto first=chooseResidentAIExistingCivilizationAffordance(
        world,resident,{0,0},thought);
    assert(first);
    assert(first->intent==CivilizationIntent::Gather);
    assert(first->material==MaterialKind::Wood);
    assert(first->resourceNode==wood.id);
    assert(first->quantity>0);
    assert(world.resourceNodes.front().quantity==15);
    assert(resident.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood)==0);

    // After a real gather, with the SAME earlier AI decision, Core offers
    // tool craft instead of gathering repeatedly. No new model turn.
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Wood,2,0.65,1.0});
    const auto next=chooseResidentAIExistingCivilizationAffordance(
        world,resident,{0,0},thought);
    assert(next);
    assert(next->intent==CivilizationIntent::Craft);
    assert(next->technique==TechniqueId::DiggingStick);
    assert(next->facilityAction==FacilityBuildAction::None);
    assert(resident.civilization.inventory.count(
        ItemKind::DiggingStick,MaterialKind::Wood)==0);

    // Actual generated equipment is ONLY allowed after the existing Core
    // craft action consumes real ingredients in its execution path.
    auto result=reproduceTechnique(
        resident.id,TechniqueId::DiggingStick,
        resident.civilization.inventory,
        resident.civilization.knowledge,
        resident.civilization.craftingSkill);
    assert(result.success);
    assert(resident.civilization.inventory.count(
        ItemKind::DiggingStick,MaterialKind::Wood)==1);
    assert(resident.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood)==0);

    // Tool recipes can have intermediate crafted ingredients. Reuse the
    // exact same self-chosen strategic goal: Fiber -> Cordage -> StoneHammer.
    World complex(4466);
    complex.minute=1200; // still within the one-day goal horizon
    complex.resourceNodes.clear();
    ResourceNode fiber=wood;
    fiber.id=200;
    fiber.material=MaterialKind::Fiber;
    fiber.quantity=10;
    fiber.maxQuantity=10;
    complex.resourceNodes.push_back(fiber);
    Character smith=makeResident();
    smith.civilization.knowledge.learn(
        TechniqueId::StoneHammer,KnowledgeLevel::Reproducible,1.0);
    smith.civilization.knowledge.learn(
        TechniqueId::FiberCordage,KnowledgeLevel::Reproducible,1.0);
    smith.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Stone,2,0.8,1.0});
    smith.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Wood,1,0.8,1.0});
    const auto findFiber=chooseResidentAIExistingCivilizationAffordance(
        complex,smith,{0,0},thought);
    assert(findFiber);
    assert(findFiber->intent==CivilizationIntent::Gather);
    assert(findFiber->material==MaterialKind::Fiber);
    assert(findFiber->resourceNode==200);

    smith.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Fiber,2,0.8,1.0});
    const auto makeBinding=chooseResidentAIExistingCivilizationAffordance(
        complex,smith,{0,0},thought);
    assert(makeBinding);
    assert(makeBinding->intent==CivilizationIntent::Craft);
    assert(makeBinding->technique==TechniqueId::FiberCordage);
    auto cordage=reproduceTechnique(
        smith.id,TechniqueId::FiberCordage,
        smith.civilization.inventory,smith.civilization.knowledge,
        smith.civilization.craftingSkill);
    assert(cordage.success);
    const auto hammer=chooseResidentAIExistingCivilizationAffordance(
        complex,smith,{0,0},thought);
    assert(hammer);
    assert(hammer->intent==CivilizationIntent::Craft);
    assert(hammer->technique==TechniqueId::StoneHammer);
    assert(smith.civilization.inventory.count(
        ItemKind::StoneHammer,MaterialKind::Stone)==0);
    assert(complex.resourceNodes.front().quantity==10);

    auto built=reproduceTechnique(
        smith.id,TechniqueId::StoneHammer,
        smith.civilization.inventory,smith.civilization.knowledge,
        smith.civilization.craftingSkill);
    assert(built.success);
    assert(smith.civilization.inventory.count(
        ItemKind::StoneHammer,MaterialKind::Stone)==1);
    assert(smith.civilization.inventory.count(
        ItemKind::Cordage,MaterialKind::Fiber)==0);

    // A model cannot issue an everlasting or foreign resident goal.
    complex.minute=thought.minute+ResidentAgencyGoalHorizonMinutes+1;
    assert(!chooseResidentAIExistingCivilizationAffordance(
        complex,smith,{0,0},thought));
    complex.minute=1200;
    auto other=thought;
    other.actor=9999;
    assert(!chooseResidentAIExistingCivilizationAffordance(
        complex,smith,{0,0},other));

    // A place with NO usable raw material may not fabricate tools.
    World empty(4466);
    empty.minute=480;
    empty.resourceNodes.clear();
    Character isolated=makeResident();
    isolated.civilization.knowledge.learn(
        TechniqueId::DiggingStick,KnowledgeLevel::Reproducible,1.0);
    assert(!chooseResidentAIExistingCivilizationAffordance(
        empty,isolated,{0,0},thought));
    // Unknown tool knowledge is NOT a dead end. First gather actual flint
    // needed by the registered StrikeStone experiment; then the same goal
    // may attempt that Core experiment, subject to its real probability.
    World discovery(4466);
    discovery.minute=480;
    discovery.resourceNodes.clear();
    ResourceNode flint=wood;
    flint.id=404;
    flint.pos={0,0};
    flint.material=MaterialKind::Flint;
    flint.quantity=8;
    flint.maxQuantity=8;
    discovery.resourceNodes.push_back(flint);
    Character novice=makeResident();
    assert(!novice.civilization.knowledge.knowsAtLeast(
        TechniqueId::SharpFlake,KnowledgeLevel::Reproducible));
    assert(residentAIToolOutputNeeded(novice,TechniqueId::SharpFlake));
    assert(!residentAIToolOutputNeeded(
        novice,TechniqueId::DesignatedSanitationArea));
    assert(!residentAIToolOutputNeeded(novice,TechniqueId::FireMaking));
    const auto seekFlint=chooseResidentAIExistingCivilizationAffordance(
        discovery,novice,{0,0},thought);
    assert(seekFlint);
    assert(seekFlint->intent==CivilizationIntent::Gather);
    assert(seekFlint->material==MaterialKind::Flint);
    assert(seekFlint->resourceNode==404);
    assert(novice.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Flint)==0);
    assert(discovery.resourceNodes.front().quantity==8);
    novice.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Flint,2,0.7,0.9});
    const auto experimental=chooseResidentAIExistingCivilizationAffordance(
        discovery,novice,{0,0},thought);
    assert(experimental);
    assert(experimental->intent==CivilizationIntent::Experiment);
    assert(experimental->experiment==ExperimentKind::StrikeStone);
    assert(experimental->technique==TechniqueId::SharpFlake);
    assert(!novice.civilization.knowledge.knowsAtLeast(
        TechniqueId::SharpFlake,KnowledgeLevel::Reproducible));
    assert(discovery.resourceNodes.front().quantity==8);
    // A full personal tool supply should not justify unrelated maintenance
    // or excess craft practice under the model's tool-only preference.
    novice.civilization.inventory.add({
        ItemKind::SharpFlake,MaterialKind::Flint,2,0.7,0.9});
    assert(!residentAIToolOutputNeeded(novice,TechniqueId::SharpFlake));

    // No legal local source: both cognition and execution remain unable
    // to conjure the material, even when the model strongly wants a tool.
    World barren(4466);
    barren.minute=480;
    barren.resourceNodes.clear();
    Character beginner=makeResident();
    const auto noIngredient=chooseResidentAIExistingCivilizationAffordance(
        barren,beginner,{0,0},thought);
    assert(!noIngredient);
    assert(beginner.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Flint)==0);
    assert(beginner.civilization.knowledge.all().empty());
    // The same resident goal remains viable after one real FAILED tool
    // experiment when a DIFFERENT, registered experiment is available.
    World hypotheses(4466);
    hypotheses.minute=480;
    hypotheses.resourceNodes.clear();
    Character inventor=makeResident();
    inventor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Flint,2,0.7,1.0});
    inventor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Fiber,2,0.7,1.0});
    auto original=chooseResidentAIExistingCivilizationAffordance(
        hypotheses,inventor,{0,0},thought);
    assert(original);
    assert(original->intent==CivilizationIntent::Experiment);
    assert(original->technique==TechniqueId::SharpFlake
        ||original->technique==TechniqueId::FiberCordage);
    // The actual Core execution site records the failure with personal
    // provenance. The other registered experiment remains untouched.
    hypotheses.minute=481;
    rememberCivilizationAttemptOutcome(inventor,*original,481,false);
    const auto revised=chooseResidentAIExistingCivilizationAffordance(
        hypotheses,inventor,{0,0},thought);
    assert(revised);
    assert(revised->intent==CivilizationIntent::Experiment);
    assert(revised->technique!=original->technique);
    assert(!residentAIPlanAlreadyAttempted(
        inventor,thought,*revised,hypotheses.minute));
    assert(!inventor.civilization.knowledge.knowsAtLeast(
        original->technique,KnowledgeLevel::Reproducible));
    assert(inventor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Flint)==2);

    // A failed node 101 cannot prevent trying the accessible, real node 202
    // for the SAME material. No resource is inserted by the resident planner.
    World shifted(4466);
    shifted.minute=482;
    shifted.resourceNodes.clear();
    ResourceNode exhausted=wood;
    exhausted.id=101;
    exhausted.quantity=0;
    shifted.resourceNodes.push_back(exhausted);
    ResourceNode available=wood;
    available.id=202;
    available.quantity=7;
    shifted.resourceNodes.push_back(available);
    Character survivor=makeResident();
    survivor.civilization.knowledge.learn(
        TechniqueId::DiggingStick,KnowledgeLevel::Reproducible,1.0);
    CivilizationUtilityDecision failedGather;
    failedGather.intent=CivilizationIntent::Gather;
    failedGather.material=MaterialKind::Wood;
    failedGather.resourceNode=101;
    rememberCivilizationAttemptOutcome(
        survivor,failedGather,481,false,
        ResidentGatherFailureCause::SourceExhausted);
    const auto newSource=chooseResidentAIExistingCivilizationAffordance(
        shifted,survivor,{0,0},thought);
    assert(newSource);
    assert(newSource->intent==CivilizationIntent::Gather);
    assert(newSource->resourceNode==202);
    assert(!residentAIPlanAlreadyAttempted(
        survivor,thought,*newSource,shifted.minute));
    assert(shifted.resourceNodes[0].quantity==0);
    assert(shifted.resourceNodes[1].quantity==7);
    // A recipe needs THREE clay, but low-skill harvesting yields just ONE
    // physical unit per trip. The single resident must finish all genuine
    // gathers under its same original one-day personal thought: successful
    // partial prerequisites cannot be mistaken for replaying a completed goal.
    World slow(4466);
    slow.minute=480;
    slow.resourceNodes.clear();
    ResourceNode clay=wood;
    clay.id=515;
    clay.material=MaterialKind::Clay;
    clay.quantity=12;
    clay.maxQuantity=12;
    slow.resourceNodes.push_back(clay);
    Character potter=makeResident();
    potter.civilization.gatheringSkill=0.0;
    potter.civilization.knowledge.learn(
        TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible,1.0);
    RelationshipBook relations;
    UnifiedUtilityDecision fallback;
    fallback.kind=UnifiedDecisionKind::Civilization;
    fallback.civilization.intent=CivilizationIntent::Explore;
    fallback.utility=0.3;
    for(int step=0;step<3;++step){
        const auto selected=chooseSingleResidentAgencyDecision(
            slow,potter,{0,0},relations,fallback,&thought,
            nullptr,std::nullopt,false);
        assert(selected.authority==ResidentAgencyAuthority::ResidentCognition);
        assert(selected.decision.kind==UnifiedDecisionKind::Civilization);
        const auto& gather=selected.decision.civilization;
        assert(gather.intent==CivilizationIntent::Gather);
        assert(gather.material==MaterialKind::Clay);
        assert(gather.resourceNode==515);
        const auto executed=executeCivilizationDecisionAtPosition(
            slow,potter,gather,{0,0});
        assert(executed.executed &&executed.success);
        assert(executed.event.quantity==1);
        slow.minute+=15;
        rememberCivilizationAttemptOutcome(potter,gather,slow.minute,true);
        assert(potter.civilization.inventory.count(
            ItemKind::RawMaterial,MaterialKind::Clay)==step+1);
    }
    const auto completedInputs=chooseSingleResidentAgencyDecision(
        slow,potter,{0,0},relations,fallback,&thought,
        nullptr,std::nullopt,false);
    assert(completedInputs.authority==ResidentAgencyAuthority::ResidentCognition);
    assert(completedInputs.decision.civilization.intent==CivilizationIntent::Craft);
    assert(completedInputs.decision.civilization.technique
        ==TechniqueId::SimpleContainer);
    assert(slow.resourceNodes.front().quantity==9);

    // Explicit failure after a partial success is NEVER whitelisted.
    Character failedPotter=potter;
    failedPotter.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Clay,2);
    CivilizationUtilityDecision failedClay;
    failedClay.intent=CivilizationIntent::Gather;
    failedClay.material=MaterialKind::Clay;
    failedClay.resourceNode=515;
    rememberCivilizationAttemptOutcome(
        failedPotter,failedClay,slow.minute+1,false,
        ResidentGatherFailureCause::ResourceAccessUnavailable);
    assert(residentAIPlanAlreadyAttempted(
        failedPotter,thought,failedClay,slow.minute+2,true));

    return 0;
}
