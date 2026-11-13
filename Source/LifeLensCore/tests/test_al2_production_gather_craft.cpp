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
    // A CONTROLLED Core runtime fixture: the four actual New Game residents,
    // real Simulation::step, movement, action tokens, execution and firsthand
    // memories. Only the initial environment/known technique and one VALID
    // local cognitive proposal are selected for reproducibility. This is
    // deliberately not claimed as spontaneous, unmodified New Game AL-2.
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
    const auto sourceStock=[&](){
        const auto found=std::find_if(
            world.resourceNodes.begin(),world.resourceNodes.end(),
            [&](const ResourceNode& node){return node.id==wood.id;});
        return found==world.resourceNodes.end() ? -1 : found->quantity;
    };
    REQUIRE(sourceStock()==wood.quantity);
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
    REQUIRE(exportCausalFixture(original,actorId,"start"));

    std::uint64_t gatherToken=0;
    std::uint64_t craftToken=0;
    CivilizationUtilityDecision gatherDecision;
    CivilizationUtilityDecision craftDecision;
    int firstGatherMinute=-1;
    int craftMinute=-1;
    int gatherUnits=0;
    int travelCells=0;
    bool savedAfterPhysicalGather=false;
    Simulation replay(seed,0,CurrentWorldGenerationVersion,rules);
    std::string error;

    for(int elapsed=0;elapsed<500;++elapsed){
        GridPos beforePos{};
        REQUIRE(original.runtimePosition(actorId,beforePos));
        const int stockBefore=sourceStock();
        original.step();
        if(savedAfterPhysicalGather)
            replay.step();
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
               &&issued.civilization.resourceNode==wood.id
               &&gatherToken==0){
                gatherToken=issued.token;
                gatherDecision=issued.civilization;
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
           &&activity.resourceNode==wood.id){
            REQUIRE(gatherToken!=0);
            REQUIRE(activity.quantity>0);
            REQUIRE(stockBefore-sourceStock()>=activity.quantity);
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
    if(gatherToken==0 ||craftToken==0 ||craftMinute<0){
        const auto& logs=original.logs();
        const std::size_t start=logs.size()>30?logs.size()-30:0;
        for(std::size_t i=start;i<logs.size();++i)
            std::cerr<<logs[i]<<std::endl;
    }

    // Core issues monotonically increasing tokens. The later Craft must be
    // an entirely new physical command, not an ACK/relabel of the Gather.
    REQUIRE(gatherToken!=0 &&craftToken!=0 &&gatherToken<craftToken);
    REQUIRE(firstGatherMinute>=480 &&craftMinute>firstGatherMinute);
    REQUIRE(gatherUnits>0 &&travelCells>0);
    REQUIRE(savedAfterPhysicalGather);
    REQUIRE(original.acceptedCognitiveDecisions().size()==1);
    const auto& completed=original.world().characters.front();
    REQUIRE(completed.civilization.inventory.count(
        ItemKind::DiggingStick,MaterialKind::Wood)>0);
    REQUIRE(sourceStock()>=0 &&sourceStock()<wood.quantity);
    REQUIRE(realFirsthandReceipt(
        completed,civilizationActionExperienceKey(gatherDecision),true,
        "resource-node:"+std::to_string(wood.id)));
    REQUIRE(realFirsthandReceipt(
        completed,civilizationActionExperienceKey(craftDecision),true,
        "내가 직접 시도한 행동"));

    // AL-2: this is evidence that one accepted resident thought AUTHORISED
    // the two distinct physical Core tokens, not merely that ordinary Core
    // fell back to Gather/Craft while the thought sat in a journal. Both
    // executions must match the original fingerprint and the real result
    // log's issued step token. No second proposal may be credited.
    const std::uint64_t goalFingerprint=cognitiveRequestFingerprint(request);
    const std::string goalKey="AL2_CORE_COGNITION_STEP_ISSUED actor="
        +std::to_string(actorId)+" goal_fp="+std::to_string(goalFingerprint);
    const auto hasCognitiveStep=[&](std::uint64_t token,const char* intent){
        for(const auto& line:original.logs()){
            if(line.find(goalKey)!=std::string::npos
               &&line.find(" thought_minute=480")!=std::string::npos
               &&line.find(" step_token="+std::to_string(token))
                   !=std::string::npos
               &&line.find(" intent="+std::string(intent))
                   !=std::string::npos)
                return true;
        }
        return false;
    };
    REQUIRE(hasCognitiveStep(gatherToken,"Gather"));
    REQUIRE(hasCognitiveStep(craftToken,"Craft"));
    const auto hasExecutedStep=[&](std::uint64_t token,const char* intent){
        for(const auto& line:original.logs())
            if(line.find("-> Civilization "+std::string(intent))
                    !=std::string::npos
               &&line.find(" core_step_token="+std::to_string(token))
                    !=std::string::npos)
                return true;
        return false;
    };
    REQUIRE(hasExecutedStep(gatherToken,"Gather"));
    REQUIRE(hasExecutedStep(craftToken,"Craft"));

    // The continuation runs through exactly the same Core tick path after
    // a byte-encoded save/load at the FIRST physical Gather result.
    std::vector<std::uint8_t> actualBytes,replayedBytes;
    REQUIRE(encodeSimulationSnapshot(
        original.captureSnapshot(),actualBytes,&error));
    REQUIRE(encodeSimulationSnapshot(
        replay.captureSnapshot(),replayedBytes,&error));
    REQUIRE(actualBytes==replayedBytes);
    REQUIRE(replay.world().characters.front().civilization.inventory.count(
        ItemKind::DiggingStick,MaterialKind::Wood)>0);
    const auto& causal=original.cognitiveCausalJournal();
    REQUIRE(!causal.empty());
    const CognitiveCausalTrace* linked=nullptr;
    for(const auto& trace:causal)if(trace.accepted &&trace.origin.actor==actorId &&trace.origin.contextFingerprint==goalFingerprint)linked=&trace;
    REQUIRE(linked!=nullptr);
    for(const auto token:{gatherToken,craftToken}){
        const CognitiveCausalStep* actual=nullptr;
        for(const auto& step:linked->steps)if(step.token==token)actual=&step;
        REQUIRE(actual!=nullptr &&actual->result==CognitiveStepResult::Succeeded &&actual->attempted);
        REQUIRE(actual->learning.changed &&actual->learning.afterCount>actual->learning.beforeCount);
    }
    REQUIRE(validCognitiveCausalJournal(causal,original.world(),original.acceptedCognitiveDecisions()));
    REQUIRE(exportCausalFixture(original,actorId,"completed"));
    std::cout<<"AL2_PRODUCT_TICK_CHAIN seed="<<seed
        <<" actor="<<actorId
        <<" gather_token="<<gatherToken
        <<" craft_token="<<craftToken
        <<" gather_minute="<<firstGatherMinute
        <<" craft_minute="<<craftMinute
        <<" harvested="<<gatherUnits
        <<" actual_movement_steps="<<travelCells
        <<" source_stock="<<wood.quantity<<"->"
        <<sourceStock()
        <<" direct_gather_and_craft_memories=1"
        <<" same_cognitive_goal_fp="<<goalFingerprint
        <<" authentic_core_step_links=2"
        <<" serialized_continuation_identical=1"
        <<" scope=controlled_Core_new_game_fixture"
        <<std::endl;
    return 0;
}
