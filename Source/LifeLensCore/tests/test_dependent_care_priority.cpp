#include <iostream>
#include <string>

#include "lifelens/Simulation.h"
#include "lifelens/ResidentCognitiveCarePreference.h"

using namespace lifelens;

#define CHECK(expr) do {     if(!(expr)){         std::cerr<<"CHECK failed: " #expr<<" line "<<__LINE__<<'\n';         return 1;     } } while(false)

static Character* findResident(Simulation& sim,CharacterId id)
{
    for(Character& resident:sim.world().characters){
        if(resident.id==id) return &resident;
    }
    return nullptr;
}

static int cognitiveCareChoiceTests()
{
    Simulation sim(909090);
    sim.setupNewGame();
    auto snap=sim.captureSnapshot();
    CHECK(snap.world.characters.size()>=2);
    const auto a=snap.world.characters[0].id;
    const auto b=snap.world.characters[1].id;
    auto& parentA=snap.world.characters[0];
    auto& parentB=snap.world.characters[1];

    // Two equally capable parents; the baseline uses the first valid
    // caregiver, while the single accepted AI intent can prefer the second.
    parentB.personality=parentA.personality;
    parentB.development=parentA.development;
    parentA.needs={0.02,0.02,0.02,0.02,0.02};
    parentB.needs=parentA.needs;
    parentA.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,5,0.8,1.0});
    parentB.civilization.inventory=parentA.civilization.inventory;

    Character child;
    child.id=998877;
    child.name="DependentChoice";
    child.sex=Sex::Female;
    child.alive=true;
    child.lifeStage=LifeStage::Toddler;
    child.hasBirthMinute=true;
    child.birthMinute=0;
    child.parentIds={a,b};
    child.civilization.character=child.id;
    applyLifeStageProfile(child,LifeStage::Toddler);
    child.needs={0.70,0.10,0.10,0.10,0.10};
    child.development.health=1.0;
    snap.world.characters.push_back(child);
    CHECK(snap.genealogy.registerBirth(child.id,a,b));
    const GridPos home=snap.runtime[a].pos;
    snap.runtime[b].pos=home;
    SimulationRuntimeSnapshot babyRuntime;
    babyRuntime.pos=home;
    snap.runtime.emplace(child.id,babyRuntime);
    for(auto id:{a,b}){
        auto& runtime=snap.runtime[id];
        runtime.goal=Goal::Idle;
        runtime.plan.clear();
        runtime.pendingContext.clear();
        runtime.penaltyUntilMinute=0;
        auto& rel=snap.relationships.getOrCreate(id,child.id);
        rel.affection=0.9;
        rel.commitment=0.8;
        rel.comfort=0.9;
        rel.trust=0.9;
    }
    snap.world.minute=29;
    Simulation ordinary(909090);
    CHECK(ordinary.restoreSnapshot(snap));
    ordinary.step();
    const auto ordinaryA=ordinary.observePendingContextAction(a);
    const auto ordinaryB=ordinary.observePendingContextAction(b);
    const CharacterId baseline=ordinaryA.active
        &&ordinaryA.kind==ContextActionKind::Parenting ?a:
        (ordinaryB.active &&ordinaryB.kind==ContextActionKind::Parenting ?b:0);
    CHECK(baseline!=0);
    const CharacterId chosen=baseline==a?b:a;

    AcceptedCognitiveDecision thought;
    thought.actor=chosen;
    thought.target=child.id;
    thought.minute=snap.world.minute;
    thought.trigger=CognitiveTriggerKind::Reflection;
    thought.contextFingerprint=99221;
    thought.backendIdentity="local-model-test";
    thought.intent=CognitiveIntentKind::CareForDependent;
    thought.priority=1.0;
    thought.rationaleDigest="test-digest";
    CHECK(appendAcceptedCognitiveDecision(
        snap.acceptedCognitiveDecisions,thought,snap.world));
    CHECK(residentCognitiveCarePreference(
        snap.world,snap.acceptedCognitiveDecisions,
        snap.world.characters[baseline==a?1:0],child)==0.0);
    CHECK(residentCognitiveCarePreference(
        snap.world,snap.acceptedCognitiveDecisions,
        snap.world.characters[chosen==a?0:1],child)>0.0);
    Simulation selected(909090);
    CHECK(selected.restoreSnapshot(snap));
    Simulation replay(909090);
    CHECK(replay.restoreSnapshot(snap));
    selected.step();
    replay.step();
    const auto pending=selected.observePendingContextAction(chosen);
    CHECK(pending.active);
    CHECK(pending.kind==ContextActionKind::Parenting);
    CHECK(pending.targetResident==child.id);
    CHECK(pending.token==
        replay.observePendingContextAction(chosen).token);
    const auto* liveChild=findResident(selected,child.id);
    CHECK(liveChild!=nullptr);
    CHECK(liveChild->needs.hunger>=0.65);
    CHECK(residentCognitiveCarePreference(
        selected.world(),snap.acceptedCognitiveDecisions,
        selected.world().characters[chosen==a?0:1],
        *liveChild)>0.0);

    // The same person's newer unrelated idea cannot silently keep their
    // old preferred dependent target forever.
    thought.intent=CognitiveIntentKind::ExploreOpportunity;
    thought.target=0;
    auto changed=snap.acceptedCognitiveDecisions;
    changed.front()=thought;
    CHECK(residentCognitiveCarePreference(
        snap.world,changed,snap.world.characters[chosen==a?0:1],
        child)==0.0);
    return 0;
}

int main()
{
    CHECK(cognitiveCareChoiceTests()==0);
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
