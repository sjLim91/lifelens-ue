#include <cassert>

#include "lifelens/Simulation.h"

using namespace lifelens;

int main()
{
    Simulation simulation(9191);
    simulation.setupNewGame();
    simulation.setExternalPhysicalExecution(true);
    assert(simulation.externalPhysicalExecutionEnabled());

    simulation.world().minute=481;
    for(auto& resident:simulation.world().characters){
        resident.needs={0.01,0.01,0.01,0.01,0.01};
    }

    Character& actor=simulation.world().characters.front();
    const CharacterId actorId=actor.id;
    actor.needs.bladder=0.99;

    simulation.runMinutes(5);
    ResidentObservation pending=simulation.observeResident(actorId);
    assert(pending.activityKind==ObservedActivityKind::Physical);
    assert(pending.physicalGoal==Goal::UseToilet);
    assert(simulation.observeEnvironment().humanWasteResidues==0);

    simulation.runMinutes(20);
    pending=simulation.observeResident(actorId);
    assert(pending.activityKind==ObservedActivityKind::Physical);
    assert(pending.physicalGoal==Goal::UseToilet);
    assert(simulation.observeEnvironment().humanWasteResidues==0);

    assert(simulation.completeExternalPhysicalAction(actorId));
    const ResidentObservation completed=simulation.observeResident(actorId);
    assert(completed.activityKind==ObservedActivityKind::Idle);

    const EnvironmentObservation environment=simulation.observeEnvironment();
    assert(environment.humanWasteResidues>=1);
    bool foundActorResidue=false;
    for(const auto& residue:environment.residues){
        if(residue.sourceCharacter==actorId){
            foundActorResidue=true;
            break;
        }
    }
    assert(foundActorResidue);

    const std::size_t residueCountAfterCompletion=environment.totalResidues;
    assert(!simulation.completeExternalPhysicalAction(actorId));
    assert(simulation.observeEnvironment().totalResidues==residueCountAfterCompletion);

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
