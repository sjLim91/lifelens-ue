#include <cassert>
#include <string>

#include "lifelens/Simulation.h"

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

    // Standalone Core remains autonomous by default.
    Simulation autonomous(9191);
    autonomous.setupNewGame();
    assert(!autonomous.externalPhysicalExecutionEnabled());
    autonomous.world().minute=481;
    for(auto& resident:autonomous.world().characters){
        resident.needs={0.01,0.01,0.01,0.01,0.01};
    }
    autonomous.world().characters.front().needs.bladder=0.99;
    autonomous.runMinutes(30);
    assert(autonomous.observeEnvironment().humanWasteResidues>=1);

    return 0;
}
