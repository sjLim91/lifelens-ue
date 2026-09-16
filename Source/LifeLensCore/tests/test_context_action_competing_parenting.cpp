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
    for(CharacterId parent:{parentA,parentB}){ auto p=sim.observePendingContextAction(parent); if(p.active && p.kind==ContextActionKind::Parenting && p.parentingTarget==child.id){ ++count; first=p; } }
    CHECK(count==1); CHECK(first.active); CHECK(first.parentingAction==ParentingAction::ToiletAssist);
    sim.runMinutes(30);
    PendingContextActionObservation second; count=0;
    for(CharacterId parent:{parentA,parentB}){ auto p=sim.observePendingContextAction(parent); if(p.active && p.kind==ContextActionKind::Parenting && p.parentingTarget==child.id){ ++count; if(p.actor!=first.actor) second=p; } }
    CHECK(count==2); CHECK(second.active);
    GridPos childPos{}; CHECK(sim.runtimePosition(child.id,childPos));
    Character* childCharacter=findCharacter(sim,child.id); CHECK(childCharacter!=nullptr);
    const double bladderBefore=childCharacter->needs.bladder; const std::size_t residueBefore=sim.world().environmentalResidues.all().size();
    CHECK(sim.completeExternalContextAction(first.actor,first.token,childPos));
    CHECK(childCharacter->needs.bladder<bladderBefore); CHECK(sim.world().environmentalResidues.all().size()==residueBefore+1);
    CHECK(!sim.observePendingContextAction(second.actor).active);
    CHECK(!sim.completeExternalContextAction(second.actor,second.token,childPos));
    CHECK(sim.world().environmentalResidues.all().size()==residueBefore+1);
    std::cout << "competing parenting arbitration passed\n"; return 0;
}
