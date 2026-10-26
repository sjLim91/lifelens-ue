#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/ResidentCivilizationOutcomeLearning.h"
#include "lifelens/ResidentSensedResourceAgency.h"
#include "lifelens/ResidentLiveRecoveryEvidence.h"

using namespace lifelens;

static Character makeResident(CharacterId id)
{
    Character actor;
    actor.id=id;
    actor.civilization.character=id;
    actor.alive=true;
    actor.lifeStage=LifeStage::Adult;
    actor.needs={0.05,0.05,0.05,0.05,0.05};
    actor.personality.curiosity=1.0;
    return actor;
}

static GridPos findReachableFixture(World& world)
{
    const GridPos origin=chunkOriginGrid(
        world.initialStartRegion().region.coord);
    for(int y=origin.y+2;y<origin.y+29;++y)
        for(int x=origin.x+2;x<origin.x+28;++x){
            const GridPos current{x,y};
            bool dry=true;
            for(int n=0;n<=4;++n)
                dry=dry &&coreGroundTraversable(world,{x+n,y});
            if(dry) return current;
        }
    assert(false &&"no locally traversable real world cells");
    return {};
}

static ResidentAgencyChoice intendedGather()
{
    ResidentAgencyChoice selection;
    selection.authority=ResidentAgencyAuthority::ResidentCognition;
    selection.cognitiveIntent=CognitiveIntentKind::ImproveFoodSecurity;
    selection.contextFingerprint=880099;
    selection.decision.kind=UnifiedDecisionKind::Civilization;
    selection.decision.civilization.intent=CivilizationIntent::Gather;
    selection.decision.civilization.material=MaterialKind::Stone;
    selection.decision.civilization.item=ItemKind::RawMaterial;
    selection.decision.civilization.resourceNode=101;
    selection.decision.civilization.quantity=1;
    selection.decision.civilization.utility=0.95;
    selection.decision.utility=0.95;
    return selection;
}

static void runPair(WorldSeed seed)
{
    World starting(seed,0,4);
    starting.minute=480; // same legal 15-minute native agency tick
    starting.characters={makeResident(1),makeResident(2)};
    starting.resourceNodes.clear();
    starting.facilities.clear();
    starting.storageSites.clear();

    const GridPos position=findReachableFixture(starting);
    const GridPos oldSource{position.x+4,position.y};
    const GridPos alternative{position.x+1,position.y};
    starting.resourceNodes={
        {101,MaterialKind::Stone,12,12,false,0,oldSource},
        {103,MaterialKind::Stone,12,12,false,0,alternative}
    };
    RelationshipBook relationships;
    const auto strategicChoice=intendedGather();

    // The real Core execution boundary denies an attempted far-away Gather.
    // It cannot grant items and has a diagnosable physical-access cause.
    const auto denied=executeCivilizationDecisionAtPosition(
        starting,starting.characters[0],strategicChoice.decision.civilization,
        position);
    assert(!denied.executed &&!denied.success);
    const auto cause=diagnoseResidentGatherFailure(
        starting,starting.characters[0],strategicChoice.decision.civilization,
        position);
    assert(cause==ResidentGatherFailureCause::ResourceAccessUnavailable);
    assert(starting.resourceNodes[0].quantity==12);
    assert(starting.resourceNodes[1].quantity==12);

    // Exact counterfactual: both worlds share all World truth, RNG, needs,
    // source inventory, actor state and original accepted strategic intent.
    // Only the learner receives the firsthand evidence from the above REAL
    // failed execution. Control has identical experienced physical world but
    // no action-memory receipt, representing the no-learning ablation.
    World withoutReceipt=starting;
    World withReceipt=starting;
    const auto beforeRng=starting.rng;
    rememberCivilizationAttemptOutcome(
        withReceipt.characters[0],strategicChoice.decision.civilization,
        withReceipt.minute,false,cause);
    const auto outcomeIndex=buildResidentResourceOutcomeIndex(
        withReceipt.characters[0],withReceipt.minute);
    assert(outcomeIndex.find(101,"Civilization:Gather:Stone").hasEvidence);
    assert(outcomeIndex.find(101,"Civilization:Gather:Stone").reason
        ==ResidentResourceOutcomeReason::AccessUnavailable);
    assert(withReceipt.characters[1].memory.entries.empty());

    const auto noLearn=chooseResidentSensedResourceGoal(
        withoutReceipt,withoutReceipt.characters[0],position,relationships,
        strategicChoice,nullptr,false);
    const auto learned=chooseResidentSensedResourceGoal(
        withReceipt,withReceipt.characters[0],position,relationships,
        strategicChoice,nullptr,false);

    // The exact provenance inspectable at the Core pending issuance site
    // exists ONLY when firsthand memory explains the substituted source.
    const auto witnessed=observeResidentLiveRecoveryEvidence(
        withReceipt,withReceipt.characters[0],position,strategicChoice,learned);
    assert(witnessed &&witnessed->actor==1
        &&witnessed->failedSource==101 &&witnessed->selectedSource==103
        &&witnessed->firsthandFailureMinute==480
        &&witnessed->selectionMinute==480
        &&witnessed->failedPendingToken==0
        &&!witnessed->directNavigationFailure);
    assert(!observeResidentLiveRecoveryEvidence(
        withoutReceipt,withoutReceipt.characters[0],position,
        strategicChoice,noLearn));

    // Same cognition owner, same material, different real Core source:
    // causal contrast is the only difference between the paired worlds.
    assert(noLearn.agency.decision.civilization.resourceNode==101);
    assert(learned.agency.decision.civilization.resourceNode==103);
    assert(noLearn.agency.authority==ResidentAgencyAuthority::ResidentCognition);
    assert(learned.agency.authority==ResidentAgencyAuthority::ResidentCognition);
    assert(learned.agency.cognitiveIntent==strategicChoice.cognitiveIntent);
    assert(learned.agency.contextFingerprint==strategicChoice.contextFingerprint);
    assert(learned.agency.decision.civilization.material==MaterialKind::Stone);
    assert(withReceipt.rng==beforeRng &&withoutReceipt.rng==beforeRng);
    assert(withReceipt.resourceNodes[0].quantity==withoutReceipt.resourceNodes[0].quantity);
    assert(withReceipt.resourceNodes[1].quantity==withoutReceipt.resourceNodes[1].quantity);

    // The resident does not hallucinate the alternate stock; its recovery
    // goes through real Core navigation and physical material extraction.
    GridPos target{};
    assert(resolveCivilizationResourceAccessGridPosition(withReceipt,103,target));
    std::vector<GridPos> route;
    assert(buildCoreGroundRoute(withReceipt,position,target,1,route));
    const auto realHarvest=executeCivilizationDecisionAtPosition(
        withReceipt,withReceipt.characters[0],
        learned.agency.decision.civilization,target);
    assert(realHarvest.executed &&realHarvest.success &&realHarvest.event.quantity>0);
    assert(withReceipt.resourceNodes[1].quantity==12-realHarvest.event.quantity);
    assert(withReceipt.resourceNodes[0].quantity==12);
    assert(withoutReceipt.resourceNodes[1].quantity==12);
    assert(withoutReceipt.characters[0].civilization.inventory.stacks().empty());

    // No mental state can leak to a neighbour, even with the same choice.
    const auto unaffected=chooseResidentSensedResourceGoal(
        withReceipt,withReceipt.characters[1],position,relationships,
        strategicChoice,nullptr,false);
    assert(unaffected.agency.decision.civilization.resourceNode==101);

    // The genuine later success on source 101 supersedes the older failure.
    withReceipt.minute+=15;
    const auto recoveredAtOriginal=executeCivilizationDecisionAtPosition(
        withReceipt,withReceipt.characters[0],
        strategicChoice.decision.civilization,oldSource);
    assert(recoveredAtOriginal.executed &&recoveredAtOriginal.success);
    rememberCivilizationAttemptOutcome(
        withReceipt.characters[0],strategicChoice.decision.civilization,
        withReceipt.minute,true);
    assert(buildResidentResourceOutcomeIndex(
        withReceipt.characters[0],withReceipt.minute)
        .find(101,"Civilization:Gather:Stone").succeeded);
    const auto repaired=chooseResidentSensedResourceGoal(
        withReceipt,withReceipt.characters[0],position,relationships,
        strategicChoice,nullptr,false);
    assert(repaired.agency.decision.civilization.resourceNode==101);

    std::cout<<"AL1_CAUSAL_PAIR seed="<<seed
             <<" resident=1 core_failed_source=101"
             <<" without_memory=101 with_firsthand_memory=103"
             <<" actual_alternative_harvest="<<realHarvest.event.quantity
             <<" success_repaired=101"
             <<" unchanged_cognition=1 isolated_neighbour=1\n";
}

int main()
{
    runPair(77);
    runPair(73144);
    return 0;
}
