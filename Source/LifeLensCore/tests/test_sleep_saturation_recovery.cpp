#include <cassert>
#include <iostream>
#include "lifelens/Simulation.h"
#include "lifelens/CoreNavigation.h"
#include "lifelens/SimulationSnapshotCodec.h"
using namespace lifelens;

int main()
{
    // A disconnected patch of dry ground from the long-run trace. The previous
    // contamination-only selector nominates dry land across an impassable gap.
    // This is a regression fixture, not a seed-dependent production policy.
    Simulation sim(874213954); sim.setupNewGame();
    auto state=sim.captureSnapshot();
    const auto id=state.world.characters.back().id;
    const GridPos start{1097,498};
    std::vector<GridPos> route;
    // Reproduce the trace's contamination pressure on reachable alternatives.
    // The dry points north of this patch have lower exposure but no route.
    for(int distance=5;distance<=7;++distance){
        for(const GridPos direction:std::vector<GridPos>{
            {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}}){
            const GridPos point{start.x+direction.x*distance,start.y+direction.y*distance};
            if(buildCoreGroundRoute(state.world,start,point,0,route)){
                state.world.environmentalResidues.deposit(
                    EnvironmentalResidueKind::HumanWaste,point,id,state.world.minute,1.0,1.0,1);
            }
        }
    }
    for(auto& c:state.world.characters){
        c.alive=c.id==id;
        if(c.id==id){c.needs={0.1,0.1,1.0,1.0,0.1}; c.health=HealthState{};}
    }
    state.world.facilities.clear(); state.world.primitiveSanitationSites.clear();
    auto& runtime=state.runtime.at(id);
    runtime.pos=start; runtime.plan.clear(); runtime.pendingContext.clear();
    runtime.navigationHasTarget=false; runtime.navigationArrived=false;
    runtime.navigationRoute.clear(); runtime.navigationRouteIndex=0;
    runtime.penaltyUntilMinute=0;
    std::string error; assert(sim.restoreSnapshot(state,&error));
    const auto original=chooseLowExposureOutdoorReliefPosition(
        sim.world().seed,sim.world().characters.back(),sim.world().environmentalResidues,
        sim.world().minute,start);
    assert(coreGroundTraversable(sim.world(),original));
    assert(!buildCoreGroundRoute(sim.world(),start,original,0,route));
    SanitationUseTarget target;
    assert(sim.sanitationUseTarget(id,target));
    assert(target.kind==SanitationUseTargetKind::EmergencyOutdoor && target.siteId==0);
    assert(buildCoreGroundRoute(sim.world(),start,target.pos,0,route));
    for(int i=0;i<10;++i){
        SanitationUseTarget repeat; assert(sim.sanitationUseTarget(id,repeat));
        assert(sameGridPos(target.pos,repeat.pos));
    }
    GridPos recommendation; assert(sim.recommendedOutdoorReliefPosition(id,recommendation));
    assert(sameGridPos(recommendation,target.pos));
    bool relieved=false,rested=false; GridPos previous=start;
    for(int minute=0;minute<150;++minute){
        sim.step(); GridPos current; assert(sim.runtimePosition(id,current));
        assert(manhattan(previous,current)<=1); previous=current;
        const auto& resident=sim.world().characters.back();
        relieved=relieved || resident.needs.bladder<0.95;
        rested=rested || resident.needs.sleep<0.9;
    }
    assert(relieved && rested);
    // Every inaccessible alternative still permits actual-position relief.
    const auto stranded=chooseLowExposureOutdoorReliefPosition(
        sim.world().seed,sim.world().characters.back(),sim.world().environmentalResidues,
        sim.world().minute,start,[](GridPos){return false;});
    assert(sameGridPos(stranded,start));
    PrimitiveSanitationSite blocked,usable;
    blocked.id=100; blocked.pos={start.x+1,start.y}; blocked.active=true;
    usable=blocked; usable.id=101; usable.pos={start.x+2,start.y};
    const auto alternative=resolveSanitationUseTarget(
        sim.world().seed,sim.world().characters.back(),sim.world().environmentalResidues,
        {blocked,usable},sim.world().minute,start,SettlementServiceRadiusGrid,
        [&](GridPos pos){return sameGridPos(pos,usable.pos);});
    assert(alternative.siteId==usable.id);

    // Instrumentation is opt-in and must not change decisions or save bytes.
    Simulation sampled(2),unsampled(2);
    sampled.setupNewGame(); unsampled.setupNewGame(); sampled.enableSleepDiagnostics(true);
    for(int minute=0;minute<150;++minute){sampled.step();unsampled.step();}
    std::vector<std::uint8_t> sampledBytes,unsampledBytes;
    assert(encodeSimulationSnapshot(sampled.captureSnapshot(),sampledBytes,&error));
    assert(encodeSimulationSnapshot(unsampled.captureSnapshot(),unsampledBytes,&error));
    assert(sampledBytes==unsampledBytes);
    std::cout<<"unreachable sanitation destination no longer traps sleep recovery\n";
}
