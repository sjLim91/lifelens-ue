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


static void diagnoseSnapshotDifference(
    const SimulationStateSnapshot& a,
    const SimulationStateSnapshot& b)
{
    std::cerr<<"snapshot diff: minute "<<a.world.minute<<" vs "<<b.world.minute
             <<", token "<<a.nextContextActionToken<<" vs "<<b.nextContextActionToken
             <<", logs "<<a.logs.size()<<" vs "<<b.logs.size()<<"\n";

    const std::size_t sharedLogs=std::min(a.logs.size(),b.logs.size());
    for(std::size_t i=0;i<sharedLogs;++i){
        if(a.logs[i]==b.logs[i]) continue;
        std::cerr<<"first log diff @"<<i<<"\nA: "<<a.logs[i]
                 <<"\nB: "<<b.logs[i]<<"\n";
        break;
    }

    std::vector<CharacterId> ids;
    ids.reserve(a.runtime.size()+b.runtime.size());
    for(const auto& item:a.runtime) ids.push_back(item.first);
    for(const auto& item:b.runtime){
        if(std::find(ids.begin(),ids.end(),item.first)==ids.end())
            ids.push_back(item.first);
    }
    std::sort(ids.begin(),ids.end());

    for(CharacterId id:ids){
        const auto ia=a.runtime.find(id);
        const auto ib=b.runtime.find(id);
        if(ia==a.runtime.end() || ib==b.runtime.end()){
            std::cerr<<"runtime presence diff id="<<id<<"\n";
            continue;
        }
        const auto& x=ia->second;
        const auto& y=ib->second;
        if(x.pos.x!=y.pos.x || x.pos.y!=y.pos.y
           || x.goal!=y.goal
           || x.actionIndex!=y.actionIndex
           || x.pendingContext.kind!=y.pendingContext.kind
           || x.pendingContext.token!=y.pendingContext.token
           || x.navigationRouteIndex!=y.navigationRouteIndex
           || x.navigationRoute.size()!=y.navigationRoute.size()
           || x.navigationTarget.x!=y.navigationTarget.x
           || x.navigationTarget.y!=y.navigationTarget.y){
            std::cerr<<"runtime diff id="<<id
                     <<" pos("<<x.pos.x<<","<<x.pos.y<<") vs ("
                     <<y.pos.x<<","<<y.pos.y<<")"
                     <<" goal "<<static_cast<int>(x.goal)<<" vs "<<static_cast<int>(y.goal)
                     <<" action "<<x.actionIndex<<" vs "<<y.actionIndex
                     <<" pending "<<static_cast<int>(x.pendingContext.kind)<<"/"<<x.pendingContext.token
                     <<" vs "<<static_cast<int>(y.pendingContext.kind)<<"/"<<y.pendingContext.token
                     <<" route "<<x.navigationRouteIndex<<"/"<<x.navigationRoute.size()
                     <<" vs "<<y.navigationRouteIndex<<"/"<<y.navigationRoute.size()
                     <<" target("<<x.navigationTarget.x<<","<<x.navigationTarget.y<<") vs ("
                     <<y.navigationTarget.x<<","<<y.navigationTarget.y<<")\n";
        }
    }

    const std::size_t characters=std::min(
        a.world.characters.size(),b.world.characters.size());
    for(std::size_t i=0;i<characters;++i){
        const auto& x=a.world.characters[i];
        const auto& y=b.world.characters[i];
        if(x.id!=y.id
           || x.needs.hunger!=y.needs.hunger
           || x.needs.thirst!=y.needs.thirst
           || x.needs.sleep!=y.needs.sleep
           || x.needs.bladder!=y.needs.bladder
           || x.needs.hygiene!=y.needs.hygiene
           || inventoryUnitCount(x.civilization.inventory)
                !=inventoryUnitCount(y.civilization.inventory)
           || x.civilization.knowledge.all().size()
                !=y.civilization.knowledge.all().size()){
            std::cerr<<"character diff index="<<i
                     <<" id "<<x.id<<" vs "<<y.id
                     <<" needs H "<<x.needs.hunger<<" vs "<<y.needs.hunger
                     <<" T "<<x.needs.thirst<<" vs "<<y.needs.thirst
                     <<" S "<<x.needs.sleep<<" vs "<<y.needs.sleep
                     <<" B "<<x.needs.bladder<<" vs "<<y.needs.bladder
                     <<" Y "<<x.needs.hygiene<<" vs "<<y.needs.hygiene
                     <<" inv "<<inventoryUnitCount(x.civilization.inventory)
                     <<" vs "<<inventoryUnitCount(y.civilization.inventory)
                     <<" knowledge "<<x.civilization.knowledge.all().size()
                     <<" vs "<<y.civilization.knowledge.all().size()<<"\n";
        }
    }
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
    if(firstBytes!=secondBytes){
        diagnoseSnapshotDifference(first.captureSnapshot(),second.captureSnapshot());
    }
    CHECK(firstBytes==secondBytes);

    // Save/restore continuation remains exact after civilization decisions.
    Simulation restored(1);
    CHECK(restored.restoreSnapshot(first.captureSnapshot(),&error));
    first.runMinutes(2500);
    restored.runMinutes(2500);
    std::vector<std::uint8_t> futureA,futureB;
    CHECK(encodeSimulationSnapshot(first.captureSnapshot(),futureA,&error));
    CHECK(encodeSimulationSnapshot(restored.captureSnapshot(),futureB,&error));
    if(futureA!=futureB){
        diagnoseSnapshotDifference(first.captureSnapshot(),restored.captureSnapshot());
    }
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
    CHECK(first.world().generatedNaturalChunks.size()==2);

    std::cout << "autonomous civilization utility + simulation loop passed\n";
    return 0;
}
