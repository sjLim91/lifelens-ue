#include <cassert>
#include <iostream>
#include <vector>
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/CoreNavigation.h"
using namespace lifelens;
#define CHECK(x) do {if(!(x)){std::cerr<<__LINE__<<": "<<#x<<"\n";return 1;}}while(false)
static Character* findCharacter(Simulation& s,CharacterId id){
    for(auto& c:s.world().characters)if(c.id==id)return &c;
    return nullptr;
}
int main(){
    Simulation escorted(50505);
    escorted.setupNewGame();
    SimulationStateSnapshot escortedSnapshot=escorted.captureSnapshot();
    CHECK(escortedSnapshot.world.characters.size()>=2);
    const CharacterId escortParent=escortedSnapshot.world.characters[0].id;
    const CharacterId escortOtherParent=escortedSnapshot.world.characters[1].id;

    for(auto& resident:escortedSnapshot.world.characters){
        resident.needs={0.05,0.05,0.05,0.05,0.05};
        if(resident.id!=escortParent) resident.alive=false;
    }

    const GridPos escortHome=escortedSnapshot.runtime[escortParent].pos;
    GridPos sanitationTarget{};
    bool foundSanitationTarget=false;
    for(int radius=4;
        radius<=std::min(12,SettlementServiceRadiusGrid)
            && !foundSanitationTarget;
        ++radius){
        const GridPos candidates[4]={
            {escortHome.x+radius,escortHome.y},
            {escortHome.x-radius,escortHome.y},
            {escortHome.x,escortHome.y+radius},
            {escortHome.x,escortHome.y-radius}
        };
        for(const GridPos candidate:candidates){
            std::vector<GridPos> route;
            if(coreGroundTraversable(escortedSnapshot.world,candidate)
               && buildCoreGroundRoute(
                    escortedSnapshot.world,
                    escortHome,candidate,0,route)
               && route.size()>=2){
                sanitationTarget=candidate;
                foundSanitationTarget=true;
                break;
            }
        }
    }
    CHECK(foundSanitationTarget);

    PrimitiveSanitationSite childPit;
    childPit.id=7001;
    childPit.kind=PrimitiveSanitationSiteKind::DugPit;
    childPit.pos=sanitationTarget;
    childPit.establishedBy=escortParent;
    childPit.establishedMinute=escortedSnapshot.world.minute;
    childPit.active=true;
    childPit.improvementWork=DugSanitationPitWorkRequired;
    childPit.improvedBy=escortParent;
    childPit.improvedMinute=escortedSnapshot.world.minute;
    escortedSnapshot.world.primitiveSanitationSites.clear();
    escortedSnapshot.world.primitiveSanitationSites.push_back(childPit);

    Character escortedChild;
    escortedChild.id=7002;
    escortedChild.name="EscortedBaby";
    escortedChild.sex=Sex::Female;
    escortedChild.alive=true;
    escortedChild.hasBirthMinute=true;
    escortedChild.birthMinute=escortedSnapshot.world.minute;
    escortedChild.lifeStage=LifeStage::Baby;
    escortedChild.parentIds={escortParent,escortOtherParent};
    escortedChild.civilization.character=escortedChild.id;
    escortedChild.needs={0.01,0.01,0.01,0.99,0.01};
    escortedChild.development.attachment=0.90;
    escortedChild.development.confidence=0.80;
    escortedChild.development.socialSkill=0.20;
    escortedChild.development.emotionalSecurity=0.90;
    escortedChild.development.health=1.0;
    applyLifeStageProfile(escortedChild,LifeStage::Baby);
    escortedSnapshot.world.characters.push_back(escortedChild);

    SimulationRuntimeSnapshot escortedChildRuntime;
    escortedChildRuntime.pos=escortHome;
    escortedSnapshot.runtime.emplace(
        escortedChild.id,escortedChildRuntime);
    CHECK(escortedSnapshot.genealogy.registerBirth(
        escortedChild.id,escortParent,escortOtherParent));

    auto& escortToChild=escortedSnapshot.relationships.getOrCreate(
        escortParent,escortedChild.id);
    escortToChild.affection=0.95;
    escortToChild.commitment=0.95;
    escortToChild.comfort=0.95;
    auto& childToEscort=escortedSnapshot.relationships.getOrCreate(
        escortedChild.id,escortParent);
    childToEscort.affection=0.95;
    childToEscort.comfort=0.95;


    CHECK(escorted.restoreSnapshot(escortedSnapshot));
    bool pickedUp=false;
    for(int i=0;i<120 &&!pickedUp;++i){
        escorted.step();
        pickedUp=escorted.captureSnapshot().runtime.at(escortParent).dependentEscort.active();
    }
    CHECK(pickedUp);
    CHECK(escorted.world().primitiveSanitationSites.front().useCount==0);
    const auto outbound=escorted.captureSnapshot();
    // A real critical need interrupts outbound care, but never its escort obligation.
    findCharacter(escorted,escortParent)->needs.thirst=0.99;
    escorted.step();
    auto interrupted=escorted.captureSnapshot();
    CHECK(interrupted.runtime.at(escortParent).dependentEscort.active());
    CHECK(interrupted.runtime.at(escortParent).dependentEscort.returning);
    CHECK(interrupted.runtime.at(escortParent).pos.x==interrupted.runtime.at(escortedChild.id).pos.x);
    CHECK(interrupted.runtime.at(escortParent).pos.y==interrupted.runtime.at(escortedChild.id).pos.y);
    std::vector<std::uint8_t> bytes;
    CHECK(encodeSimulationSnapshot(interrupted,bytes));
    SimulationStateSnapshot decoded;
    CHECK(decodeSimulationSnapshot(bytes,decoded));
    CHECK(decoded.runtime.at(escortParent).dependentEscort.child==escortedChild.id);
    Simulation resumed(50505);
    CHECK(resumed.restoreSnapshot(decoded));
    // After survival work ends, the saved obligation resumes before ordinary planning.
    auto relieved=resumed.captureSnapshot();
    for(auto& adult:relieved.world.characters)adult.needs={0.02,0.02,0.02,0.02,0.02};
    auto& r=relieved.runtime.at(escortParent);
    r.pendingContext.clear();r.plan.clear();r.actionIndex=0;r.goal=Goal::Idle;
    r.navigationRoute.clear();r.navigationRouteIndex=0;
    r.navigationHasTarget=false;r.navigationRouteFailed=false;
    CHECK(resumed.restoreSnapshot(relieved));
    bool returned=false;
    for(int i=0;i<180 &&!returned;++i){
        GridPos before{},parent{},child{};
        CHECK(resumed.runtimePosition(escortParent,before));
        resumed.step();
        CHECK(resumed.runtimePosition(escortParent,parent));
        CHECK(resumed.runtimePosition(escortedChild.id,child));
        CHECK(parent.x==child.x &&parent.y==child.y);
        CHECK(std::abs(parent.x-before.x)+std::abs(parent.y-before.y)<=1);
        returned=!resumed.captureSnapshot().runtime.at(escortParent).dependentEscort.active();
    }
    CHECK(returned);
    GridPos returnedChild{};
    CHECK(resumed.runtimePosition(escortedChild.id,returnedChild));
    CHECK(returnedChild.x==escortHome.x &&returnedChild.y==escortHome.y);
    CHECK(resumed.world().primitiveSanitationSites.front().useCount==0);
    // Normal assistance still uses the site exactly once and persists the return.
    Simulation normal(50505);CHECK(normal.restoreSnapshot(escortedSnapshot));
    bool used=false;
    for(int i=0;i<180 &&!used;++i){
        normal.step();
        used=normal.world().primitiveSanitationSites.front().useCount==1;
    }
    CHECK(used);
    auto returning=normal.captureSnapshot();
    CHECK(returning.runtime.at(escortParent).dependentEscort.active());
    CHECK(returning.runtime.at(escortParent).dependentEscort.returning);
    CHECK(encodeSimulationSnapshot(returning,bytes));
    CHECK(decodeSimulationSnapshot(bytes,decoded));
    Simulation returnCopy(50505);CHECK(returnCopy.restoreSnapshot(decoded));
    const auto pending=returnCopy.observePendingContextAction(escortParent);
    CHECK(pending.active);
    returnCopy.setExternalPhysicalExecution(true);
    const auto bladder=findCharacter(returnCopy,escortedChild.id)->needs.bladder;
    CHECK(returnCopy.completeExternalContextAction(escortParent,pending.token,escortHome));
    CHECK(!returnCopy.completeExternalContextAction(escortParent,pending.token,escortHome));
    CHECK(returnCopy.world().primitiveSanitationSites.front().useCount==1);
    CHECK(findCharacter(returnCopy,escortedChild.id)->needs.bladder==bladder);
    CHECK(!returnCopy.captureSnapshot().runtime.at(escortParent).dependentEscort.active());
    // Site invalidation becomes a physical return with no invented assistance.
    Simulation invalid(50505);CHECK(invalid.restoreSnapshot(outbound));
    invalid.world().primitiveSanitationSites.front().active=false;
    CHECK(encodeSimulationSnapshot(invalid.captureSnapshot(),bytes));
    for(int i=0;i<180 &&invalid.captureSnapshot().runtime.at(escortParent).dependentEscort.active();++i)
        invalid.step();
    CHECK(!invalid.captureSnapshot().runtime.at(escortParent).dependentEscort.active());
    CHECK(invalid.world().primitiveSanitationSites.front().useCount==0);
    // Legacy in-flight return payloads upgrade to the dedicated state on load.
    auto legacy=returning;
    legacy.runtime.at(escortParent).dependentEscort.clear();
    CHECK(encodeSimulationSnapshot(legacy,bytes));
    CHECK(decodeSimulationSnapshot(bytes,decoded));
    Simulation upgraded(50505);CHECK(upgraded.restoreSnapshot(decoded));
    CHECK(upgraded.captureSnapshot().runtime.at(escortParent).dependentEscort.active());
    CHECK(upgraded.captureSnapshot().runtime.at(escortParent).dependentEscort.returning);
    // Survival preemption also preserves a return already underway.
    findCharacter(upgraded,escortParent)->needs.thirst=0.99;
    upgraded.step();
    CHECK(upgraded.captureSnapshot().runtime.at(escortParent).dependentEscort.active());
    CHECK(upgraded.captureSnapshot().runtime.at(escortParent).dependentEscort.returning);
    // A dependent has exactly one carrier in persisted truth.
    auto duplicate=returning;
    duplicate.runtime.at(escortOtherParent).dependentEscort=
        duplicate.runtime.at(escortParent).dependentEscort;
    duplicate.runtime.at(escortOtherParent).pos=duplicate.runtime.at(escortParent).pos;
    CHECK(!encodeSimulationSnapshot(duplicate,bytes));
    CHECK(!normal.restoreSnapshot(duplicate));
    CHECK(encodeSimulationSnapshot(returning,bytes));
    bytes.pop_back();
    decoded.world.minute=123;
    CHECK(!decodeSimulationSnapshot(bytes,decoded));
    CHECK(decoded.world.minute==123);
    // A dead caregiver leaves the child at its actual last position for community care.
    auto bereaved=interrupted;
    for(auto& adult:bereaved.world.characters)if(adult.id==escortParent)adult.alive=false;
    Simulation death(50505);CHECK(death.restoreSnapshot(bereaved));
    GridPos last{};CHECK(death.runtimePosition(escortedChild.id,last));
    death.step();
    GridPos after{};CHECK(death.runtimePosition(escortedChild.id,after));
    CHECK(after.x==last.x &&after.y==last.y);
    CHECK(!death.captureSnapshot().runtime.at(escortParent).dependentEscort.active());
    auto lostChild=interrupted;
    for(auto& child:lostChild.world.characters)if(child.id==escortedChild.id)child.alive=false;
    Simulation childDeath(50505);CHECK(childDeath.restoreSnapshot(lostChild));
    childDeath.step();
    CHECK(!childDeath.captureSnapshot().runtime.at(escortParent).dependentEscort.active());
}
