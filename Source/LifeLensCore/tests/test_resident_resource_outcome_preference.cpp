#include "lifelens/ResidentResourceOutcomePreference.h"
#include "lifelens/ResidentCivilizationOutcomeLearning.h"
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
    assert(bestGatherDecisionAtPosition(world,learned,{4,4},nullptr,MaterialKind::Stone).resourceNode==102);
    world.minute=101;
    const auto succeeded=executeCivilizationDecisionAtPosition(world,learned,attempted,{4,4});
    assert(succeeded.executed &&succeeded.success);
    record(learned,101,101,true);
    assert(bestGatherDecisionAtPosition(world,learned,{4,4},nullptr,MaterialKind::Stone).resourceNode==101);
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
}
