#include <cassert>
#include <iostream>

#include "lifelens/Simulation.h"
#include "lifelens/SettlementProgression.h"

using namespace lifelens;

namespace {

ConstructedFacility completedFixture(
    FacilityId id,
    FacilityKind kind,
    GridPos pos,
    CharacterId owner,
    int minute)
{
    ConstructedFacility facility=
        makeFacilityConstructionSite(id,kind,pos,owner,minute);
    for(auto& requirement:facility.requirements){
        requirement.delivered=requirement.required;
    }
    facility.constructionWork=facility.requiredWork;
    assert(activateConstructedFacility(facility,owner,minute));
    return facility;
}

int findWeatherMinute(
    World& world,
    GridPos position,
    bool wantHarsh)
{
    const ChunkCoord chunk=chunkCoordForGrid(position);
    const int originalMinute=world.minute;
    constexpr int SearchHorizonMinutes=4*365*SimulationMinutesPerDay;
    for(int offset=0;offset<=SearchHorizonMinutes;offset+=30){
        const int minute=offset;
        const DynamicEnvironmentObservation dynamic=
            deriveDynamicEnvironment(
                world.genesisIdentity(),chunk,minute);
        const EnvironmentalConsequenceProfile consequence=
            deriveEnvironmentalConsequences(dynamic);
        const SleepEnvironmentEvaluation sleep=
            evaluateSleepEnvironment(consequence,false);
        if(wantHarsh){
            if(dynamic.precipitationIntensity01>=0.55
               && sleep.weatherProtectionPreferred){
                world.minute=originalMinute;
                return minute;
            }
        }else if(dynamic.precipitationIntensity01<=0.04
                 && !sleep.weatherProtectionPreferred){
            world.minute=originalMinute;
            return minute;
        }
    }
    world.minute=originalMinute;
    return -1;
}

void setOnlyRequesterAlive(
    Simulation& simulation,
    CharacterId requester)
{
    for(auto& resident:simulation.world().characters){
        if(resident.id==requester) continue;
        resident.alive=false;
        resident.deathMinute=simulation.world().minute;
    }
}

}

int main()
{
    Simulation simulation(770041);
    simulation.setupNewGame();
    World& world=simulation.world();
    Character& sleeper=world.characters.front();
    const CharacterId sleeperId=sleeper.id;
    sleeper.needs={0.01,0.01,0.92,0.01,0.01};

    GridPos start{};
    assert(simulation.runtimePosition(sleeperId,start));

    const GridPos sleepingPlacePos{start.x+2,start.y};
    const GridPos shelterPos{start.x+5,start.y};
    world.facilities.clear();
    world.facilities.push_back(completedFixture(
        88001,FacilityKind::SleepingPlace,sleepingPlacePos,
        sleeperId,world.minute));
    world.facilities.push_back(completedFixture(
        88002,FacilityKind::Shelter,shelterPos,
        sleeperId,world.minute));

    const int mildMinute=findWeatherMinute(world,start,false);
    const int harshMinute=findWeatherMinute(world,start,true);
    assert(mildMinute>=0);
    assert(harshMinute>=0);

    GridPos target{};
    FacilityId targetId=0;

    // Clear weather: a nearby dedicated primitive bed remains a rational target.
    world.minute=mildMinute;
    assert(simulation.settlementSleepTarget(sleeperId,target,targetId));
    assert(targetId==88001);
    assert(target.x==sleepingPlacePos.x && target.y==sleepingPlacePos.y);

    // Strong precipitation: a reachable Shelter wins despite being farther.
    world.minute=harshMinute;
    assert(simulation.settlementSleepTarget(sleeperId,target,targetId));
    assert(targetId==88002);
    assert(target.x==shelterPos.x && target.y==shelterPos.y);

    const double exposedBedRecovery=sleepRecoveryPerMinuteAt(
        world,sleepingPlacePos,&world.facilities[0]);
    const double protectedRecovery=sleepRecoveryPerMinuteAt(
        world,shelterPos,&world.facilities[1]);
    const double unavoidableOutdoorRecovery=sleepRecoveryPerMinuteAt(
        world,start,nullptr);
    assert(protectedRecovery>exposedBedRecovery);
    assert(exposedBedRecovery>=unavoidableOutdoorRecovery);

    // Same authoritative seed/state must choose the same target.
    const FacilityId deterministicId=targetId;
    GridPos deterministicTarget=target;
    target={};
    targetId=0;
    assert(simulation.settlementSleepTarget(sleeperId,target,targetId));
    assert(targetId==deterministicId);
    assert(target.x==deterministicTarget.x);
    assert(target.y==deterministicTarget.y);

    // No Shelter must not make sleep impossible. The exposed primitive bed
    // remains available for a severely fatigued resident.
    world.facilities[1].state=FacilityState::Ruined;
    world.facilities[1].active=false;
    world.facilities[1].durability=0.0;
    target={};
    targetId=0;
    assert(simulation.settlementSleepTarget(sleeperId,target,targetId));
    assert(targetId==88001);

    // With no facility at all, autonomous severe fatigue still reaches the
    // existing outdoor emergency fallback instead of starving Sleep forever.
    world.facilities.clear();
    setOnlyRequesterAlive(simulation,sleeperId);
    sleeper.needs={0.01,0.01,0.98,0.01,0.01};
    bool sawOutdoorEmergencySleep=false;
    for(int minute=0;minute<240 && !sawOutdoorEmergencySleep;++minute){
        simulation.step();
        const ResidentPresentationObservation presentation=
            simulation.observeResidentPresentation(sleeperId);
        if(presentation.active
           && presentation.kind==PresentationActionKind::Physical
           && presentation.physicalGoal==Goal::Sleep
           && presentation.phase==PresentationActionPhase::Interacting){
            sawOutdoorEmergencySleep=
                presentation.sleepContext
                    ==SleepPresentationContext::EmergencyOutdoor;
        }
    }
    assert(sawOutdoorEmergencySleep);

    // Capacity remains authoritative before weather utility. Fill a nearer
    // Shelter's four reservation slots, then verify another usable Shelter is
    // selected rather than the exposed bed.
    Simulation capacitySim(770043);
    capacitySim.setupNewGame();
    World& capacityWorld=capacitySim.world();
    CharacterId requester=capacityWorld.characters.front().id;
    GridPos capacityStart{};
    assert(capacitySim.runtimePosition(requester,capacityStart));
    capacityWorld.minute=findWeatherMinute(capacityWorld,capacityStart,true);
    assert(capacityWorld.minute>=0);

    const GridPos fullShelterPos{capacityStart.x+4,capacityStart.y};
    const GridPos alternateShelterPos{capacityStart.x+8,capacityStart.y};
    const GridPos closeBedPos{capacityStart.x+2,capacityStart.y};
    capacityWorld.facilities.clear();
    capacityWorld.facilities.push_back(completedFixture(
        88101,FacilityKind::SleepingPlace,closeBedPos,
        requester,capacityWorld.minute));
    capacityWorld.facilities.push_back(completedFixture(
        88102,FacilityKind::Shelter,fullShelterPos,
        requester,capacityWorld.minute));
    capacityWorld.facilities.push_back(completedFixture(
        88103,FacilityKind::Shelter,alternateShelterPos,
        requester,capacityWorld.minute));

    SimulationStateSnapshot snapshot=capacitySim.captureSnapshot();
    Character extra=capacityWorld.characters.back();
    extra.id=999991;
    extra.civilization.character=extra.id;
    extra.alive=true;
    extra.deathMinute=-1;
    snapshot.world.characters.push_back(extra);
    SimulationRuntimeSnapshot extraRuntime=
        snapshot.runtime.begin()->second;
    extraRuntime.goal=Goal::Sleep;
    extraRuntime.plan={Action{ActionType::EmergencyUse,0,120}};
    extraRuntime.actionIndex=0;
    extraRuntime.navigationTarget=fullShelterPos;
    extraRuntime.navigationHasTarget=true;
    extraRuntime.navigationArrived=false;
    snapshot.runtime[extra.id]=extraRuntime;

    int reservations=0;
    for(auto& entry:snapshot.runtime){
        if(entry.first==requester || reservations>=4) continue;
        entry.second.goal=Goal::Sleep;
        entry.second.plan={Action{ActionType::EmergencyUse,0,120}};
        entry.second.actionIndex=0;
        entry.second.navigationTarget=fullShelterPos;
        entry.second.navigationHasTarget=true;
        entry.second.navigationArrived=false;
        ++reservations;
    }
    assert(reservations==4);

    std::string error;
    assert(capacitySim.restoreSnapshot(snapshot,&error));
    assert(error.empty());
    target={};
    targetId=0;
    assert(capacitySim.settlementSleepTarget(requester,target,targetId));
    assert(targetId==88103);

    std::cout<<"weather-aware sleep selection and exposed fallback passed\n";
    return 0;
}
