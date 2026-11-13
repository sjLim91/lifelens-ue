#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/CoreNavigation.h"
#include "fixtures/cognitive_causal_export.h"
#include "lifelens/ResidentCivilizationOutcomeLearning.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

#define REQUIRE(condition) do { \
    if(!(condition)){ \
        std::cerr<<"AL2_PRODUCT_TICK_FAILED line="<<__LINE__ \
                 <<" check=" #condition <<std::endl; \
        return 1; \
    } \
} while(false)

static bool realFirsthandReceipt(const Character& resident,
    const std::string& actionKey,bool success,
    const std::string& location)
{
    for(const auto& memory:resident.memory.entries){
        if(memory.source!=MemorySource::DirectWitness
           ||!memory.witnessed ||memory.sourceCharacter!=resident.id
           ||memory.who!=resident.id ||memory.minute<480
           ||memory.tags.size()!=3
           ||memory.tags[0]!="action_experience"
           ||memory.tags[1]!=actionKey
           ||memory.tags[2]!=(success?"success":"failure")
           ||memory.where!=location)
            continue;
        const std::string prefix="실제 문명 행동 "+actionKey;
        if(memory.what==prefix+(success
                ?" 수행에 성공했다"
                :" 수행을 시도했지만 실패했다"))
            return true;
    }
    return false;
}

int main()
{
    // Controlled Core live-tick AL-2 failure-and-recovery fixture: another
    // real resident physically consumes the selected finite resource source
    // through the ordinary Core executor while this builder is en route.
    // The builder itself is NEVER manually executed: every attempt, failed
    // token, successful alternative gather, craft and direct memory comes
    // from actual Simulation::step(). This is NOT a spontaneous New Game.
    SimulationRuleset rules=DefaultSimulationRuleset;
    rules.needs.hungerPerMinute=0.0;
    rules.needs.thirstPerMinute=0.0;
    rules.needs.sleepPerMinute=0.0;
    rules.needs.bladderPerMinute=0.0;
    rules.needs.hygienePerMinute=0.0;
    constexpr WorldSeed seed=874213954ULL;
    Simulation original(seed,0,CurrentWorldGenerationVersion,rules);
    original.setupNewGame();
    World& world=original.world();
    REQUIRE(world.minute==480 &&world.characters.size()==4);
    const CharacterId actorId=world.characters.front().id;
    GridPos origin{};
    REQUIRE(original.runtimePosition(actorId,origin));

    for(auto& resident:world.characters)
        resident.needs={0.03,0.03,0.03,0.03,0.03};
    Character& actor=world.characters.front();
    actor.civilization.inventory=Inventory{};
    actor.civilization.knowledge=KnowledgeState{};
    actor.civilization.knowledge.learn(
        TechniqueId::DiggingStick,KnowledgeLevel::Reproducible,1.0);
    actor.personality.curiosity=0.95;
    // Ecology snapshots require disabled wildlife to have no live populations.
    // Clear the controlled fixture's animals together with its feature flag.
    world.wildlifePopulations.clear();
    world.wildlifeEnabled=false;
    // Preserve ALL immutable generated resource-patch IDs and capacities.
    // Snapshot validation rejects a New Game whose generated chunks refer
    // to missing resource nodes. Deplete the controlled fixture's natural
    // stock instead, including matching per-tree wood accounting; never
    // delete an authoritative generated resource identity.
    for(auto& node:world.resourceNodes){
        node.quantity=0;
        for(auto& tree:node.trees)
            tree.remainingWood=0;
    }
    world.storageSites.clear();
    world.facilities.clear();

    // A finite Core resource source at a reachable local spot. The planner
    // never receives a virtual Wood item or a hand-executed Gather.
    GridPos sourcePos=origin;
    bool foundRoute=false;
    for(int distance=3;distance>=2 &&!foundRoute;--distance){
        const GridPos candidates[]={
            {origin.x+distance,origin.y},
            {origin.x,origin.y+distance},
            {origin.x-distance,origin.y},
            {origin.x,origin.y-distance}
        };
        for(const auto candidate:candidates){
            std::vector<GridPos> route;
            if(coreGroundTraversable(world,candidate)
               &&buildCoreGroundRoute(world,origin,candidate,1,route)){
                sourcePos=candidate;
                foundRoute=true;
                break;
            }
        }
    }
    REQUIRE(foundRoute);
    ResourceNode wood;
    wood.id=9000101;
    wood.material=MaterialKind::Wood;
    wood.pos=sourcePos;
    wood.quantity=80;
    wood.maxQuantity=80;
    wood.renewable=false;
    world.resourceNodes.push_back(wood);
    ResourceNode alternative=wood;
    alternative.id=9000102;
    bool alternativeAvailable=false;
    for(int distance=3;distance<=6 &&!alternativeAvailable;++distance){
        for(const GridPos candidate:std::vector<GridPos>{
            {origin.x-distance,origin.y},
            {origin.x,origin.y-distance},
            {origin.x+distance,origin.y},
            {origin.x,origin.y+distance}}){
            if(candidate.x==sourcePos.x &&candidate.y==sourcePos.y)
                continue;
            std::vector<GridPos> viable;
            if(coreGroundTraversable(world,candidate)
               &&buildCoreGroundRoute(world,origin,candidate,1,viable)){
                alternative.pos=candidate;
                alternativeAvailable=true;
                break;
            }
        }
    }
    REQUIRE(alternativeAvailable);
    world.resourceNodes.push_back(alternative);
    const auto sourceStock=[&](ResourceNodeId id){
        const auto found=std::find_if(
            world.resourceNodes.begin(),world.resourceNodes.end(),
            [&](const ResourceNode& node){return node.id==id;});
        return found==world.resourceNodes.end() ? -1 : found->quantity;
    };
    REQUIRE(sourceStock(wood.id)==wood.quantity);
    REQUIRE(sourceStock(alternative.id)==alternative.quantity);
    GridPos access{};
    REQUIRE(resolveCivilizationResourceAccessGridPosition(
        world,wood.id,access));
    std::vector<GridPos> accessRoute;
    REQUIRE(buildCoreGroundRoute(world,origin,access,1,accessRoute));
    REQUIRE(actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood)==0);
    REQUIRE(actor.civilization.inventory.count(
        ItemKind::DiggingStick,MaterialKind::Wood)==0);

    const auto request=original.observeCognitiveRequest(
        actorId,CognitiveTriggerKind::Reflection);
    REQUIRE(request.actor==actorId &&request.minute==480);
    CognitiveProposal proposal;
    proposal.actor=actorId;
    proposal.intent=CognitiveIntentKind::CraftUsefulTools;
    proposal.priority=0.96;
    REQUIRE(original.acceptCognitiveProposal(
        request,proposal,"controlled-local-cognition","al2-runtime-fixture"));
    REQUIRE(original.acceptedCognitiveDecisions().size()==1);

    std::uint64_t failedToken=0;
    std::uint64_t gatherToken=0;
    std::uint64_t craftToken=0;
    ResourceNodeId failedSource=0;
    ResourceNodeId successfulSource=0;
    int removedByCompetitor=0;
    CivilizationUtilityDecision gatherDecision;
    CivilizationUtilityDecision craftDecision;
    int firstGatherMinute=-1;
    int craftMinute=-1;
    int gatherUnits=0;
    int travelCells=0;
    bool savedAfterPhysicalGather=false;
    bool savedBeforePhysicalFailure=false;
    Simulation replay(seed,0,CurrentWorldGenerationVersion,rules);
    Simulation preFailureReplay(seed,0,CurrentWorldGenerationVersion,rules);
    std::string error;

    for(int elapsed=0;elapsed<500;++elapsed){
        GridPos beforePos{};
        REQUIRE(original.runtimePosition(actorId,beforePos));
        const int stockA=sourceStock(wood.id);
        const int stockB=sourceStock(alternative.id);
        original.step();
        if(savedAfterPhysicalGather)
            replay.step();
        if(savedBeforePhysicalFailure)
            preFailureReplay.step();
        GridPos afterPos{};
        REQUIRE(original.runtimePosition(actorId,afterPos));
        if(afterPos.x!=beforePos.x ||afterPos.y!=beforePos.y)
            ++travelCells;

        const auto pending=original.observePendingContextAction(actorId);
        if(pending.active &&pending.kind==ContextActionKind::Civilization){
            const auto state=original.captureSnapshot();
            const auto it=state.runtime.find(actorId);
            REQUIRE(it!=state.runtime.end());
            const auto& issued=it->second.pendingContext;
            if(issued.civilization.intent==CivilizationIntent::Gather
               &&issued.civilization.material==MaterialKind::Wood
               &&(issued.civilization.resourceNode==wood.id
                   ||issued.civilization.resourceNode==alternative.id)){
                if(failedToken==0){
                    failedToken=issued.token;
                    failedSource=issued.civilization.resourceNode;
                    REQUIRE(issued.cognitiveGoal.active());
                    REQUIRE(issued.cognitiveGoal.token==failedToken);
                    REQUIRE(issued.cognitiveGoal.actor==actorId);
                    REQUIRE(issued.cognitiveGoal.contextFingerprint
                        ==cognitiveRequestFingerprint(request));
                    REQUIRE(issued.cognitiveGoal.thoughtMinute==request.minute);
                    // Core issued the builder's physical trip. A separate
                    // resident independently uses the ordinary Core gather
                    // executor at the node while the builder is travelling.
                    // This physically debits finite stock, rather than
                    // assigning quantity=0 or inventing a failed receipt.
                    Character& competitor=world.characters[1];
                    CivilizationUtilityDecision competing=issued.civilization;
                    competing.quantity=32;
                    GridPos accessSite{};
                    REQUIRE(resolveCivilizationResourceAccessGridPosition(
                        world,failedSource,accessSite));
                    for(int attempt=0;attempt<100
                            &&sourceStock(failedSource)>0;++attempt){
                        const auto result=executeCivilizationDecisionAtPosition(
                            world,competitor,competing,accessSite);
                        REQUIRE(result.executed &&result.success
                            &&result.event.quantity>0);
                        removedByCompetitor+=result.event.quantity;
                    }
                    REQUIRE(removedByCompetitor>0
                        &&sourceStock(failedSource)==0);
                    // The competing resident exhausted the REAL stock, but
                    // our worker has not physically arrived. Save this very
                    // in-flight, accepted-thought-owned command and restore
                    // it into a second production Simulation. No token,
                    // action memory or source choice may be injected.
                    auto liveState=original.captureSnapshot();
                    const auto& liveLink=liveState.runtime.at(actorId)
                        .pendingContext.cognitiveGoal;
                    REQUIRE(liveLink.active() &&liveLink.token==failedToken);
                    auto forged=liveState;
                    forged.runtime.at(actorId).pendingContext.cognitiveGoal
                        .contextFingerprint^=1ULL;
                    std::vector<std::uint8_t> invalidBytes;
                    std::string invalidReason;
                    REQUIRE(!encodeSimulationSnapshot(
                        forged,invalidBytes,&invalidReason));
                    REQUIRE(!invalidReason.empty());
                    REQUIRE(!preFailureReplay.restoreSnapshot(
                        forged,&invalidReason));
                    // Emulate a genuine older v8 save with no optional
                    // cognition-action tail. Its accepted journal and
                    // physical pending command are unchanged, but Core must
                    // not invent an action attribution on legacy decode.
                    auto legacy=liveState;
                    legacy.runtime.at(actorId).pendingContext.cognitiveGoal.clear();
                    std::vector<std::uint8_t> legacyBytes;
                    REQUIRE(encodeSimulationSnapshot(
                        legacy,legacyBytes,&error));
                    SimulationStateSnapshot legacyDecoded;
                    REQUIRE(decodeSimulationSnapshot(
                        legacyBytes,legacyDecoded,&error));
                    REQUIRE(!legacyDecoded.runtime.at(actorId)
                        .pendingContext.cognitiveGoal.active());
                    std::vector<std::uint8_t> beforeFailureBytes;
                    REQUIRE(encodeSimulationSnapshot(
                        liveState,beforeFailureBytes,&error));
                    SimulationStateSnapshot decodedBeforeFailure;
                    REQUIRE(decodeSimulationSnapshot(
                        beforeFailureBytes,decodedBeforeFailure,&error));
                    const auto& restoredLink=decodedBeforeFailure
                        .runtime.at(actorId).pendingContext.cognitiveGoal;
                    REQUIRE(restoredLink.token==failedToken);
                    REQUIRE(restoredLink.contextFingerprint
                        ==cognitiveRequestFingerprint(request));
                    REQUIRE(preFailureReplay.restoreSnapshot(
                        decodedBeforeFailure,&error));
                    savedBeforePhysicalFailure=true;
                }else if(issued.civilization.resourceNode!=failedSource
                         &&gatherToken==0){
                    gatherToken=issued.token;
                    successfulSource=issued.civilization.resourceNode;
                    gatherDecision=issued.civilization;
                }
            }
            if(issued.civilization.intent==CivilizationIntent::Craft
               &&issued.civilization.facilityAction==FacilityBuildAction::None
               &&issued.civilization.technique==TechniqueId::DiggingStick
               &&craftToken==0){
                craftToken=issued.token;
                craftDecision=issued.civilization;
            }
        }
        const auto activity=original.observeResidentCivilizationActivity(actorId);
        // Presentation may retain the last Core action for more than one
        // observation. Credit a physical quantity delta exactly once, only
        // when the action was actually executed in the just-finished tick.
        const bool actionExecutedThisTick=activity.active
            &&activity.minute==original.world().minute-1;
        if(actionExecutedThisTick &&activity.success
           &&activity.kind==CivilizationActivityKind::Gather
           &&activity.resourceNode==successfulSource){
            REQUIRE(gatherToken!=0 &&successfulSource!=failedSource);
            REQUIRE(activity.quantity>0);
            const int stockBefore=successfulSource==wood.id?stockA:stockB;
            REQUIRE(stockBefore-sourceStock(successfulSource)
                >=activity.quantity);
            gatherUnits+=activity.quantity;
            if(firstGatherMinute<0)
                firstGatherMinute=activity.minute;
            if(!savedAfterPhysicalGather){
                std::vector<std::uint8_t> encoded;
                if(!encodeSimulationSnapshot(
                       original.captureSnapshot(),encoded,&error)){
                    std::cerr<<"AL2_SNAPSHOT_ENCODE_ERROR "
                             <<error<<std::endl;
                    return 1;
                }
                SimulationStateSnapshot decoded;
                REQUIRE(decodeSimulationSnapshot(
                    encoded,decoded,&error));
                REQUIRE(replay.restoreSnapshot(decoded,&error));
                savedAfterPhysicalGather=true;
            }
        }
        if(actionExecutedThisTick &&activity.success
           &&activity.kind==CivilizationActivityKind::Craft
           &&activity.technique==TechniqueId::DiggingStick)
            craftMinute=activity.minute;

        const auto& current=original.world().characters.front();
        if(current.civilization.inventory.count(
               ItemKind::DiggingStick,MaterialKind::Wood)>0)
            break;
    }
    if(failedToken==0 ||gatherToken==0 ||craftToken==0 ||craftMinute<0){
        const auto& logs=original.logs();
        const std::size_t start=logs.size()>30?logs.size()-30:0;
        for(std::size_t i=start;i<logs.size();++i)
            std::cerr<<logs[i]<<std::endl;
    }

    // Core issues monotonically increasing tokens. The later Craft must be
    // an entirely new physical command, not an ACK/relabel of the Gather.
    REQUIRE(failedToken!=0 &&gatherToken!=0 &&craftToken!=0);
    REQUIRE(failedToken<gatherToken &&gatherToken<craftToken);
    REQUIRE(failedSource!=0 &&successfulSource!=0
        &&failedSource!=successfulSource);
    REQUIRE(sourceStock(failedSource)==0 &&removedByCompetitor>0);
    const auto& finishedActor=original.world().characters.front();
    bool firsthandSourceExhaustion=false;
    for(const auto& memory:finishedActor.memory.entries){
        if(memory.source==MemorySource::DirectWitness
           &&memory.witnessed
           &&memory.who==actorId
           &&memory.sourceCharacter==actorId
           &&memory.where=="resource-node:"+std::to_string(failedSource)
           &&memory.tags.size()==3
           &&memory.tags[0]=="action_experience"
           &&memory.tags[2]=="failure"
           &&memory.what.find("source-exhausted")!=std::string::npos)
            firsthandSourceExhaustion=true;
    }
    REQUIRE(firsthandSourceExhaustion);
    bool witnessedPhysicalFailure=false;
    bool witnessedRecovery=false;
    for(const auto& line:original.logs()){
        if(line.find("AL1_CORE_GATHER_FAILURE actor="
            +std::to_string(actorId))!=std::string::npos
           &&line.find(" pending_token="+std::to_string(failedToken))
               !=std::string::npos)
            witnessedPhysicalFailure=true;
        if(line.find("AL1_CORE_GATHER_RESULT actor="
            +std::to_string(actorId))!=std::string::npos
           &&line.find(" pending_token="+std::to_string(gatherToken))
               !=std::string::npos)
            witnessedRecovery=true;
    }
    REQUIRE(witnessedPhysicalFailure &&witnessedRecovery);
    // The builder's own original accepted Core cognition must be credited
    // through the failed A token, the B recovery token, and the final tool
    // Craft. A fallback plan or an unrelated latest thought cannot claim a
    // token: require the exact original fingerprint AND actual Core receipts.
    const auto goalFingerprint=cognitiveRequestFingerprint(request);
    const auto hasCognitionStep=[&](std::uint64_t token,const char* intent){
        const std::string tag=
            "AL2_CORE_COGNITION_STEP_ISSUED actor="
            +std::to_string(actorId)
            +" goal_fp="+std::to_string(goalFingerprint)
            +" thought_minute="+std::to_string(request.minute)
            +" step_token="+std::to_string(token)+" ";
        for(const auto& line:original.logs())
            if(line.find(tag)!=std::string::npos
                &&line.find(" intent="+std::string(intent))
                    !=std::string::npos)
                return true;
        return false;
    };
    REQUIRE(hasCognitionStep(failedToken,"Gather"));
    REQUIRE(hasCognitionStep(gatherToken,"Gather"));
    REQUIRE(hasCognitionStep(craftToken,"Craft"));
    REQUIRE(firstGatherMinute>=480 &&craftMinute>firstGatherMinute);
    REQUIRE(gatherUnits>0 &&travelCells>0);
    REQUIRE(savedAfterPhysicalGather &&savedBeforePhysicalFailure);
    REQUIRE(original.acceptedCognitiveDecisions().size()==1);
    const auto& completed=original.world().characters.front();
    REQUIRE(completed.civilization.inventory.count(
        ItemKind::DiggingStick,MaterialKind::Wood)>0);
    REQUIRE(sourceStock(successfulSource)>=0
        &&sourceStock(successfulSource)<wood.quantity);
    REQUIRE(realFirsthandReceipt(
        completed,civilizationActionExperienceKey(gatherDecision),true,
        "resource-node:"+std::to_string(successfulSource)));
    REQUIRE(realFirsthandReceipt(
        completed,civilizationActionExperienceKey(craftDecision),true,
        "내가 직접 시도한 행동"));

    // The continuation runs through exactly the same Core tick path after
    // a byte-encoded save/load at the FIRST physical Gather result.
    std::vector<std::uint8_t> actualBytes,replayedBytes,preFailureBytes;
    REQUIRE(encodeSimulationSnapshot(
        original.captureSnapshot(),actualBytes,&error));
    REQUIRE(encodeSimulationSnapshot(
        replay.captureSnapshot(),replayedBytes,&error));
    REQUIRE(actualBytes==replayedBytes);
    REQUIRE(encodeSimulationSnapshot(
        preFailureReplay.captureSnapshot(),preFailureBytes,&error));
    REQUIRE(actualBytes==preFailureBytes);
    REQUIRE(replay.world().characters.front().civilization.inventory.count(
        ItemKind::DiggingStick,MaterialKind::Wood)>0);
    const CognitiveCausalTrace* causal=nullptr;
    for(const auto& trace:original.cognitiveCausalJournal())if(trace.accepted &&trace.origin.actor==actorId)causal=&trace;
    REQUIRE(causal!=nullptr);
    bool actualFailure=false,actualRecovery=false,actualCraft=false;
    for(const auto& step:causal->steps){
        if(step.token==failedToken)actualFailure=step.result==CognitiveStepResult::Failed &&step.attempted &&step.learning.changed;
        if(step.token==gatherToken)actualRecovery=step.result==CognitiveStepResult::Succeeded &&step.learning.changed
            &&step.source==CognitiveTraceSource::Continuation &&step.previousToken==failedToken;
        if(step.token==craftToken)actualCraft=step.result==CognitiveStepResult::Succeeded &&step.learning.changed;
    }
    REQUIRE(actualFailure &&actualRecovery &&actualCraft);
    REQUIRE(exportCausalFixture(original,actorId,"failure"));
    std::cout<<"AL2_PRODUCT_FAILURE_RECOVERY_CHAIN seed="<<seed
        <<" actor="<<actorId
        <<" failed_token="<<failedToken
        <<" failed_source="<<failedSource
        <<" competitor_removed="<<removedByCompetitor
        <<" recovered_token="<<gatherToken
        <<" alternative_source="<<successfulSource
        <<" craft_token="<<craftToken
        <<" gather_minute="<<firstGatherMinute
        <<" craft_minute="<<craftMinute
        <<" harvested="<<gatherUnits
        <<" actual_movement_steps="<<travelCells
        <<" source_stock="<<wood.quantity<<"->"
        <<sourceStock(successfulSource)
        <<" one_cognitive_goal_fp="<<goalFingerprint
        <<" linked_core_tokens=3"
        <<" real_exhaustion_failure_and_recovery=1"
        <<" direct_gather_and_craft_memories=1"
        <<" serialized_continuation_identical=1"
        <<" saved_inflight_cognition_link_survives_failure_recovery=1"
        <<" forged_fingerprint_rejected=1"
        <<" scope=controlled_competing_Core_resident_fixture"
        <<std::endl;
    return 0;
}
