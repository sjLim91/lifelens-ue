#include <cassert>
#include <string>

#include "lifelens/Simulation.h"
#include "lifelens/SimulationCalendar.h"

using namespace lifelens;

static Simulation makeUrgentToiletSimulation(std::uint64_t seed)
{
    Simulation simulation(seed);
    simulation.setupNewGame();
    simulation.setExternalPhysicalExecution(true);
    simulation.world().minute=481;
    for(auto& resident:simulation.world().characters){
        resident.needs={0.01,0.01,0.01,0.01,0.01};
    }
    simulation.world().characters.front().needs.bladder=0.99;
    simulation.runMinutes(5);
    return simulation;
}

int main()
{
    Simulation emergency=makeUrgentToiletSimulation(9191);
    assert(emergency.externalPhysicalExecutionEnabled());
    const CharacterId emergencyActorId=emergency.world().characters.front().id;

    ResidentObservation pending=emergency.observeResident(emergencyActorId);
    assert(pending.activityKind==ObservedActivityKind::Physical);
    assert(pending.physicalGoal==Goal::UseToilet);
    assert(emergency.observeEnvironment().humanWasteResidues==0);

    emergency.runMinutes(20);
    pending=emergency.observeResident(emergencyActorId);
    assert(pending.activityKind==ObservedActivityKind::Physical);
    assert(pending.physicalGoal==Goal::UseToilet);
    assert(emergency.observeEnvironment().humanWasteResidues==0);

    const GridPos emergencyResolvedPosition{7,-4};
    assert(emergency.completeExternalPhysicalAction(
        emergencyActorId,true,emergencyResolvedPosition));
    assert(emergency.observeResident(emergencyActorId).activityKind==ObservedActivityKind::Idle);

    GridPos authoritativePosition{};
    assert(emergency.runtimePosition(emergencyActorId,authoritativePosition));
    assert(authoritativePosition.x==emergencyResolvedPosition.x);
    assert(authoritativePosition.y==emergencyResolvedPosition.y);

    const EnvironmentObservation emergencyEnvironment=emergency.observeEnvironment();
    assert(emergencyEnvironment.humanWasteResidues>=1);
    bool foundActorResidue=false;
    for(const auto& residue:emergencyEnvironment.residues){
        if(residue.sourceCharacter==emergencyActorId){
            foundActorResidue=true;
            assert(residue.pos.x==emergencyResolvedPosition.x);
            assert(residue.pos.y==emergencyResolvedPosition.y);
            break;
        }
    }
    assert(foundActorResidue);

    // Runtime position is already part of the authoritative Core snapshot. A
    // restored runtime must expose exactly the same grid position for Unreal to
    // project back into world space without storing a second transform authority.
    const SimulationStateSnapshot saved=emergency.captureSnapshot();
    Simulation restored(1);
    std::string restoreError;
    assert(restored.restoreSnapshot(saved,&restoreError));
    assert(restoreError.empty());
    GridPos restoredPosition{};
    assert(restored.runtimePosition(emergencyActorId,restoredPosition));
    assert(restoredPosition.x==emergencyResolvedPosition.x);
    assert(restoredPosition.y==emergencyResolvedPosition.y);

    const std::size_t residueCountAfterCompletion=emergencyEnvironment.totalResidues;
    assert(!emergency.completeExternalPhysicalAction(
        emergencyActorId,true,emergencyResolvedPosition));
    assert(emergency.observeEnvironment().totalResidues==residueCountAfterCompletion);

    // An authored toilet/latrine/world affordance must satisfy the same Core
    // intent without fabricating an outdoor sanitation residue. Production
    // external completion must also drive the same authoritative emotion relief
    // as the standalone Core physical execution path.
    Simulation facility=makeUrgentToiletSimulation(9192);
    const CharacterId facilityActorId=facility.world().characters.front().id;
    const double bladderBeforeFacility=facility.world().characters.front().needs.bladder;
    const double reliefBeforeFacility=facility.world().characters.front().emotion.relief;
    const GridPos facilityResolvedPosition{2,3};
    assert(facility.completeExternalPhysicalAction(
        facilityActorId,false,facilityResolvedPosition));
    assert(facility.observeEnvironment().humanWasteResidues==0);
    assert(facility.world().characters.front().needs.bladder<bladderBeforeFacility);
    assert(facility.world().characters.front().emotion.relief>reliefBeforeFacility);
    GridPos facilityPosition{};
    assert(facility.runtimePosition(facilityActorId,facilityPosition));
    assert(facilityPosition.x==facilityResolvedPosition.x);
    assert(facilityPosition.y==facilityResolvedPosition.y);

    // External/native sleep is not an atomic presentation ACK. The physical
    // executor first reaches a real use point, then Core simulation minutes
    // reduce fatigue one minute at a time until the resident actually wakes.
    // This keeps native observation consistent with headless/Web sleep.
    Simulation settlementSleep(9193);
    settlementSleep.setupNewGame();
    settlementSleep.setExternalPhysicalExecution(true);
    Character& sleeper=settlementSleep.world().characters.front();
    const CharacterId sleeperId=sleeper.id;
    GridPos sleeperStart{};
    assert(settlementSleep.runtimePosition(sleeperId,sleeperStart));
    const GridPos bedPosition{sleeperStart.x+5,sleeperStart.y+2};
    ConstructedFacility bed=makeFacilityConstructionSite(
        99001,
        FacilityKind::SleepingPlace,
        bedPosition,
        sleeperId,
        settlementSleep.world().minute);
    for(auto& requirement:bed.requirements){
        requirement.delivered=requirement.required;
    }
    bed.constructionWork=bed.requiredWork;
    assert(activateConstructedFacility(
        bed,0,settlementSleep.world().minute));
    settlementSleep.world().facilities.push_back(bed);
    for(auto& resident:settlementSleep.world().characters){
        resident.needs={0.01,0.01,0.01,0.01,0.01};
    }
    sleeper.needs.sleep=0.99;

    bool sleepPending=false;
    for(int minute=0;minute<30 && !sleepPending;++minute){
        settlementSleep.step();
        const ResidentObservation observed=
            settlementSleep.observeResident(sleeperId);
        sleepPending=
            observed.activityKind==ObservedActivityKind::Physical
            && observed.physicalGoal==Goal::Sleep;
    }
    assert(sleepPending);

    // External movement/approach is not sleep time. Until the world executor
    // confirms arrival, fatigue may accrue but must never improve.
    const double fatigueBeforeArrival=sleeper.needs.sleep;
    settlementSleep.step();
    assert(sleeper.needs.sleep>=fatigueBeforeArrival);

    GridPos sleepTarget{};
    FacilityId sleepFacilityId=0;
    assert(settlementSleep.settlementSleepTarget(
        sleeperId,sleepTarget,sleepFacilityId));
    assert(sleepFacilityId==99001);
    assert(sleepTarget.x==bedPosition.x);
    assert(sleepTarget.y==bedPosition.y);

    const double fatigueAtArrival=sleeper.needs.sleep;
    assert(settlementSleep.beginExternalSleepUse(
        sleeperId,sleepTarget));

    GridPos arrivedPosition{};
    assert(settlementSleep.runtimePosition(sleeperId,arrivedPosition));
    assert(arrivedPosition.x==sleepTarget.x);
    assert(arrivedPosition.y==sleepTarget.y);

    // A legacy completion ACK cannot double-apply a whole sleep session once
    // progressive external sleep has begun.
    assert(!settlementSleep.completeExternalPhysicalAction(
        sleeperId,true,sleepTarget));

    settlementSleep.runMinutes(60);
    assert(sleeper.needs.sleep<fatigueAtArrival);
    assert(sleeper.needs.sleep>RestedSleepNeedTarget);
    ResidentObservation sleepingAfterHour=
        settlementSleep.observeResident(sleeperId);
    assert(sleepingAfterHour.activityKind==ObservedActivityKind::Physical);
    assert(sleepingAfterHour.physicalGoal==Goal::Sleep);
    assert(settlementSleep.world().facilities.front().usageCount==0);

    bool wokeAfterElapsedRest=false;
    for(int minute=0;
        minute<MaximumSleepSessionMinutes && !wokeAfterElapsedRest;
        ++minute){
        settlementSleep.step();
        const ResidentObservation observed=
            settlementSleep.observeResident(sleeperId);
        wokeAfterElapsedRest=!(
            observed.activityKind==ObservedActivityKind::Physical
            && observed.physicalGoal==Goal::Sleep);
    }
    assert(wokeAfterElapsedRest);
    assert(sleeper.needs.sleep<0.20);
    assert(settlementSleep.world().facilities.front().usageCount==1);
    assert(settlementSleep.world().facilities.front().lastUsedMinute>=
        settlementSleep.world().facilities.front().completedMinute);

    // Standalone Core remains autonomous by default.
    Simulation autonomous(9191);
    autonomous.setupNewGame();
    assert(!autonomous.externalPhysicalExecutionEnabled());
    autonomous.world().minute=481;
    for(auto& resident:autonomous.world().characters){
        resident.needs={0.01,0.01,0.01,0.01,0.01};
    }
    autonomous.world().characters.front().needs.bladder=0.99;
    for(int minute=0;
        minute<SimulationMinutesPerDay
        && autonomous.observeEnvironment().humanWasteResidues==0;
        ++minute){
        autonomous.step();
    }
    assert(autonomous.observeEnvironment().humanWasteResidues>=1);

    return 0;
}
