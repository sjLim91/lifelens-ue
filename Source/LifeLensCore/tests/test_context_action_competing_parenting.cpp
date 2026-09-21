#include <cstddef>
#include <iostream>
#include <string>
#include "lifelens/LifeStage.h"
#include "lifelens/Simulation.h"
using namespace lifelens;
#define CHECK(x) do { if(!(x)){ std::cerr << "check failed line " << __LINE__ << '\n'; return 1; } } while(false)
static Character* findCharacter(Simulation& sim, CharacterId id){ for(auto& c:sim.world().characters) if(c.id==id) return &c; return nullptr; }
int main(){
    Simulation sim(20202); sim.setupNewGame();
    SimulationStateSnapshot snapshot=sim.captureSnapshot();
    CHECK(snapshot.world.characters.size()>=3);
    const CharacterId parentA=snapshot.world.characters[0].id;
    const CharacterId parentB=snapshot.world.characters[2].id;
    const int secondCareMinute=snapshot.world.minute+30;
    for(auto& c:snapshot.world.characters) if(c.id==parentA || c.id==parentB) c.needs={0.05,0.05,0.05,0.05,0.05};
    snapshot.runtime[parentA].penaltyUntilMinute=secondCareMinute;
    snapshot.runtime[parentB].penaltyUntilMinute=secondCareMinute;
    Character child; child.id=99; child.name="ArbitrationBaby"; child.sex=Sex::Female;
    child.hasBirthMinute=true; child.birthMinute=snapshot.world.minute; child.lifeStage=LifeStage::Baby;
    child.parentIds={parentA,parentB}; child.civilization.character=child.id;
    child.needs={0.01,0.01,0.01,0.99,0.01}; child.development.attachment=0.90;
    child.development.confidence=0.80; child.development.socialSkill=0.20;
    child.development.emotionalSecurity=0.90; child.development.health=1.0;
    applyLifeStageProfile(child,LifeStage::Baby); snapshot.world.characters.push_back(child);
    SimulationRuntimeSnapshot childRuntime; childRuntime.pos=snapshot.runtime[parentB].pos;
    snapshot.runtime.emplace(child.id,childRuntime); CHECK(snapshot.genealogy.registerBirth(child.id,parentA,parentB));
    for(CharacterId parent:{parentA,parentB}){
        auto& a=snapshot.relationships.getOrCreate(parent,child.id); a.affection=0.95; a.commitment=0.95; a.comfort=0.95;
        auto& b=snapshot.relationships.getOrCreate(child.id,parent); b.affection=0.95; b.comfort=0.95;
    }
    std::string error; CHECK(sim.restoreSnapshot(snapshot,&error)); CHECK(error.empty()); sim.setExternalPhysicalExecution(true);
    sim.step();
    PendingContextActionObservation first; int count=0;
    for(CharacterId parent:{parentA,parentB}){ auto p=sim.observePendingContextAction(parent); if(p.active && p.kind==ContextActionKind::Parenting && p.targetResident==child.id){ ++count; first=p; } }
    CHECK(count==1); CHECK(first.active); CHECK(first.parentingAction==ParentingAction::ToiletAssist);
    sim.runMinutes(30);
    PendingContextActionObservation second; count=0;
    for(CharacterId parent:{parentA,parentB}){ auto p=sim.observePendingContextAction(parent); if(p.active && p.kind==ContextActionKind::Parenting && p.targetResident==child.id){ ++count; if(p.actor!=first.actor) second=p; } }
    CHECK(count==2); CHECK(second.active);
    GridPos childPos{}; CHECK(sim.runtimePosition(child.id,childPos));
    Character* childCharacter=findCharacter(sim,child.id); CHECK(childCharacter!=nullptr);
    const double bladderBefore=childCharacter->needs.bladder; const std::size_t residueBefore=sim.world().environmentalResidues.all().size();
    CHECK(sim.completeExternalContextAction(first.actor,first.token,childPos));
    CHECK(childCharacter->needs.bladder<bladderBefore); CHECK(sim.world().environmentalResidues.all().size()==residueBefore+1);
    CHECK(!sim.observePendingContextAction(second.actor).active);
    CHECK(!sim.completeExternalContextAction(second.actor,second.token,childPos));
    CHECK(sim.world().environmentalResidues.all().size()==residueBefore+1);
    // If both biological parents are unavailable, a living adult in the same
    // household can provide temporary dependent care without rewriting genealogy.
    Simulation fallback(30303); fallback.setupNewGame();
    SimulationStateSnapshot orphanSnapshot=fallback.captureSnapshot();
    CHECK(orphanSnapshot.world.characters.size()>=4);
    const CharacterId deadParentA=orphanSnapshot.world.characters[0].id;
    const CharacterId householdCaregiver=orphanSnapshot.world.characters[1].id;
    const CharacterId deadParentB=orphanSnapshot.world.characters[2].id;
    const int orphanCareMinute=orphanSnapshot.world.minute;

    for(auto& resident:orphanSnapshot.world.characters){
        resident.needs={0.05,0.05,0.05,0.05,0.05};
        if(resident.id==deadParentA || resident.id==deadParentB){
            resident.alive=false;
        }
    }

    Character orphan;
    orphan.id=199;
    orphan.name="HouseholdCareBaby";
    orphan.sex=Sex::Male;
    orphan.hasBirthMinute=true;
    orphan.birthMinute=orphanCareMinute;
    orphan.lifeStage=LifeStage::Baby;
    orphan.parentIds={deadParentA,deadParentB};
    orphan.civilization.character=orphan.id;
    orphan.needs={0.01,0.01,0.01,0.96,0.01};
    orphan.development.attachment=0.55;
    orphan.development.confidence=0.45;
    orphan.development.socialSkill=0.15;
    orphan.development.emotionalSecurity=0.50;
    orphan.development.health=1.0;
    applyLifeStageProfile(orphan,LifeStage::Baby);
    orphanSnapshot.world.characters.push_back(orphan);

    SimulationRuntimeSnapshot orphanRuntime;
    orphanRuntime.pos=orphanSnapshot.runtime[householdCaregiver].pos;
    orphanSnapshot.runtime.emplace(orphan.id,orphanRuntime);
    CHECK(orphanSnapshot.genealogy.registerBirth(
        orphan.id,deadParentA,deadParentB));

    orphanSnapshot.households=HouseholdBook{};
    CHECK(orphanSnapshot.households.create(
        990,{householdCaregiver,orphan.id},0,0.0));

    auto& caregiverToOrphan=
        orphanSnapshot.relationships.getOrCreate(
            householdCaregiver,orphan.id);
    caregiverToOrphan.affection=0.80;
    caregiverToOrphan.commitment=0.78;
    caregiverToOrphan.comfort=0.82;
    auto& orphanToCaregiver=
        orphanSnapshot.relationships.getOrCreate(
            orphan.id,householdCaregiver);
    orphanToCaregiver.affection=0.70;
    orphanToCaregiver.comfort=0.76;

    std::string fallbackError;
    CHECK(fallback.restoreSnapshot(orphanSnapshot,&fallbackError));
    CHECK(fallbackError.empty());
    fallback.setExternalPhysicalExecution(true);
    fallback.step();

    const PendingContextActionObservation fallbackPending=
        fallback.observePendingContextAction(householdCaregiver);
    CHECK(fallbackPending.active);
    CHECK(fallbackPending.kind==ContextActionKind::Parenting);
    CHECK(fallbackPending.targetResident==orphan.id);
    CHECK(fallbackPending.parentingAction==ParentingAction::ToiletAssist);

    GridPos orphanPos{};
    CHECK(fallback.runtimePosition(orphan.id,orphanPos));
    Character* liveOrphan=findCharacter(fallback,orphan.id);
    CHECK(liveOrphan!=nullptr);
    const double orphanBladderBefore=liveOrphan->needs.bladder;
    CHECK(fallback.completeExternalContextAction(
        householdCaregiver,fallbackPending.token,orphanPos));
    CHECK(liveOrphan->needs.bladder<orphanBladderBefore);
    CHECK(liveOrphan->parentIds.size()==2);
    CHECK(liveOrphan->parentIds[0]==deadParentA);
    CHECK(liveOrphan->parentIds[1]==deadParentB);
    CHECK(fallback.genealogy().relationBetween(
        orphan.id,deadParentA)==KinshipType::Parent);
    CHECK(fallback.genealogy().relationBetween(
        orphan.id,deadParentB)==KinshipType::Parent);

    std::cout << "competing parenting arbitration passed\n"; return 0;
}
