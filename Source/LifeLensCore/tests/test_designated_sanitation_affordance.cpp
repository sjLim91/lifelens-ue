#include <cassert>
#include <string>
#include <vector>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

namespace {

Simulation makeUrgentToiletSimulation(std::uint64_t seed)
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

const PrimitiveSanitationSite* siteById(
    const World& world,
    SanitationSiteId siteId)
{
    return findPrimitiveSanitationSite(world.primitiveSanitationSites,siteId);
}

} // namespace

int main()
{
    // Reproducible personal knowledge makes site establishment a real Craft
    // candidate. Knowledge itself still does not mutate World state.
    World world(8080);
    world.minute=600;
    Character builder;
    builder.id=11;
    builder.civilization.character=builder.id;
    builder.civilization.craftingSkill=0.8;
    builder.personality.conscientiousness=0.8;
    builder.personality.orderliness=0.8;
    builder.civilization.knowledge.learn(
        TechniqueId::DesignatedSanitationArea,
        KnowledgeLevel::Reproducible,
        0.86);

    assert(world.primitiveSanitationSites.empty());
    const CivilizationUtilityDecision craft=bestCraftDecision(world,builder);
    assert(craft.intent==CivilizationIntent::Craft);
    assert(craft.technique==TechniqueId::DesignatedSanitationArea);
    assert(world.primitiveSanitationSites.empty());

    const CivilizationExecutionResult established=
        executeCivilizationDecision(world,builder,craft);
    assert(established.executed);
    assert(established.success);
    assert(established.event.type==CivilizationEventType::Crafted);
    assert(established.event.technique==TechniqueId::DesignatedSanitationArea);
    assert(established.sanitationSiteId!=0);
    assert(world.primitiveSanitationSites.size()==1);
    assert(world.objects.empty());

    const PrimitiveSanitationSite* establishedSite=
        siteById(world,established.sanitationSiteId);
    assert(establishedSite!=nullptr);
    assert(establishedSite->active);
    assert(establishedSite->establishedBy==builder.id);
    assert(establishedSite->pos.x==established.sanitationSitePos.x);
    assert(establishedSite->pos.y==established.sanitationSitePos.y);
    assert(establishedSite->useCount==0);

    // An active site suppresses duplicate establishment. The civilization
    // layer must not multiply facilities merely because several residents know
    // the same technique.
    const CivilizationUtilityDecision duplicate=bestCraftDecision(world,builder);
    assert(duplicate.technique!=TechniqueId::DesignatedSanitationArea);
    const std::size_t siteCountBefore=world.primitiveSanitationSites.size();
    assert(!establishDesignatedSanitationArea(
        world.seed,builder,world.environmentalResidues,
        world.primitiveSanitationSites,world.minute).established);
    assert(world.primitiveSanitationSites.size()==siteCountBefore);

    Character unskilled;
    unskilled.id=12;
    unskilled.civilization.character=unskilled.id;
    World unskilledWorld(8081);
    unskilledWorld.minute=600;
    assert(!canEstablishDesignatedSanitationArea(
        unskilledWorld.seed,unskilled,unskilledWorld.environmentalResidues,
        unskilledWorld.primitiveSanitationSites,unskilledWorld.minute));
    assert(!establishDesignatedSanitationArea(
        unskilledWorld.seed,unskilled,unskilledWorld.environmentalResidues,
        unskilledWorld.primitiveSanitationSites,unskilledWorld.minute).established);
    assert(unskilledWorld.primitiveSanitationSites.empty());

    // Runtime resolution prefers the Core-owned designated site. If that site
    // is invalidated, resolution returns to an unstructured emergency target.
    const SanitationUseTarget designatedTarget=resolveSanitationUseTarget(
        world.seed,builder,world.environmentalResidues,
        world.primitiveSanitationSites,world.minute);
    assert(designatedTarget.kind==SanitationUseTargetKind::DesignatedArea);
    assert(designatedTarget.siteId==established.sanitationSiteId);
    assert(designatedTarget.pos.x==establishedSite->pos.x);
    assert(designatedTarget.pos.y==establishedSite->pos.y);

    PrimitiveSanitationSite* mutableSite=findPrimitiveSanitationSite(
        world.primitiveSanitationSites,established.sanitationSiteId);
    assert(mutableSite!=nullptr);
    mutableSite->active=false;
    const SanitationUseTarget fallbackTarget=resolveSanitationUseTarget(
        world.seed,builder,world.environmentalResidues,
        world.primitiveSanitationSites,world.minute);
    assert(fallbackTarget.kind==SanitationUseTargetKind::EmergencyOutdoor);
    assert(fallbackTarget.siteId==0);

    // External execution must carry the same site identity and GridPos all the
    // way to Core ACK. A stale ID or wrong position fails closed with no needs,
    // use-count or environmental mutation.
    Simulation simulation=makeUrgentToiletSimulation(8181);
    Character& actor=simulation.world().characters.front();
    const CharacterId actorId=actor.id;
    actor.civilization.knowledge.learn(
        TechniqueId::DesignatedSanitationArea,
        KnowledgeLevel::Reproducible,
        0.90);
    const PrimitiveSanitationSiteCreationResult runtimeSite=
        establishDesignatedSanitationArea(
            simulation.world().seed,actor,
            simulation.world().environmentalResidues,
            simulation.world().primitiveSanitationSites,
            simulation.world().minute);
    assert(runtimeSite.established);

    SanitationUseTarget runtimeTarget;
    assert(simulation.sanitationUseTarget(actorId,runtimeTarget));
    assert(runtimeTarget.kind==SanitationUseTargetKind::DesignatedArea);
    assert(runtimeTarget.siteId==runtimeSite.siteId);
    assert(runtimeTarget.pos.x==runtimeSite.pos.x);
    assert(runtimeTarget.pos.y==runtimeSite.pos.y);

    const ResidentObservation pending=simulation.observeResident(actorId);
    assert(pending.activityKind==ObservedActivityKind::Physical);
    assert(pending.physicalGoal==Goal::UseToilet);

    const double bladderBefore=actor.needs.bladder;
    const std::size_t residueBefore=simulation.observeEnvironment().totalResidues;
    assert(!simulation.completeExternalPhysicalAction(
        actorId,false,runtimeTarget.pos,runtimeTarget.siteId+1000));
    assert(actor.needs.bladder==bladderBefore);
    assert(simulation.observeEnvironment().totalResidues==residueBefore);
    assert(siteById(simulation.world(),runtimeTarget.siteId)->useCount==0);

    const GridPos wrongPosition{runtimeTarget.pos.x+1,runtimeTarget.pos.y};
    assert(!simulation.completeExternalPhysicalAction(
        actorId,false,wrongPosition,runtimeTarget.siteId));
    assert(actor.needs.bladder==bladderBefore);
    assert(simulation.observeEnvironment().totalResidues==residueBefore);
    assert(siteById(simulation.world(),runtimeTarget.siteId)->useCount==0);

    assert(simulation.completeExternalPhysicalAction(
        actorId,false,runtimeTarget.pos,runtimeTarget.siteId));
    assert(actor.needs.bladder<bladderBefore);
    const PrimitiveSanitationSite* usedSite=
        siteById(simulation.world(),runtimeTarget.siteId);
    assert(usedSite!=nullptr);
    assert(usedSite->useCount==1);

    GridPos runtimePosition{};
    assert(simulation.runtimePosition(actorId,runtimePosition));
    assert(runtimePosition.x==runtimeTarget.pos.x);
    assert(runtimePosition.y==runtimeTarget.pos.y);

    const EnvironmentObservation environment=simulation.observeEnvironment();
    assert(environment.totalResidues==residueBefore+1);
    bool residueAtSite=false;
    for(const auto& residue:environment.residues){
        if(residue.sourceCharacter==actorId
           && residue.pos.x==runtimeTarget.pos.x
           && residue.pos.y==runtimeTarget.pos.y){
            residueAtSite=true;
            break;
        }
    }
    assert(residueAtSite);

    // The site is full Core state: binary snapshot v5 preserves stable identity,
    // exact GridPos, active flag and accumulated use count.
    const SimulationStateSnapshot captured=simulation.captureSnapshot();
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(captured,bytes,&error));
    assert(error.empty());

    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(error.empty());
    assert(decoded.world.primitiveSanitationSites.size()==1);

    Simulation restored(1);
    assert(restored.restoreSnapshot(decoded,&error));
    assert(error.empty());
    const PrimitiveSanitationSite* restoredSite=
        siteById(restored.world(),runtimeTarget.siteId);
    assert(restoredSite!=nullptr);
    assert(restoredSite->active);
    assert(restoredSite->pos.x==runtimeTarget.pos.x);
    assert(restoredSite->pos.y==runtimeTarget.pos.y);
    assert(restoredSite->useCount==1);

    restored.world().primitiveSanitationSites.front().active=false;
    SanitationUseTarget restoredFallback;
    assert(restored.sanitationUseTarget(actorId,restoredFallback));
    assert(restoredFallback.kind==SanitationUseTargetKind::EmergencyOutdoor);
    assert(restoredFallback.siteId==0);

    return 0;
}
