#include <cassert>

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

    assert(emergency.completeExternalPhysicalAction(emergencyActorId,true));
    assert(emergency.observeResident(emergencyActorId).activityKind==ObservedActivityKind::Idle);

    const EnvironmentObservation emergencyEnvironment=emergency.observeEnvironment();
    assert(emergencyEnvironment.humanWasteResidues>=1);
    bool foundActorResidue=false;
    for(const auto& residue:emergencyEnvironment.residues){
        if(residue.sourceCharacter==emergencyActorId){
            foundActorResidue=true;
            break;
        }
    }
    assert(foundActorResidue);

    const std::size_t residueCountAfterCompletion=emergencyEnvironment.totalResidues;
    assert(!emergency.completeExternalPhysicalAction(emergencyActorId,true));
    assert(emergency.observeEnvironment().totalResidues==residueCountAfterCompletion);

    // An authored toilet/latrine/world affordance must satisfy the same Core
    // intent without fabricating an outdoor sanitation residue.
    Simulation facility=makeUrgentToiletSimulation(9192);
    const CharacterId facilityActorId=facility.world().characters.front().id;
    const double bladderBeforeFacility=facility.world().characters.front().needs.bladder;
    assert(facility.completeExternalPhysicalAction(facilityActorId,false));
    assert(facility.observeEnvironment().humanWasteResidues==0);
    assert(facility.world().characters.front().needs.bladder<bladderBeforeFacility);

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
