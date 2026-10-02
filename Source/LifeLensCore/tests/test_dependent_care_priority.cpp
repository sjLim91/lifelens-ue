#include <iostream>
#include <string>

#include "lifelens/Simulation.h"

using namespace lifelens;

#define CHECK(expr) do {     if(!(expr)){         std::cerr<<"CHECK failed: " #expr<<" line "<<__LINE__<<'\n';         return 1;     } } while(false)

static Character* findResident(Simulation& sim,CharacterId id)
{
    for(Character& resident:sim.world().characters){
        if(resident.id==id) return &resident;
    }
    return nullptr;
}

int main()
{
    Simulation sim(707070);
    sim.setupNewGame();
    SimulationStateSnapshot snapshot=sim.captureSnapshot();
    CHECK(snapshot.world.characters.size()>=2);

    const CharacterId gestationalParent=snapshot.world.characters[0].id;
    const CharacterId caregiver=snapshot.world.characters[1].id;

    // Keep exactly one living biological caregiver. It is deliberately not
    // the gestational parent, so this scenario must use real carried water.
    for(Character& resident:snapshot.world.characters){
        resident.needs={0.05,0.05,0.05,0.05,0.05};
        if(resident.id!=caregiver){
            resident.alive=false;
        }
    }
    Character* caregiverSnapshot=nullptr;
    for(Character& resident:snapshot.world.characters){
        if(resident.id==caregiver){
            caregiverSnapshot=&resident;
            break;
        }
    }
    CHECK(caregiverSnapshot!=nullptr);
    caregiverSnapshot->civilization.inventory.add({
        ItemKind::SimpleContainer,
        MaterialKind::Unknown,
        1,
        0.70,
        1.0
    });
    CHECK(emptySimpleContainerCount(
        caregiverSnapshot->civilization.inventory)==1);

    ResourceNodeId waterNode=0;
    GridPos waterAccess{};
    for(const ResourceNode& node:snapshot.world.resourceNodes){
        if(node.material!=MaterialKind::Water || node.quantity<=0) continue;
        if(resolveCivilizationResourceAccessGridPosition(
            snapshot.world,node.id,waterAccess)){
            waterNode=node.id;
            break;
        }
    }
    CHECK(waterNode!=0);

    const GridPos childHome{
        waterAccess.x+6,
        waterAccess.y
    };
    snapshot.runtime[caregiver].pos=childHome;
    snapshot.runtime[caregiver].goal=Goal::Idle;
    snapshot.runtime[caregiver].plan.clear();
    snapshot.runtime[caregiver].plan.push_back(
        {ActionType::Idle,0,120});

    Character child;
    child.id=9001;
    child.name="CarePriorityBaby";
    child.sex=Sex::Female;
    child.alive=true;
    child.hasBirthMinute=true;
    child.birthMinute=snapshot.world.minute;
    child.lifeStage=LifeStage::Baby;
    child.parentIds={gestationalParent,caregiver};
    child.civilization.character=child.id;
    child.needs={0.10,0.96,0.10,0.10,0.10};
    child.development.health=1.0;
    child.development.attachment=0.70;
    child.development.emotionalSecurity=0.70;
    applyLifeStageProfile(child,LifeStage::Baby);
    snapshot.world.characters.push_back(child);

    SimulationRuntimeSnapshot childRuntime;
    childRuntime.pos=childHome;
    snapshot.runtime.emplace(child.id,childRuntime);
    CHECK(snapshot.genealogy.registerBirth(
        child.id,gestationalParent,caregiver));

    auto& caregiverToChild=
        snapshot.relationships.getOrCreate(
            caregiver,child.id);
    caregiverToChild.affection=0.92;
    caregiverToChild.commitment=0.90;
    caregiverToChild.comfort=0.88;
    auto& childToCaregiver=
        snapshot.relationships.getOrCreate(
            child.id,caregiver);
    childToCaregiver.affection=0.80;
    childToCaregiver.comfort=0.82;

    std::string error;
    CHECK(sim.restoreSnapshot(snapshot,&error));
    CHECK(error.empty());
    sim.setExternalPhysicalExecution(true);

    // Critical direct care must preempt the caregiver's low-priority plan and
    // schedule physical water acquisition. The child remains at home.
    sim.step();
    PendingContextActionObservation fetch=
        sim.observePendingContextAction(caregiver);
    CHECK(fetch.active);
    CHECK(fetch.kind==ContextActionKind::Civilization);
    CHECK(fetch.civilizationIntent==CivilizationIntent::Gather);
    CHECK(fetch.material==MaterialKind::Water);
    CHECK(fetch.resourceNode==waterNode);
    CHECK(fetch.hasSpatialTarget);
    CHECK(fetch.targetPos.x==waterAccess.x);
    CHECK(fetch.targetPos.y==waterAccess.y);

    GridPos childPos{};
    CHECK(sim.runtimePosition(child.id,childPos));
    CHECK(childPos.x==childHome.x && childPos.y==childHome.y);

    CHECK(sim.completeExternalContextAction(
        caregiver,fetch.token,fetch.targetPos));
    Character* liveCaregiver=findResident(sim,caregiver);
    CHECK(liveCaregiver!=nullptr);
    CHECK(portableWaterCount(
        liveCaregiver->civilization.inventory)==1);
    CHECK(sim.runtimePosition(child.id,childPos));
    CHECK(childPos.x==childHome.x && childPos.y==childHome.y);

    // The five-minute emergency cadence should now send the caregiver back to
    // the child and Feed using the carried water.
    for(int i=0;i<5;++i) sim.step();
    PendingContextActionObservation feed=
        sim.observePendingContextAction(caregiver);
    CHECK(feed.active);
    CHECK(feed.kind==ContextActionKind::Parenting);
    CHECK(feed.targetResident==child.id);
    CHECK(feed.parentingAction==ParentingAction::Feed);
    CHECK(feed.hasSpatialTarget);
    CHECK(feed.targetPos.x==childHome.x);
    CHECK(feed.targetPos.y==childHome.y);

    Character* liveChild=findResident(sim,child.id);
    CHECK(liveChild!=nullptr);
    const double thirstBefore=liveChild->needs.thirst;
    CHECK(sim.completeExternalContextAction(
        caregiver,feed.token,childHome));
    CHECK(liveChild->needs.thirst<thirstBefore);
    CHECK(portableWaterCount(
        liveCaregiver->civilization.inventory)==0);

    // A gestational parent can nurse a Baby without inventing carried water.
    Character nursingParent=*liveCaregiver;
    nursingParent.id=42;
    Character nursingBaby=*liveChild;
    nursingBaby.id=43;
    nursingBaby.parentIds={42,44};
    nursingBaby.lifeStage=LifeStage::Baby;
    CHECK(nursingCareAvailable(nursingParent,nursingBaby));
    nursingBaby.parentIds={44,42};
    CHECK(!nursingCareAvailable(nursingParent,nursingBaby));

    std::cout
        <<"P0-B dependent care priority and physical provisioning passed\n";
    return 0;
}
