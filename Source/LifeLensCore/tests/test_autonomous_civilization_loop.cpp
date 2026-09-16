#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/SocialUtility.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static bool containsLog(const std::vector<std::string>& logs,const std::string& token)
{
    for(const auto& line:logs) if(line.find(token)!=std::string::npos) return true;
    return false;
}

static int totalKnowledge(const World& world,KnowledgeLevel minimum)
{
    int total=0;
    for(const auto& character:world.characters){
        for(const auto& record:character.civilization.knowledge.all()){
            if(static_cast<int>(record.level)>=static_cast<int>(minimum)) ++total;
        }
    }
    return total;
}

static int totalInventoryUnits(const World& world)
{
    int total=0;
    for(const auto& character:world.characters) total+=inventoryUnitCount(character.civilization.inventory);
    for(const auto& storage:world.storageSites) total+=inventoryUnitCount(storage.inventory);
    return total;
}

static int nonRenewableQuantity(const World& world)
{
    int total=0;
    for(const auto& node:world.resourceNodes) if(!node.renewable) total+=node.quantity;
    return total;
}

int main()
{
    // Civilization is a third utility axis, but urgent survival still wins.
    World utilityWorld(919191);
    utilityWorld.objects.push_back({1,ObjectKind::Fridge,{0,0},std::nullopt,{-0.075,0,0,0,0},5});
    utilityWorld.objects.push_back({2,ObjectKind::Sink,{0,0},std::nullopt,{0,-0.08,0,0,0},5});
    utilityWorld.objects.push_back({3,ObjectKind::Bed,{0,0},std::nullopt,{0,0,-0.05,0,0},5});
    utilityWorld.objects.push_back({4,ObjectKind::Toilet,{0,0},std::nullopt,{0,0,0,-0.1,0},5});

    Character utilityActor;
    utilityActor.id=41;
    utilityActor.name="UtilityActor";
    utilityActor.civilization.character=utilityActor.id;
    utilityActor.needs={0.08,0.08,0.08,0.08,0.08};
    utilityActor.personality.curiosity=0.95;
    utilityActor.personality.openness=0.92;
    utilityActor.personality.adaptability=0.88;
    utilityActor.personality.patience=0.82;
    utilityActor.civilization.gatheringSkill=0.68;
    utilityActor.civilization.learningSkill=0.72;
    utilityWorld.characters={utilityActor};
    RelationshipBook noRelationships;

    const UnifiedUtilityDecision calm=chooseUnifiedUtilityDecision(utilityWorld,utilityWorld.characters[0],noRelationships);
    CHECK(calm.kind==UnifiedDecisionKind::Civilization);
    CHECK(calm.civilization.intent==CivilizationIntent::Gather);

    utilityWorld.characters[0].needs.hunger=0.96;
    const UnifiedUtilityDecision hungry=chooseUnifiedUtilityDecision(utilityWorld,utilityWorld.characters[0],noRelationships);
    CHECK(hungry.kind==UnifiedDecisionKind::Physical);
    CHECK(hungry.physicalGoal==Goal::Eat);

    // Personal knowledge and inventory change autonomous priorities. The novice
    // still gathers, while only the resident who personally knows SharpFlake and
    // owns the needed parts can attempt the next ChippedStoneTool experiment.
    World divergenceWorld(818181);
    Character novice=utilityActor;
    novice.id=101;
    novice.civilization=IndividualCivilizationState{};
    novice.civilization.character=novice.id;
    novice.needs={0.05,0.05,0.05,0.05,0.05};

    Character flintKnower=novice;
    flintKnower.id=102;
    flintKnower.civilization.character=flintKnower.id;
    flintKnower.personality.curiosity=1.0;
    flintKnower.personality.openness=1.0;
    flintKnower.personality.patience=1.0;
    flintKnower.civilization.learningSkill=1.0;
    flintKnower.civilization.knowledge.learn(TechniqueId::SharpFlake,KnowledgeLevel::Reproducible,0.9);
    flintKnower.civilization.inventory.add({ItemKind::SharpFlake,MaterialKind::Flint,1,0.7,1.0});
    flintKnower.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,1,0.5,1.0});

    const CivilizationUtilityDecision noviceDecision=chooseCivilizationUtilityDecision(divergenceWorld,novice);
    const CivilizationUtilityDecision knowerDecision=chooseCivilizationUtilityDecision(divergenceWorld,flintKnower);
    CHECK(noviceDecision.intent==CivilizationIntent::Gather);
    CHECK(knowerDecision.intent==CivilizationIntent::Experiment);
    CHECK(knowerDecision.technique==TechniqueId::ChippedStoneTool);
    CHECK(!novice.civilization.knowledge.knowsAtLeast(TechniqueId::SharpFlake,KnowledgeLevel::Reproducible));

    // Store is an autonomous option once personal carrying pressure is high.
    Character storer=novice;
    storer.id=103;
    storer.civilization.character=storer.id;
    storer.personality.orderliness=1.0;
    storer.personality.conscientiousness=1.0;
    storer.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Stone,10,0.5,1.0});
    const CivilizationUtilityDecision storeDecision=bestStoreDecision(divergenceWorld,storer);
    CHECK(storeDecision.intent==CivilizationIntent::Store);
    CHECK(storeDecision.quantity>0);
    const int storageBefore=divergenceWorld.storageSites[0].inventory.count(ItemKind::RawMaterial,MaterialKind::Stone);
    CivilizationExecutionResult stored=executeCivilizationDecision(divergenceWorld,storer,storeDecision);
    CHECK(stored.executed && stored.success);
    CHECK(divergenceWorld.storageSites[0].inventory.count(ItemKind::RawMaterial,MaterialKind::Stone)>storageBefore);

    // Experiment and craft choices are gated by actual personal inputs/knowledge.
    Character experimenter=novice;
    experimenter.id=104;
    experimenter.civilization.character=experimenter.id;
    experimenter.personality.curiosity=1.0;
    experimenter.personality.openness=1.0;
    experimenter.personality.patience=1.0;
    experimenter.civilization.learningSkill=1.0;
    experimenter.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Flint,30,0.5,1.0});
    CivilizationUtilityDecision experimentDecision=bestExperimentDecision(divergenceWorld,experimenter);
    CHECK(experimentDecision.intent==CivilizationIntent::Experiment);
    CHECK(experimentDecision.technique==TechniqueId::SharpFlake);

    bool discovered=false;
    for(int minute=0;minute<500 && !discovered;++minute){
        divergenceWorld.minute=minute;
        experimentDecision=bestExperimentDecision(divergenceWorld,experimenter);
        if(experimentDecision.intent==CivilizationIntent::None){
            experimenter.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Flint,4,0.5,1.0});
            continue;
        }
        const CivilizationExecutionResult attempt=executeCivilizationDecision(divergenceWorld,experimenter,experimentDecision);
        discovered=attempt.executed && attempt.success;
        if(!discovered && experimenter.civilization.inventory.count(ItemKind::RawMaterial,MaterialKind::Flint)<2)
            experimenter.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Flint,4,0.5,1.0});
    }
    CHECK(discovered);
    CHECK(experimenter.civilization.knowledge.knowsAtLeast(TechniqueId::SharpFlake,KnowledgeLevel::Reproducible));
    experimenter.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Flint,4,0.5,1.0});
    const CivilizationUtilityDecision craftDecision=bestCraftDecision(divergenceWorld,experimenter);
    CHECK(craftDecision.intent==CivilizationIntent::Craft);
    const CivilizationExecutionResult crafted=executeCivilizationDecision(divergenceWorld,experimenter,craftDecision);
    CHECK(crafted.executed && crafted.success);

    // Presentation/read contract is driven only by a real executed civilization
    // decision. It is transient runtime state, not save-game state.
    Simulation activityProbe(4242001);
    activityProbe.setupNewGame();
    ResidentCivilizationActivityObservation observedActivity;
    bool foundActiveCivilization=false;
    for(int minute=0;minute<2000 && !foundActiveCivilization;++minute){
        activityProbe.step();
        for(const auto& resident:activityProbe.world().characters){
            const auto activity=activityProbe.observeResidentCivilizationActivity(resident.id);
            if(!activity.active) continue;
            observedActivity=activity;
            foundActiveCivilization=true;
            break;
        }
    }
    CHECK(foundActiveCivilization);
    CHECK(observedActivity.kind!=CivilizationActivityKind::None);
    CHECK(observedActivity.residentId!=0);
    CHECK(observedActivity.minute>=0);
    CHECK(observedActivity.minute<=activityProbe.world().minute);
    if(observedActivity.kind==CivilizationActivityKind::Gather){
        CHECK(observedActivity.resourceNode!=0);
        CHECK(observedActivity.hasSpatialTarget);
        const ResourceNode* observedNode=nullptr;
        for(const auto& node:activityProbe.world().resourceNodes){
            if(node.id==observedActivity.resourceNode){ observedNode=&node; break; }
        }
        CHECK(observedNode!=nullptr);
        CHECK(observedActivity.targetGridX==observedNode->pos.x);
        CHECK(observedActivity.targetGridY==observedNode->pos.y);
    }

    const SimulationStateSnapshot activeSnapshot=activityProbe.captureSnapshot();
    Simulation restoredProbe(1);
    std::string probeError;
    CHECK(restoredProbe.restoreSnapshot(activeSnapshot,&probeError));
    CHECK(probeError.empty());
    CHECK(!restoredProbe.observeResidentCivilizationActivity(observedActivity.residentId).active);

    activityProbe.runMinutes(5);
    CHECK(!activityProbe.observeResidentCivilizationActivity(observedActivity.residentId).active);

    // Full Simulation integration: same seed must progress identically while
    // actually consuming natural resources and creating individual knowledge.
    Simulation first(4242001);
    Simulation second(4242001);
    first.setupNewGame();
    second.setupNewGame();
    const int initialNonRenewable=nonRenewableQuantity(first.world());

    first.runMinutes(20000);
    second.runMinutes(20000);

    CHECK(containsLog(first.logs(),"-> Civilization "));
    CHECK(containsLog(first.logs(),"discovered "));
    CHECK(nonRenewableQuantity(first.world())<initialNonRenewable);
    CHECK(totalInventoryUnits(first.world())>0);
    CHECK(totalKnowledge(first.world(),KnowledgeLevel::Hypothesized)>0);
    CHECK(totalKnowledge(first.world(),KnowledgeLevel::Reproducible)>0);

    std::vector<std::uint8_t> firstBytes,secondBytes;
    std::string error;
    CHECK(encodeSimulationSnapshot(first.captureSnapshot(),firstBytes,&error));
    CHECK(error.empty());
    CHECK(encodeSimulationSnapshot(second.captureSnapshot(),secondBytes,&error));
    CHECK(firstBytes==secondBytes);

    // Save/restore continuation remains exact after civilization decisions.
    Simulation restored(1);
    CHECK(restored.restoreSnapshot(first.captureSnapshot(),&error));
    first.runMinutes(2500);
    restored.runMinutes(2500);
    std::vector<std::uint8_t> futureA,futureB;
    CHECK(encodeSimulationSnapshot(first.captureSnapshot(),futureA,&error));
    CHECK(encodeSimulationSnapshot(restored.captureSnapshot(),futureB,&error));
    CHECK(futureA==futureB);

    // NEW GAME is a true reset: depleted generated resources and ad-hoc storage
    // cannot leak. Production now starts with no civilization storage facility.
    CHECK(!first.world().resourceNodes.empty());
    first.world().resourceNodes.front().quantity=1;
    StorageSite leakedStorage;
    leakedStorage.id=9999;
    leakedStorage.inventory.add({ItemKind::RawMaterial,MaterialKind::Stone,99,0.5,1.0});
    first.world().storageSites.push_back(leakedStorage);
    first.setupNewGame();
    CHECK(!first.world().resourceNodes.empty());
    for(const auto& node:first.world().resourceNodes) CHECK(node.quantity==node.maxQuantity);
    CHECK(first.world().storageSites.empty());
    CHECK(first.world().generatedNaturalChunks.size()==1);

    std::cout << "autonomous civilization utility + simulation loop passed\n";
    return 0;
}
