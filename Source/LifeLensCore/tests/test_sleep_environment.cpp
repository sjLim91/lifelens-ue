#include <cassert>
#include <iostream>
#include "lifelens/Simulation.h"
#include "lifelens/CoreNavigation.h"

using namespace lifelens;

static ConstructedFacility facility(FacilityId id,FacilityKind kind,GridPos pos)
{
    ConstructedFacility f;
    f.id=id; f.kind=kind; f.pos=pos; f.active=true;
    f.state=FacilityState::Operational; f.durability=1.0;
    f.initiatedBy=1; f.lastWorkedBy=1; f.startedMinute=0; f.completedMinute=0;
    const auto spec=facilityConstructionSpec(kind);
    f.requiredWork=spec.requiredWork; f.constructionWork=f.requiredWork;
    f.requirements=spec.requirements;
    for(auto& r:f.requirements) r.delivered=r.required;
    return f;
}

static int weatherMinute(const World& world,GridPos pos,bool rain)
{
    for(int minute=0;minute<365*1440;minute+=60){
        const auto w=deriveDynamicEnvironment(world.genesisIdentity(),chunkCoordForGrid(pos),minute);
        const auto e=evaluateSleepEnvironment(w,deriveEnvironmentalConsequences(w),false);
        if(rain ? w.precipitationIntensity01>=0.70 : e.exposure01<0.25) return minute;
    }
    assert(false && "deterministic weather fixture unavailable"); return 0;
}

int main()
{
    DynamicEnvironmentObservation clear; clear.airTemperatureC=19;
    auto dry=evaluateSleepEnvironment(clear,deriveEnvironmentalConsequences(clear),false);
    assert(!dry.weatherProtectionPreferred && dry.recoveryMultiplier01==1.0);
    auto storm=clear; storm.precipitationIntensity01=1; storm.surfaceWetness01=1;
    storm.windIntensity01=1; storm.airTemperatureC=-10;
    const auto exposed=evaluateSleepEnvironment(storm,deriveEnvironmentalConsequences(storm),false);
    const auto protectedSleep=evaluateSleepEnvironment(storm,deriveEnvironmentalConsequences(storm),true);
    assert(exposed.exposedEmergencyOnly && protectedSleep.weatherProtectionPreferred);
    assert(exposed.recoveryMultiplier01<protectedSleep.recoveryMultiplier01);
    assert(DefaultPhysiologyBalance.outdoorSleepRecoveryPerMinute*exposed.recoveryMultiplier01
        >DefaultSimulationRuleset.needs.sleepPerMinute
            +deriveEnvironmentalConsequences(storm).perMinuteNeedsDelta.sleep);
    assert(sleepTravelBudgetCells(1.0)<sleepTravelBudgetCells(0.6));

    Simulation sim(2); sim.setupNewGame();
    auto& world=sim.world(); world.characters.resize(1); world.objects.clear();
    const auto id=world.characters.front().id;
    world.characters.front().needs={0.01,0.01,1.0,0.01,0.01};
    GridPos start; assert(sim.runtimePosition(id,start));
    GridPos bed{},roof{}; bool found=false;
    for(const GridPos direction:std::vector<GridPos>{{1,0},{-1,0},{0,1},{0,-1}}){
        bed={start.x+direction.x*2,start.y+direction.y*2};
        roof={start.x+direction.x*5,start.y+direction.y*5};
        std::vector<GridPos> route;
        if(buildCoreGroundRoute(world,start,roof,0,route) && route.size()==5){found=true;break;}
    }
    assert(found);
    world.facilities={facility(99001,FacilityKind::SleepingPlace,bed),facility(99002,FacilityKind::Shelter,roof)};
    world.minute=weatherMinute(world,start,false);
    GridPos target; FacilityId targetId;
    assert(sim.settlementSleepTarget(id,target,targetId) && targetId==99001);
    world.minute=weatherMinute(world,start,true);
    assert(sim.settlementSleepTarget(id,target,targetId) && targetId==99002);
    for(int i=0;i<10;++i){ FacilityId repeat; assert(sim.settlementSleepTarget(id,target,repeat) && repeat==targetId); }
    assert(sleepRecoveryPerMinuteAt(world,bed,&world.facilities[0])
        <sleepRecoveryPerMinuteAt(world,roof,&world.facilities[1]));
    // A roof elsewhere provides no protection to exposed bedding/ground.
    const double unprotected=sleepRecoveryPerMinuteAt(world,bed,nullptr);
    assert(sleepRecoveryPerMinuteAt(world,bed,&world.facilities[1])==unprotected);
    double previousNeed=world.characters.front().needs.sleep;
    GridPos previous=start; bool arrived=false;
    for(int i=0;i<40;++i){
        sim.step(); GridPos current; assert(sim.runtimePosition(id,current));
        assert(manhattan(previous,current)<=1);
        if(!sameGridPos(current,roof)) assert(world.characters.front().needs.sleep>=previousNeed);
        if(sim.observeResidentPresentation(id).sleepContext==SleepContext::Protected){arrived=true;break;}
        previous=current; previousNeed=world.characters.front().needs.sleep;
    }
    assert(arrived);
    world.characters.front().needs.sleep=0.5;
    world.characters.front().needs.thirst=1.0;
    sim.step(); assert(sim.observeResidentPresentation(id).physicalGoal!=Goal::Sleep);

    // Full Shelter reserves all four slots: choose another usable roof.
    Simulation capacity(2); capacity.setupNewGame();
    auto crowded=capacity.captureSnapshot();
    const auto requester=crowded.world.characters.front().id;
    GridPos home=crowded.runtime.at(requester).pos;
    GridPos firstRoof{home.x+3,home.y},secondRoof{home.x+4,home.y};
    std::vector<GridPos> capacityRoute;
    assert(buildCoreGroundRoute(crowded.world,home,secondRoof,0,capacityRoute));
    crowded.world.facilities={facility(99201,FacilityKind::Shelter,firstRoof),
        facility(99202,FacilityKind::Shelter,secondRoof)};
    Character extra=crowded.world.characters.back(); extra.id=99999;
    extra.civilization.character=extra.id; extra.name="CapacityFixture";
    crowded.world.characters.push_back(extra);
    crowded.runtime[extra.id]=crowded.runtime.at(requester);
    for(std::size_t i=1;i<crowded.world.characters.size();++i){
        auto& r=crowded.runtime.at(crowded.world.characters[i].id);
        r.goal=Goal::Sleep; r.plan={{ActionType::EmergencyUse,0,600}}; r.actionIndex=0;
        r.pos=firstRoof; r.navigationHasTarget=true; r.navigationTarget=firstRoof;
        r.navigationArrived=true; r.pendingContext.clear();
    }
    crowded.world.characters.front().needs={0.01,0.01,1.0,0.01,0.01};
    crowded.world.minute=weatherMinute(crowded.world,home,true);
    std::string error;
    assert(capacity.restoreSnapshot(crowded,&error));
    assert(capacity.settlementSleepTarget(requester,target,targetId) && targetId==99202);

    // Clear-to-storm transition: existing exposed interaction replans at a
    // bounded checkpoint and walks, then stays committed to the roof.
    Simulation transitionSim(2); transitionSim.setupNewGame();
    for(std::size_t i=1;i<transitionSim.world().characters.size();++i) transitionSim.world().characters[i].alive=false;
    transitionSim.world().facilities={facility(99001,FacilityKind::SleepingPlace,bed),facility(99002,FacilityKind::Shelter,roof)};
    auto worsening=transitionSim.captureSnapshot();
    worsening.world.characters.front().needs={0.01,0.01,0.9,0.01,0.01};
    auto& resting=worsening.runtime.at(id);
    resting.goal=Goal::Sleep; resting.plan={{ActionType::EmergencyUse,0,600}};
    resting.actionIndex=0; resting.pos=bed; resting.navigationTarget=bed;
    resting.navigationHasTarget=true; resting.navigationArrived=true;
    resting.pendingContext.clear();
    worsening.world.minute=weatherMinute(worsening.world,bed,true);
    worsening.world.minute+=(15-worsening.world.minute%15)%15;
    if(!transitionSim.restoreSnapshot(worsening,&error)){std::cerr<<error<<"\n";return 1;}
    transitionSim.step();
    auto transition=transitionSim.observeResidentPresentation(id);
    assert(transition.phase==PresentationActionPhase::Moving);
    assert(transition.targetGrid.x==roof.x && transition.targetGrid.y==roof.y);
    for(int i=0;i<25;++i) transitionSim.step();
    assert(transitionSim.observeResidentPresentation(id).sleepContext==SleepContext::Protected);

    Simulation emergency(2); emergency.setupNewGame();
    emergency.world().characters.resize(1); emergency.world().facilities.clear();
    auto& tired=emergency.world().characters.front(); tired.needs={0.01,0.01,1.0,0.01,0.01};
    GridPos outside; assert(emergency.runtimePosition(tired.id,outside));
    emergency.world().minute=weatherMinute(emergency.world(),outside,true);
    emergency.step();
    assert(emergency.observeResidentPresentation(tired.id).sleepContext==SleepContext::ExposedEmergency);
    assert(tired.needs.sleep<1.0);
    // Failed local self-care must not turn its retry cooldown into a ban on
    // emergency rest. Reproduce the long-run saturated fatigue/bladder loop.
    Simulation backoff(2); backoff.setupNewGame();
    auto blocked=backoff.captureSnapshot();
    blocked.world.facilities.clear(); blocked.world.objects.clear();
    for(std::size_t i=1;i<blocked.world.characters.size();++i) blocked.world.characters[i].alive=false;
    auto& blockedResident=blocked.world.characters.front();
    blockedResident.needs={0.2,0.4,1.0,1.0,1.0};
    auto& blockedRuntime=blocked.runtime.at(blockedResident.id);
    blockedRuntime.goal=Goal::Idle; blockedRuntime.plan.clear(); blockedRuntime.actionIndex=0;
    blockedRuntime.pendingContext.clear(); blockedRuntime.penaltyUntilMinute=blocked.world.minute+60;
    const GridPos blockedPosition=blockedRuntime.pos;
    assert(backoff.restoreSnapshot(blocked,&error));
    backoff.step();
    const auto restingDuringBackoff=backoff.observeResidentPresentation(blockedResident.id);
    assert(restingDuringBackoff.physicalGoal==Goal::Sleep);
    assert(restingDuringBackoff.phase==PresentationActionPhase::Interacting);
    GridPos actual; assert(backoff.runtimePosition(blockedResident.id,actual));
    assert(sameGridPos(actual,blockedPosition));
    assert(backoff.world().characters.front().needs.sleep<1.0);
    std::cout<<"weather sleep selection, recovery, travel, interruption and emergency PASS\n";
}
