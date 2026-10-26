#include "lifelens/ResidentResourceOutcomePreference.h"
#include "lifelens/ResidentGatherHypothesis.h"
#include "lifelens/ResidentCivilizationOutcomeLearning.h"
#include "lifelens/ResidentAIExistingAffordance.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace lifelens;

static std::string gatherKey(MaterialKind material)
{
    CivilizationUtilityDecision decision;
    decision.intent=CivilizationIntent::Gather;
    decision.material=material;
    return civilizationActionExperienceKey(decision);
}

static void record(Character& actor,ResourceNodeId node,int minute,bool success,
    ResidentGatherFailureCause cause=ResidentGatherFailureCause::Unknown,
    MaterialKind material=MaterialKind::Stone)
{
    CivilizationUtilityDecision decision;
    decision.intent=CivilizationIntent::Gather;
    decision.resourceNode=node;
    decision.material=material;
    rememberCivilizationAttemptOutcome(actor,decision,minute,success,cause);
}

int main()
{
    Simulation savedWorld(73144,0,1);
    savedWorld.setupNewGame();
    Character actor=savedWorld.world().characters.front();
    actor.memory.entries.clear();
    actor.beliefs=BeliefState{};
    actor.needs={0.05,0.05,0.05,0.05,0.05};
    const std::string key=gatherKey(MaterialKind::Stone);
    assert(!residentResourceOutcomePreference(actor,101,key,100).hasEvidence);

    record(actor,101,100,false,ResidentGatherFailureCause::SourceExhausted);
    const auto depleted=residentResourceOutcomePreference(actor,101,key,100);
    assert(depleted.hasEvidence &&!depleted.succeeded);
    assert(depleted.reason==ResidentResourceOutcomeReason::SourceUnavailable);
    assert(depleted.utilityAdjustment<0 &&depleted.utilityAdjustment>=-0.08);
    assert(residentResourceOutcomePreference(actor,102,key,100).utilityAdjustment==0.0);
    assert(residentResourceOutcomePreference(actor,101,gatherKey(MaterialKind::Wood),100)
        .utilityAdjustment==0.0);
    // A real source failure is personal, not public World truth.
    auto other=actor;other.id+=1;
    assert(!residentResourceOutcomePreference(other,101,key,100).hasEvidence);
    for(auto& memory:other.memory.entries) memory.sourceCharacter=other.id;
    assert(!residentResourceOutcomePreference(other,101,key,100).hasEvidence);

    Character empty=actor;empty.memory.entries.clear();empty.beliefs=BeliefState{};
    auto removed=empty,access=empty,container=empty,unknown=empty;
    record(removed,101,100,false,ResidentGatherFailureCause::SourceRemoved);
    record(access,101,100,false,ResidentGatherFailureCause::ResourceAccessUnavailable);
    record(container,101,100,false,ResidentGatherFailureCause::ContainerMissing);
    record(unknown,101,100,false);
    assert(residentResourceOutcomePreference(removed,101,key,100).utilityAdjustment
        ==depleted.utilityAdjustment);
    assert(residentResourceOutcomePreference(access,101,key,100).reason
        ==ResidentResourceOutcomeReason::AccessUnavailable);
    assert(residentResourceOutcomePreference(container,101,key,100).reason
        ==ResidentResourceOutcomeReason::ContainerMissing);
    assert(std::abs(residentResourceOutcomePreference(container,101,key,100).utilityAdjustment)
        <std::abs(residentResourceOutcomePreference(unknown,101,key,100).utilityAdjustment));
    assert(std::abs(residentResourceOutcomePreference(unknown,101,key,100).utilityAdjustment)
        <std::abs(residentResourceOutcomePreference(access,101,key,100).utilityAdjustment));
    assert(std::abs(residentResourceOutcomePreference(access,101,key,100).utilityAdjustment)
        <std::abs(depleted.utilityAdjustment));

    // Same-minute firsthand conflict with equal utility is resolved by a
    // stable reason enum, never by memory/snapshot traversal order.
    // This is only a tie-break; genuine later Core outcomes still dominate.
    auto conflicting=empty;
    record(conflicting,101,100,false,ResidentGatherFailureCause::SourceExhausted);
    auto accessFailure=empty;
    record(accessFailure,101,100,false,
        ResidentGatherFailureCause::ResourceAccessUnavailable);
    conflicting.memory.add(accessFailure.memory.entries.front());
    for(auto& episode:conflicting.memory.entries)
        episode.confidence=0.0;
    const auto forwardReason=residentResourceOutcomePreference(
        conflicting,101,key,100).reason;
    std::reverse(conflicting.memory.entries.begin(),conflicting.memory.entries.end());
    const auto reverseReason=residentResourceOutcomePreference(
        conflicting,101,key,100).reason;
    assert(forwardReason==reverseReason);
    assert(forwardReason==ResidentResourceOutcomeReason::SourceUnavailable);
    // A later real success repairs the local conclusion regardless of the
    // same-minute conflict and never gives another resident the memory.
    record(conflicting,101,101,true);
    assert(residentResourceOutcomePreference(conflicting,101,key,101).succeeded);

    // Hearsay, inferred proximity signals, future records and model prose
    // cannot become firsthand evidence even if they mention the same node.
    for(int variant=0;variant<7;++variant){
        auto forged=actor;
        auto& memory=forged.memory.entries.front();
        if(variant==0) memory.source=MemorySource::ToldByOther;
        if(variant==1) memory.source=MemorySource::Inferred;
        if(variant==2) memory.witnessed=false;
        if(variant==3) memory.minute=101;
        if(variant==4) memory.where="model-claimed-resource-node:101";
        if(variant==5) memory.what="the model predicted failure";
        if(variant==6) memory.confidence=std::numeric_limits<double>::quiet_NaN();
        assert(!residentResourceOutcomePreference(forged,101,key,100).hasEvidence);
    }
    assert(!residentResourceOutcomePreference(actor,101,key,99).hasEvidence);
    assert(!residentResourceOutcomePreference(actor,101,key,1541).hasEvidence);
    const auto faded=residentResourceOutcomePreference(actor,101,key,820);
    assert(std::abs(faded.utilityAdjustment)<std::abs(depleted.utilityAdjustment));
    assert(residentResourceOutcomePreference(actor,101,key,1540).utilityAdjustment==0.0);
    assert(!residentResourceOutcomePreference(actor,0,key,100).hasEvidence);
    assert(!residentResourceOutcomePreference(actor,101,"Civilization:Store:Stone",100).hasEvidence);

    // Duplicate records cannot amplify preference. Same-minute success and
    // a later real success repair confidence regardless of memory order.
    auto duplicate=actor;
    duplicate.memory.add(actor.memory.entries.front());
    assert(residentResourceOutcomePreference(duplicate,101,key,100).utilityAdjustment
        ==depleted.utilityAdjustment);
    auto success=empty;
    record(success,101,100,true);
    auto both=actor;
    both.memory.add(success.memory.entries.front());
    assert(residentResourceOutcomePreference(both,101,key,100).succeeded);
    const auto adjustment=residentResourceOutcomePreference(both,101,key,100).utilityAdjustment;
    assert(adjustment>0 &&adjustment<=0.03);
    std::reverse(both.memory.entries.begin(),both.memory.entries.end());
    assert(residentResourceOutcomePreference(both,101,key,100).utilityAdjustment==adjustment);
    record(actor,101,101,true);
    assert(residentResourceOutcomePreference(actor,101,key,101).succeeded);
    // M2-04: observed source/access/container failures must generate
    // different *hypotheses*, never fabricated Core facts or new actions.
    const auto sourceIndex=buildResidentResourceOutcomeIndex(removed,100);
    const auto sourceThought=describeResidentGatherHypothesis(
        sourceIndex,removed.id,100,101,key);
    assert(sourceThought.available
        &&sourceThought.cause==ResidentGatherHypothesisCause::SourceDepletedOrRemoved
        &&sourceThought.nextEvidence
            ==ResidentGatherHypothesisNextEvidence::ObserveSourceStockAgain);
    const auto accessThought=describeResidentGatherHypothesis(
        buildResidentResourceOutcomeIndex(access,100),access.id,100,101,key);
    assert(accessThought.cause==ResidentGatherHypothesisCause::RouteOrAccessBlocked);
    const auto missingVessel=describeResidentGatherHypothesis(
        buildResidentResourceOutcomeIndex(container,100),container.id,100,101,key);
    assert(missingVessel.cause
        ==ResidentGatherHypothesisCause::ContainerPrerequisiteMissing);
    assert(missingVessel.nextEvidence
        ==ResidentGatherHypothesisNextEvidence::ObserveContainerAvailability);
    const auto unknownThought=describeResidentGatherHypothesis(
        buildResidentResourceOutcomeIndex(unknown,100),unknown.id,100,101,key);
    assert(unknownThought.cause
        ==ResidentGatherHypothesisCause::AttemptFailedWithoutKnownCause);
    const auto recoveredThought=describeResidentGatherHypothesis(
        buildResidentResourceOutcomeIndex(actor,101),actor.id,101,101,key);
    assert(recoveredThought.cause
        ==ResidentGatherHypothesisCause::RecoveredByRealSuccess
        &&recoveredThought.nextEvidence==ResidentGatherHypothesisNextEvidence::None);
    assert(!describeResidentGatherHypothesis(
        sourceIndex,removed.id+1,100,101,key).available);
    assert(!describeResidentGatherHypothesis(
        sourceIndex,removed.id,101,101,key).available);
    assert(!describeResidentGatherHypothesis(
        sourceIndex,removed.id,100,102,key).available);
    assert(!describeResidentGatherHypothesis(
        sourceIndex,removed.id,100,101,"Civilization:Gather:Wood").available);
    const auto agedIndex=buildResidentResourceOutcomeIndex(removed,1541);
    assert(!describeResidentGatherHypothesis(
        agedIndex,removed.id,1541,101,key).available);


    // A fresh local Core node contradicts ONLY the claim that an old
    // witnessed source remains depleted. It never fabricates a successful
    // harvest, full-map awareness, path clearance or inventory.
    World reobservedWorld(73144,0,1);
    reobservedWorld.minute=100;
    reobservedWorld.characters={removed};
    reobservedWorld.resourceNodes={
        {101,MaterialKind::Stone,0,20,true,0,{4,4}},
    };
    const auto originalRng=reobservedWorld.rng;
    const auto hypothesis=describeResidentGatherHypothesis(
        buildResidentResourceOutcomeIndex(
            reobservedWorld.characters.front(),100),
        removed.id,100,101,key);
    assert(hypothesis.available);
    assert(reviewResidentGatherHypothesisFromCore(
        reobservedWorld,reobservedWorld.characters.front(),{4,4},hypothesis)
        ==ResidentGatherHypothesisRecheck::StillUnconfirmed);
    reobservedWorld.resourceNodes.front().quantity=20;
    assert(reviewResidentGatherHypothesisFromCore(
        reobservedWorld,reobservedWorld.characters.front(),{4,4},hypothesis)
        ==ResidentGatherHypothesisRecheck::FreshCoreStockContradictsDepletion);
    assert(reviewResidentGatherHypothesisFromCore(
        reobservedWorld,reobservedWorld.characters.front(),{40,40},hypothesis)
        ==ResidentGatherHypothesisRecheck::StillUnconfirmed);
    assert(reviewResidentGatherHypothesisFromCore(
        reobservedWorld,reobservedWorld.characters.front(),{4,4},
        accessThought)==ResidentGatherHypothesisRecheck::StillUnconfirmed);
    assert(reviewResidentGatherHypothesisFromCore(
        reobservedWorld,reobservedWorld.characters.front(),{4,4},
        { })==ResidentGatherHypothesisRecheck::InvalidContext);
    assert(reobservedWorld.rng==originalRng);
    assert(reobservedWorld.characters.front().civilization.inventory.stacks().empty());

    // The planning-pass index stores only real memory source/action keys.
    // Repeated lookups are read-only and never traverse later-added memory.
    {
        auto indexedActor=actor;
        record(indexedActor,202,102,false,ResidentGatherFailureCause::ResourceAccessUnavailable,
            MaterialKind::Wood);
        for(int i=0;i<1000;++i){
            MemoryRecord noise;
            noise.minute=102;
            noise.source=MemorySource::Inferred;
            noise.witnessed=false;
            noise.what="unrelated proximity inference";
            noise.tags={"proximity_resource_inference","node:"+std::to_string(1000+i),"material:3"};
            indexedActor.memory.add(std::move(noise));
        }
        const auto index=buildResidentResourceOutcomeIndex(indexedActor,102);
        assert(index.actor==indexedActor.id &&index.currentMinute==102);
        assert(index.entries.size()==2); // 1000 unrelated records, only two source/action keys
        assert(index.find(101,key).succeeded);
        assert(index.find(202,gatherKey(MaterialKind::Wood)).reason
            ==ResidentResourceOutcomeReason::AccessUnavailable);
        assert(!index.find(202,key).hasEvidence);
        for(std::uint64_t node=1000;node<10000;++node)
            assert(!index.find(node,key).hasEvidence);
        const double cached=index.find(101,key).utilityAdjustment;
        record(indexedActor,101,103,false,ResidentGatherFailureCause::SourceExhausted);
        assert(index.find(101,key).utilityAdjustment==cached);
        assert(buildResidentResourceOutcomeIndex(indexedActor,103)
            .find(101,key).utilityAdjustment<0.0);
        for(const std::string& malformed:{
            std::string("resource-node:0"),std::string("resource-node:0101"),
            std::string("resource-node:101x"),std::string("resource-node:-1"),
            std::string("resource-node:18446744073709551616")}){
            auto bad=empty;
            record(bad,101,100,true);
            bad.memory.entries.front().where=malformed;
            assert(buildResidentResourceOutcomeIndex(bad,100).entries.empty());
        }
        auto largest=empty;
        record(largest,std::numeric_limits<std::uint64_t>::max(),100,true);
        assert(buildResidentResourceOutcomeIndex(largest,100)
            .find(std::numeric_limits<std::uint64_t>::max(),key).succeeded);
    }

    // Real existing Core selection: same world and same goals, different
    // personal executed outcomes choose different live sources. This requires
    // the bounded preference in bestGatherDecisionAtPosition (not a test-only
    // chooser); no accepted/model proposal is needed.
    World world(73144,0,1);
    world.resourceNodes={
        {101,MaterialKind::Stone,0,20,true,20,{4,4}},
        {102,MaterialKind::Stone,20,20,true,20,{4,4}}
    };
    world.storageSites.clear();
    world.facilities.clear();
    world.minute=100;
    auto learned=empty;
    CivilizationUtilityDecision attempted;
    attempted.intent=CivilizationIntent::Gather;
    attempted.resourceNode=101;
    attempted.material=MaterialKind::Stone;
    attempted.quantity=1;
    const auto failed=executeCivilizationDecisionAtPosition(world,learned,attempted,{4,4});
    assert(!failed.executed);
    record(learned,101,100,false,diagnoseResidentGatherFailure(world,learned,attempted,{4,4}));
    world.resourceNodes[0].regenerateDay(); // actual supply recovers; memory stays personal
    assert(world.resourceNodes[0].quantity==20);
    assert(bestGatherDecisionAtPosition(world,empty,{4,4},nullptr,MaterialKind::Stone).resourceNode==101);
    // The global survival/Civilization slot must not be secretly rewritten
    // by a personal memory. Only the single agency's explicit local goal
    // planning pass supplies its same-actor, same-minute learned index.
    assert(bestGatherDecisionAtPosition(world,learned,{4,4},nullptr,MaterialKind::Stone).resourceNode==101);
    const auto initialIndex=buildResidentResourceOutcomeIndex(learned,world.minute);
    assert(bestGatherDecisionAtPosition(world,learned,{4,4},nullptr,MaterialKind::Stone,
        nullptr,&initialIndex).resourceNode==102);
    // A different actor's index cannot be injected as this resident's
    // firsthand evidence.
    auto forgedIndex=initialIndex;
    forgedIndex.actor=learned.id+1;
    assert(bestGatherDecisionAtPosition(world,learned,{4,4},nullptr,MaterialKind::Stone,
        nullptr,&forgedIndex).resourceNode==101);
    world.minute=101;
    const auto succeeded=executeCivilizationDecisionAtPosition(world,learned,attempted,{4,4});
    assert(succeeded.executed &&succeeded.success);
    record(learned,101,101,true);
    const auto resumedIndex=buildResidentResourceOutcomeIndex(learned,world.minute);
    assert(bestGatherDecisionAtPosition(world,learned,{4,4},nullptr,MaterialKind::Stone,
        nullptr,&resumedIndex).resourceNode==101);
    // AL-1: a REAL navigation failure is not a Gather execution result.
    // One resident learns to prefer the reachable second source while the
    // other resident's memories and the actual source stocks stay untouched.
    auto routeLearner=empty;
    routeLearner.memory.entries.clear();
    routeLearner.beliefs=BeliefState{};
    const std::string routeKey=gatherKey(MaterialKind::Stone);
    assert(!rememberResidentGatherRouteFailure(
        routeLearner,routeKey,101,0,101)); // no Core token
    assert(rememberResidentGatherRouteFailure(
        routeLearner,routeKey,101,991,101));
    assert(!rememberResidentGatherRouteFailure(
        routeLearner,routeKey,101,991,101)); // idempotent
    assert(routeLearner.memory.entries.size()==1);
    assert(routeLearner.memory.entries.front().tags.size()==4);
    assert(residentGatherRouteFailurePendingToken(
        routeLearner.memory.entries.front())==991);
    assert(routeLearner.beliefs.find(
        0,actionExperienceProposition(routeKey.c_str()))==nullptr);
    assert(routeLearner.memory.entries.front().tags[0]
        =="navigation_experience");
    assert(routeLearner.memory.entries.front().what
        =="Core 이동 경로 실패로 자원 채집을 시작하지 못했다");
    assert(routeLearner.memory.entries.front().source
        ==MemorySource::DirectWitness);
    assert(routeLearner.memory.entries.front().sourceCharacter
        ==routeLearner.id);
    assert(!residentAIVerifiedGatherOutcome(
        routeLearner,routeLearner.memory.entries.front(),routeKey,101));
    const auto routePreference=buildResidentResourceOutcomeIndex(routeLearner,101);
    assert(routePreference.find(101,routeKey).reason
        ==ResidentResourceOutcomeReason::AccessUnavailable);
    assert(routePreference.find(101,routeKey).utilityAdjustment<0.0);
    assert(!routePreference.find(102,routeKey).hasEvidence);
    world.minute=101;
    assert(bestGatherDecisionAtPosition(world,routeLearner,{4,4},
        nullptr,MaterialKind::Stone,nullptr,&routePreference).resourceNode==102);
    assert(bestGatherDecisionAtPosition(world,empty,{4,4},
        nullptr,MaterialKind::Stone).resourceNode==101);
    assert(rememberResidentGatherRouteFailure(
        routeLearner,routeKey,101,992,102));
    assert(routeLearner.memory.entries.size()==1); // bounded refresh
    assert(residentGatherRouteFailurePendingToken(
        routeLearner.memory.entries.front())==992);
    // A forged token cannot be converted into direct Core route evidence.
    auto corruptToken=routeLearner;
    corruptToken.memory.entries.front().tags[3]="context-token:NaN";
    assert(!buildResidentResourceOutcomeIndex(corruptToken,102)
        .find(101,routeKey).hasEvidence);
    // An older snapshot episode with no token remains learnable but is not
    // falsely reported as having a traceable original pending token.
    auto legacyRoute=routeLearner;
    legacyRoute.memory.entries.front().tags.pop_back();
    assert(residentGatherRouteFailurePendingToken(
        legacyRoute.memory.entries.front())==0);
    assert(buildResidentResourceOutcomeIndex(legacyRoute,102)
        .find(101,routeKey).hasEvidence);
    auto fabricated=routeLearner;
    fabricated.memory.entries.front().source=MemorySource::ToldByOther;
    assert(!buildResidentResourceOutcomeIndex(fabricated,102)
        .find(101,routeKey).hasEvidence);
    auto forgedText=routeLearner;
    forgedText.memory.entries.front().what="model claimed route blocked";
    assert(!buildResidentResourceOutcomeIndex(forgedText,102)
        .find(101,routeKey).hasEvidence);
    // A later confirmed harvest of this same source supersedes spatial
    // uncertainty; a prior blocked path is not perpetual world truth.
    world.minute=103;
    const auto genuineHarvest=executeCivilizationDecisionAtPosition(
        world,routeLearner,attempted,{4,4});
    assert(genuineHarvest.executed &&genuineHarvest.success
        &&genuineHarvest.event.quantity>0);
    record(routeLearner,101,103,true);
    assert(buildResidentResourceOutcomeIndex(routeLearner,103)
        .find(101,routeKey).succeeded);
    assert(!buildResidentResourceOutcomeIndex(routeLearner,
        103+ResidentResourceOutcomeHorizonMinutes+1)
        .find(101,routeKey).hasEvidence);
    world.minute=101;

    // An unseen/exhausted source remains excluded even with positive memory.
    world.resourceNodes[0].quantity=0;
    assert(bestGatherDecisionAtPosition(world,learned,{4,4},nullptr,MaterialKind::Stone).resourceNode==102);

    // Snapshot-backed episodes continue to give exactly the same personal
    // preference after binary roundtrip, without new persistent AI state.
    savedWorld.world().characters.front().memory=learned.memory;
    savedWorld.world().characters.front().beliefs=learned.beliefs;
    // Recorded test memories must be in the saved world's past.
    savedWorld.world().minute=std::max(101,savedWorld.world().minute);
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(savedWorld.captureSnapshot(),bytes,&error));
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    const auto& restored=decoded.world.characters.front();
    const int minute=savedWorld.world().minute;
    assert(residentResourceOutcomePreference(restored,101,key,minute).utilityAdjustment
        ==residentResourceOutcomePreference(learned,101,key,minute).utilityAdjustment);

    // Existing binary snapshot retains the actor's distinct route-failure
    // episode as well as the later physically confirmed success.
    savedWorld.world().characters.front().memory=routeLearner.memory;
    savedWorld.world().characters.front().beliefs=routeLearner.beliefs;
    assert(encodeSimulationSnapshot(savedWorld.captureSnapshot(),bytes,&error));
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    const auto& restoredRoute=decoded.world.characters.front();
    const auto afterLoad=buildResidentResourceOutcomeIndex(
        restoredRoute,savedWorld.world().minute);
    assert(afterLoad.find(101,routeKey).succeeded);
    bool routeEpisodeSurvived=false;
    for(const auto& memory:restoredRoute.memory.entries)
        if(memory.where=="resource-node:101"
           &&memory.tags.size()==4
           &&residentGatherRouteFailurePendingToken(memory)==992
           &&memory.tags[0]=="navigation_experience"
           &&memory.tags[1]==routeKey
           &&memory.tags[2]=="route-failed")
            routeEpisodeSurvived=true;
    assert(routeEpisodeSurvived);
}
