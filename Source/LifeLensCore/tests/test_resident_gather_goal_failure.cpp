#include <cassert>
#include <iostream>
#include "al1_natural_witness.h"
#include "lifelens/ResidentResourceOutcomePreference.h"
#include "lifelens/CoreNavigation.h"

using namespace lifelens;

int main()
{
    // Controlled contention regression, explicitly NOT natural acceptance:
    // start with a genuinely issued natural goal, then place a competing
    // worker at its target and execute a real Core harvest in the fixture.
    // Core must encounter failure, remember it, keep
    // the same predicate and issue/physically complete a reachable alternative.
    Simulation sim(874213954ULL);
    sim.setupNewGame();
    PendingContextActionObservation original;
    for(int tick=0;tick<14*1440 &&!original.active;++tick){
        sim.step();
        for(const auto& actor:sim.world().characters){
            const auto pending=sim.observePendingContextAction(actor.id);
            if(!pending.gatherGoal.active() ||maximumResidentNeed(actor)>0.65
               ||pending.material==MaterialKind::Water)
                continue;
            const auto sensed=observeLocalResourceProximity(
                sim.world(),actor.id,pending.targetPos,ResidentLocalPerceptionMaxRadius);
            for(const auto& other:sensed.nearbyResources){
                if(other.material!=pending.material ||other.nodeId==pending.resourceNode)
                    continue;
                GridPos access{};
                std::vector<GridPos> route;
                if(resolveCivilizationResourceAccessGridPosition(sim.world(),other.nodeId,access)
                   &&buildCoreGroundRoute(sim.world(),pending.targetPos,access,1,route)){
                    original=pending;
                    break;
                }
            }
            if(original.active) break;
        }
    }
    assert(original.active);
    auto fixture=sim.captureSnapshot();
    auto competitor=std::find_if(fixture.world.characters.begin(),fixture.world.characters.end(),
        [&](const Character& c){return c.id!=original.actor &&c.alive;});
    assert(competitor!=fixture.world.characters.end());
    fixture.runtime.at(competitor->id).pos=original.targetPos;
    CivilizationUtilityDecision preceding;
    preceding.intent=CivilizationIntent::Gather;
    preceding.resourceNode=original.resourceNode;
    preceding.material=original.material;
    preceding.quantity=al1_test::resource(sim,original.resourceNode).quantity;
    const auto drained=executeCivilizationDecisionAtPosition(
        fixture.world,*competitor,preceding,original.targetPos);
    assert(drained.executed &&drained.success &&drained.event.quantity>0);
    std::string error;
    const bool restored=sim.restoreSnapshot(fixture,&error);
    if(!restored) std::cerr<<error<<"\n";
    assert(restored);
    PendingContextActionObservation alternative;
    for(int tick=0;tick<120 &&!alternative.active;++tick){
        sim.step();
        const auto p=sim.observePendingContextAction(original.actor);
        if(p.active &&p.gatherGoal.previousToken==original.token) alternative=p;
    }
    assert(alternative.active &&alternative.resourceNode!=original.resourceNode);
    assert(alternative.gatherGoal.goalId==original.gatherGoal.goalId
        &&alternative.gatherGoal.targetQuantity==original.gatherGoal.targetQuantity
        &&alternative.gatherGoal.attempt==2);
    const auto failure=buildResidentResourceOutcomeIndex(
        al1_test::resident(sim,original.actor),sim.world().minute).find(
            original.resourceNode,"Civilization:Gather:"+std::string(materialName(original.material)));
    assert(failure.hasEvidence &&!failure.succeeded
        &&failure.reason==ResidentResourceOutcomeReason::SourceUnavailable);
    const int held=al1_test::resident(sim,original.actor).civilization.inventory.count(
        ItemKind::RawMaterial,original.material);
    const int stock=al1_test::resource(sim,alternative.resourceNode).quantity;
    // Controlled deadline boundary: even a fresh recovery token must not
    // harvest past its original goal's deadline, through either executor.
    auto expired=sim.captureSnapshot();
    expired.runtime.at(original.actor).pendingContext.gatherGoal.expiresMinute=
        expired.world.minute;
    for(bool external:{false,true}){
        expired.world.externalPhysicalExecution=external;
        Simulation overdue(1);
        assert(overdue.restoreSnapshot(expired,&error));
        const auto memoryCount=al1_test::resident(overdue,original.actor).memory.entries.size();
        if(external)
            assert(!overdue.completeExternalContextAction(original.actor,
                alternative.token,alternative.targetPos));
        else overdue.step();
        assert(!overdue.observePendingContextAction(original.actor).active);
        assert(al1_test::resident(overdue,original.actor).civilization.inventory.count(
            ItemKind::RawMaterial,original.material)==held);
        assert(al1_test::resource(overdue,alternative.resourceNode).quantity==stock);
        assert(al1_test::resident(overdue,original.actor).memory.entries.size()==memoryCount);
        bool reported=false;
        for(const auto& event:overdue.logs())
            if(event.find("AL1_CORE_GOAL_RESULT actor=")!=std::string::npos
               &&al1_test::number(event,"pending_token")==alternative.token
               &&al1_test::field(event,"outcome")=="expired") reported=true;
        assert(reported);
    }
    bool finished=false;
    for(int tick=0;tick<120 &&!finished;++tick){
        sim.step();
        for(const auto& event:sim.logs())
            if(event.find("AL1_CORE_GATHER_RESULT actor=")!=std::string::npos
               &&al1_test::number(event,"pending_token")==alternative.token)
                finished=true;
    }
    assert(finished);
    assert(al1_test::resident(sim,original.actor).civilization.inventory.count(
        ItemKind::RawMaterial,original.material)>held);
    assert(al1_test::resource(sim,alternative.resourceNode).quantity<stock);
    std::cout<<"AL1_CONTROLLED_EXHAUSTION_RECOVERY actor="<<original.actor
        <<" goal_id="<<original.gatherGoal.goalId
        <<" failed_token="<<original.token<<" recovery_token="<<alternative.token
        <<" failed_source="<<original.resourceNode<<" alternative="<<alternative.resourceNode
        <<" physical_completion=1 natural_failure_acceptance=NOT_PROVEN\n";
}
