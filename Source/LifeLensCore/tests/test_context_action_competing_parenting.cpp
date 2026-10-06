#include <cstddef>
#include <iostream>
#include <string>
#include "lifelens/LifeStage.h"\n#include "lifelens/CoreNavigation.h"
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

    // With real sanitation infrastructure, autonomous ToiletAssist is a
    // two-stage physical action: reach the child, then escort the dependent
    // through Core navigation to the site. The child must move continuously
    // with the caregiver; waste is deposited at the site rather than at home.
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

    std::string escortedError;
    CHECK(escorted.restoreSnapshot(
        escortedSnapshot,&escortedError));
    CHECK(escortedError.empty());

    Character* liveEscortedChild=
        findCharacter(escorted,escortedChild.id);
    CHECK(liveEscortedChild!=nullptr);
    const double escortedBladderBefore=
        liveEscortedChild->needs.bladder;
    GridPos previousEscortedChildPos=escortHome;
    bool dependentMoved=false;
    bool assistedAtPit=false;

    for(int minute=0;minute<180 && !assistedAtPit;++minute){
        escorted.step();

        GridPos currentChildPos{};
        GridPos currentParentPos{};
        CHECK(escorted.runtimePosition(
            escortedChild.id,currentChildPos));
        CHECK(escorted.runtimePosition(
            escortParent,currentParentPos));

        const int dx=std::abs(
            currentChildPos.x-previousEscortedChildPos.x);
        const int dy=std::abs(
            currentChildPos.y-previousEscortedChildPos.y);
        CHECK(dx<=1 && dy<=1);

        if(currentChildPos.x!=escortHome.x
           || currentChildPos.y!=escortHome.y){
            dependentMoved=true;
            CHECK(currentChildPos.x==currentParentPos.x);
            CHECK(currentChildPos.y==currentParentPos.y);
        }
        previousEscortedChildPos=currentChildPos;

        assistedAtPit=
            liveEscortedChild->needs.bladder
                <escortedBladderBefore;
    }

    CHECK(dependentMoved);
    CHECK(assistedAtPit);
    GridPos finalEscortedChildPos{};
    GridPos finalEscortParentPos{};
    CHECK(escorted.runtimePosition(
        escortedChild.id,finalEscortedChildPos));
    CHECK(escorted.runtimePosition(
        escortParent,finalEscortParentPos));
    CHECK(finalEscortedChildPos.x==sanitationTarget.x);
    CHECK(finalEscortedChildPos.y==sanitationTarget.y);
    CHECK(finalEscortParentPos.x==sanitationTarget.x);
    CHECK(finalEscortParentPos.y==sanitationTarget.y);

    const PrimitiveSanitationSite* usedPit=
        findPrimitiveSanitationSite(
            escorted.world().primitiveSanitationSites,
            childPit.id);
    CHECK(usedPit!=nullptr);
    CHECK(usedPit->useCount==1);

    bool foundChildWasteAtPit=false;
    bool foundChildWasteAtHome=false;
    for(const auto& residue:escorted.world().environmentalResidues.all()){
        if(residue.kind!=EnvironmentalResidueKind::HumanWaste
           || residue.sourceCharacter!=escortedChild.id){
            continue;
        }
        if(residue.pos.x==sanitationTarget.x
           && residue.pos.y==sanitationTarget.y){
            foundChildWasteAtPit=true;
            CHECK(
                residue.radiusTiles
                ==primitiveSanitationResidueRadiusTiles(
                    PrimitiveSanitationSiteKind::DugPit));
        }
        if(residue.pos.x==escortHome.x
           && residue.pos.y==escortHome.y){
            foundChildWasteAtHome=true;
        }
    }
    CHECK(foundChildWasteAtPit);
    CHECK(!foundChildWasteAtHome);
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
        orphan.id,deadParentA)==KinshipType::Child);
    CHECK(fallback.genealogy().relationBetween(
        orphan.id,deadParentB)==KinshipType::Child);

    // A nuclear-household orphan can still receive community care when no
    // parent, co-resident adult, or close adult kin remains available.
    Simulation community(40404); community.setupNewGame();
    SimulationStateSnapshot communitySnapshot=community.captureSnapshot();
    CHECK(communitySnapshot.world.characters.size()>=4);
    const CharacterId communityDeadA=communitySnapshot.world.characters[0].id;
    const CharacterId communityAdultA=communitySnapshot.world.characters[1].id;
    const CharacterId communityDeadB=communitySnapshot.world.characters[2].id;
    const CharacterId communityAdultB=communitySnapshot.world.characters[3].id;

    for(auto& resident:communitySnapshot.world.characters){
        resident.needs={0.05,0.05,0.05,0.05,0.05};
        if(resident.id==communityDeadA || resident.id==communityDeadB){
            resident.alive=false;
        }
    }

    Character loneOrphan;
    loneOrphan.id=299;
    loneOrphan.name="CommunityCareBaby";
    loneOrphan.sex=Sex::Female;
    loneOrphan.hasBirthMinute=true;
    loneOrphan.birthMinute=communitySnapshot.world.minute;
    loneOrphan.lifeStage=LifeStage::Baby;
    loneOrphan.parentIds={communityDeadA,communityDeadB};
    loneOrphan.civilization.character=loneOrphan.id;
    loneOrphan.needs={0.01,0.01,0.01,0.97,0.01};
    loneOrphan.development.attachment=0.45;
    loneOrphan.development.confidence=0.40;
    loneOrphan.development.socialSkill=0.12;
    loneOrphan.development.emotionalSecurity=0.42;
    loneOrphan.development.health=1.0;
    applyLifeStageProfile(loneOrphan,LifeStage::Baby);
    communitySnapshot.world.characters.push_back(loneOrphan);

    SimulationRuntimeSnapshot loneRuntime;
    loneRuntime.pos=communitySnapshot.runtime[communityAdultA].pos;
    communitySnapshot.runtime.emplace(loneOrphan.id,loneRuntime);
    CHECK(communitySnapshot.genealogy.registerBirth(
        loneOrphan.id,communityDeadA,communityDeadB));

    communitySnapshot.households=HouseholdBook{};
    CHECK(communitySnapshot.households.create(
        991,{loneOrphan.id},0,0.0));

    for(CharacterId caregiverId:{communityAdultA,communityAdultB}){
        auto& toChild=communitySnapshot.relationships.getOrCreate(
            caregiverId,loneOrphan.id);
        toChild.affection=0.60;
        toChild.commitment=0.52;
        toChild.comfort=0.62;
        auto& fromChild=communitySnapshot.relationships.getOrCreate(
            loneOrphan.id,caregiverId);
        fromChild.affection=0.50;
        fromChild.comfort=0.55;
    }

    std::string communityError;
    CHECK(community.restoreSnapshot(communitySnapshot,&communityError));
    CHECK(communityError.empty());
    community.setExternalPhysicalExecution(true);
    community.step();

    PendingContextActionObservation communityPending;
    int communityCareCount=0;
    for(CharacterId caregiverId:{communityAdultA,communityAdultB}){
        const PendingContextActionObservation pending=
            community.observePendingContextAction(caregiverId);
        if(pending.active
           && pending.kind==ContextActionKind::Parenting
           && pending.targetResident==loneOrphan.id){
            ++communityCareCount;
            communityPending=pending;
        }
    }
    CHECK(communityCareCount==1);
    CHECK(communityPending.active);

    GridPos loneOrphanPos{};
    CHECK(community.runtimePosition(loneOrphan.id,loneOrphanPos));
    Character* liveLoneOrphan=findCharacter(community,loneOrphan.id);
    CHECK(liveLoneOrphan!=nullptr);
    const double loneBladderBefore=liveLoneOrphan->needs.bladder;
    CHECK(community.completeExternalContextAction(
        communityPending.actor,communityPending.token,loneOrphanPos));
    CHECK(liveLoneOrphan->needs.bladder<loneBladderBefore);
    CHECK(liveLoneOrphan->parentIds.size()==2);
    CHECK(community.genealogy().relationBetween(
        loneOrphan.id,communityDeadA)==KinshipType::Child);
    CHECK(community.genealogy().relationBetween(
        loneOrphan.id,communityDeadB)==KinshipType::Child);

    std::cout << "competing parenting arbitration passed\n"; return 0;
}
