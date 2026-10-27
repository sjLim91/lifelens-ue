#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/Simulation.h"
#include "lifelens/CoreNavigation.h"
#include "lifelens/ResidentResourceOutcomePreference.h"

using namespace lifelens;

// CONTROLLED runtime branch regression. Its initial pending command and
// navigation-failed flag are restored from a fixture snapshot to enter the
// actual Core failure-handling branch. It is NOT a naturally occurring New
// Game route failure, nor proof of autonomous generation of that failure.
int main()
{
    Simulation sim(77,0,1);
    sim.setupDemo();
    auto snapshot=sim.captureSnapshot();
    assert(snapshot.world.characters.size()==1);
    Character& actor=snapshot.world.characters[0];
    const CharacterId actorId=actor.id;
    assert(actorId!=0 &&actor.alive);
    actor.needs={0.05,0.05,0.05,0.05,0.05};
    const auto runtimeIt=snapshot.runtime.find(actorId);
    assert(runtimeIt!=snapshot.runtime.end());
    auto& rt=runtimeIt->second;
    const GridPos position=rt.pos;

    // Two genuine, finite Core resource nodes, close enough to be sensed.
    // The replacement is routed and extracted by ordinary Simulation::step.
    GridPos sourceOld{position.x+4,position.y};
    GridPos sourceNew{position.x+1,position.y};
    bool pairFound=false;
    for(int y=position.y-5;y<=position.y+5 &&!pairFound;++y)
      for(int x=position.x-5;x<=position.x+5 &&!pairFound;++x){
        const GridPos a{x,y};
        if(perceptionGridDistance(position,a)>7
           ||perceptionGridDistance(position,a)<=2) continue;
        for(const GridPos b:std::vector<GridPos>{
            {position.x+1,position.y},
            {position.x,position.y+1},
            {position.x-1,position.y},
            {position.x,position.y-1}}){
            std::vector<GridPos> route;
            if(!sameGridPos(a,b)
               &&coreGroundTraversable(snapshot.world,b)
               &&buildCoreGroundRoute(snapshot.world,position,b,1,route)){
                sourceOld=a;
                sourceNew=b;
                pairFound=true;
                break;
            }
        }
      }
    assert(pairFound);
    snapshot.world.resourceNodes={
        {101,MaterialKind::Stone,12,12,false,0,sourceOld},
        {103,MaterialKind::Stone,12,12,false,0,sourceNew}
    };
    GridPos oldAccess{},newAccess{};
    assert(resolveCivilizationResourceAccessGridPosition(
        snapshot.world,101,oldAccess));
    assert(resolveCivilizationResourceAccessGridPosition(
        snapshot.world,103,newAccess));
    assert(!gridWithinRadius(position,oldAccess,1));

    const std::uint64_t failedToken=991;
    snapshot.nextContextActionToken=failedToken+1;
    rt.pendingContext={};
    rt.pendingContext.token=failedToken;
    rt.pendingContext.kind=ContextActionKind::Civilization;
    rt.pendingContext.issuedMinute=snapshot.world.minute;
    rt.pendingContext.civilization.intent=CivilizationIntent::Gather;
    rt.pendingContext.civilization.resourceNode=101;
    rt.pendingContext.civilization.material=MaterialKind::Stone;
    rt.pendingContext.civilization.item=ItemKind::RawMaterial;
    rt.pendingContext.civilization.quantity=1;
    rt.pendingContext.civilization.utility=0.85;
    rt.pendingContext.hasSpatialTarget=true;
    rt.pendingContext.targetPos=oldAccess;
    rt.navigationHasTarget=true;
    rt.navigationTarget=oldAccess;
    rt.navigationArrivalRadius=1;
    rt.navigationRouteFailed=true;
    rt.navigationRoute.clear();
    rt.navigationRouteIndex=0;
    rt.plan.clear();

    std::string error;
    assert(sim.restoreSnapshot(snapshot,&error) && error.empty());
    const int failureMinute=sim.world().minute;
    sim.step();
    const auto issued=sim.observePendingContextAction(actorId);
    assert(issued.active &&issued.token==failedToken+1
        &&issued.kind==ContextActionKind::Civilization
        &&issued.civilizationIntent==CivilizationIntent::Gather
        &&issued.resourceNode==103
        &&issued.material==MaterialKind::Stone);
    const auto& coreActor=sim.world().characters.front();
    assert(sim.world().resourceNodes[0].quantity==12);
    assert(sim.world().resourceNodes[1].quantity==12);
    bool directRouteMemory=false;
    for(const auto& memory:coreActor.memory.entries)
        if(memory.source==MemorySource::DirectWitness
           &&memory.witnessed &&memory.who==actorId
           &&memory.sourceCharacter==actorId
           &&memory.minute==failureMinute
           &&memory.where=="resource-node:101"
           &&residentGatherRouteFailurePendingToken(memory)==failedToken)
            directRouteMemory=true;
    assert(directRouteMemory);

    bool issuedLog=false;
    for(const auto& log:sim.logs())
        if(log.find("AL1_CORE_RESOURCE_REPLAN actor="
            +std::to_string(actorId))!=std::string::npos
           &&log.find(" failed_pending_token=991 pending_token=992")
               !=std::string::npos
           &&log.find(" selected_source=103")!=std::string::npos)
            issuedLog=true;
    assert(issuedLog);

    bool realHarvest=false;
    for(int tick=0;tick<100;++tick){
        sim.step();
        if(sim.world().resourceNodes[1].quantity>=12) continue;
        const auto& resident=sim.world().characters.front();
        bool directHarvestMemory=false;
        for(const auto& memory:resident.memory.entries)
            if(memory.source==MemorySource::DirectWitness
               &&memory.witnessed
               &&memory.sourceCharacter==actorId
               &&memory.who==actorId
               &&memory.where=="resource-node:103"
               &&memory.tags.size()==3
               &&memory.tags[0]=="action_experience"
               &&memory.tags[2]=="success")
                directHarvestMemory=true;
        if(!directHarvestMemory) continue;
        for(const auto& log:sim.logs())
            if(log.find("AL1_CORE_GATHER_RESULT actor="
                +std::to_string(actorId))!=std::string::npos
               &&log.find(" pending_token=992 source=103")
                   !=std::string::npos){
                realHarvest=true;
                break;
            }
        if(realHarvest) break;
    }
    assert(realHarvest);
    assert(sim.world().resourceNodes[0].quantity==12);
    assert(sim.world().resourceNodes[1].quantity<12);
    std::cout<<"AL1_CONTROLLED_RUNTIME_ROUTE_RECOVERY"
             <<" actor="<<actorId
             <<" failed_token="<<failedToken
             <<" recovery_token="<<(failedToken+1)
             <<" original_source_remaining=12"
             <<" replacement_harvested="
             <<(12-sim.world().resourceNodes[1].quantity)
             <<" NOT_A_NATURAL_FAILURE_WITNESS=1\n";
}
