#include <cassert>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <string>

#include "al1_natural_witness.h"
#include "lifelens/CoreNavigation.h"
#include "lifelens/ResidentLocalResourceMemory.h"
#include "lifelens/ResidentKnownResourceMemory.h"
#include "lifelens/ResidentResourceOutcomePreference.h"

using namespace lifelens;
using namespace al1_test;

// Read-only New Game audit. A diagnostic run with zero recoveries is NOT
// acceptance. Never inject a failure, source, position, need, or AI decision.
struct MeasuredState {
    int inventory=0;
    int stock=0;
    GridPos position{};
};

struct IssuedAction {
    PendingContextActionObservation action;
    MeasuredState atIssue;
    MeasuredState beforeTick;
    std::string goalResult;
    bool recovery=false;
    bool sameGoal=false;
    bool completed=false;
};

static MeasuredState measure(const Simulation& sim,
    const PendingContextActionObservation& action)
{
    MeasuredState state;
    state.inventory=resident(sim,action.actor).civilization.inventory.count(
        ItemKind::RawMaterial,action.material);
    const auto* node=findCivilizationResourceNodeSpatial(sim.world(),action.resourceNode);
    state.stock=node ? node->quantity : 0;
    assert(sim.runtimePosition(action.actor,state.position));
    return state;
}

// These diagnostics inspect ONLY IDs already sensed or personally remembered.
// audit_world_stock is independent ground truth for the test; it is NOT
// information granted to the resident or a claim of fresh remote knowledge.
static void describeKnownAlternatives(const Simulation& sim,WorldSeed seed,
    const PendingContextActionObservation& failed)
{
    const auto& actor=resident(sim,failed.actor);
    GridPos position{};
    assert(sim.runtimePosition(actor.id,position));
    struct Known { const char* provenance=""; int minute=-1; double confidence=0; };
    std::map<ResourceNodeId,Known> known;
    for(const auto& memory:residentKnownResources(actor,sim.world().minute))
        if(memory.material==failed.material)
            known[memory.nodeId]={"dated-local-observation",memory.minute,memory.confidence};
    const auto sensed=observeLocalResourceProximity(
        sim.world(),actor.id,position,ResidentLocalPerceptionMaxRadius);
    for(const auto& fact:sensed.nearbyResources)
        if(fact.material==failed.material)
            known[fact.nodeId]={"current-proximity",sim.world().minute,0.6};
    const auto firsthand=buildResidentResourceOutcomeIndex(actor,sim.world().minute);
    const std::string actionKey="Civilization:Gather:"
        +std::string(materialName(failed.material));
    for(const auto& [key,value]:firsthand.entries)
        if(key.first==actionKey &&value.succeeded &&!known.count(key.second))
            known[key.second]={"direct-success",value.minute,0.0};
    for(const auto& memory:actor.memory.entries){
        if(!isResidentSensedResourceMemory(memory,actor.id)
           ||memory.tags[2]!="material:"+std::to_string(static_cast<int>(failed.material)))
            continue;
        ResourceNodeId id=0;
        if(!resident_resource_outcome_detail::parseSourceNode(
            "resource-node:"+memory.tags[1].substr(5),id) ||known.count(id)) continue;
        known[id]={"remembered-proximity",memory.minute,
            memory.effectiveConfidence(sim.world().minute)};
    }
    int alternatives=0;
    for(const auto& [id,knowledge]:known){
        if(id==failed.resourceNode) continue;
        ++alternatives;
        GridPos access{};
        std::vector<GridPos> route;
        const bool reachable=resolveCivilizationResourceAccessGridPosition(
            sim.world(),id,access)
            &&buildCoreGroundRoute(sim.world(),position,access,1,route);
        const auto* node=findCivilizationResourceNodeSpatial(sim.world(),id);
        std::cout<<"AL1_NATURAL_KNOWN_ALTERNATIVE seed="<<seed
            <<" actor="<<actor.id<<" goal_id="<<failed.gatherGoal.goalId
            <<" failed_token="<<failed.token<<" source="<<id
            <<" provenance="<<knowledge.provenance
            <<" remembered_minute="<<knowledge.minute
            <<" effective_confidence="<<knowledge.confidence
            <<" reachable="<<reachable<<" route_steps="<<route.size()
            <<" remaining_minutes="<<(failed.gatherGoal.expiresMinute-sim.world().minute)
            <<" audit_world_stock="<<(node ? node->quantity : 0)<<"\n";
    }
    std::cout<<"AL1_NATURAL_ALTERNATIVE_CONTEXT seed="<<seed
        <<" actor="<<actor.id<<" goal_id="<<failed.gatherGoal.goalId
        <<" token="<<failed.token<<" known_alternatives="<<alternatives
        <<" hunger="<<actor.needs.hunger<<" thirst="<<actor.needs.thirst
        <<" fatigue="<<actor.needs.sleep<<" bladder="<<actor.needs.bladder
        <<" hygiene="<<actor.needs.hygiene<<"\n";
}

static void inspectNaturalGame(WorldSeed seed,int days)
{
    Simulation simulation(seed);
    simulation.setupNewGame();
    assert(simulation.world().characters.size()==4);
    std::map<std::uint64_t,IssuedAction> issued;
    std::map<std::uint64_t,ResourceNodeId> failures;
    int actualIssued=0,physicallyCompleted=0,sameGoalCompleted=0;
    int achieved=0,actualFailures=0;

    const auto captureIssued=[&](CharacterId actorId){
        const auto action=simulation.observePendingContextAction(actorId);
        if(!action.active ||action.kind!=ContextActionKind::Civilization
           ||action.civilizationIntent!=CivilizationIntent::Gather
           ||issued.count(action.token)) return;
        const auto current=measure(simulation,action);
        issued.emplace(action.token,IssuedAction{action,current,current});
    };

    // Observe synchronous Core events: an adjacent issued Gather may finish
    // within the same tick and disappear from the pending observer by step's
    // return. Capturing it here observes production state without changing it.
    simulation.onEvent([&](const std::string& line){
        if(line.find("AL1_CORE_GOAL_ISSUED actor=")!=std::string::npos){
            captureIssued(number(line,"actor"));
        }else if(line.find("AL1_CORE_GOAL_RESULT actor=")!=std::string::npos){
            const auto found=issued.find(number(line,"pending_token"));
            if(found!=issued.end()) found->second.goalResult=line;
        }else if(line.find("AL1_CORE_GATHER_FAILURE actor=")!=std::string::npos){
            const auto token=number(line,"pending_token");
            failures.emplace(token,number(line,"source"));
            ++actualFailures;
            const auto found=issued.find(token);
            std::cout<<"AL1_NATURAL_FAILURE seed="<<seed
                <<" result_minute="<<simulation.world().minute<<" "<<line;
            if(found!=issued.end()){
                const auto& action=found->second.action;
                const auto current=measure(simulation,action);
                assert(current.inventory==found->second.beforeTick.inventory);
                std::cout<<" goal_id="<<action.gatherGoal.goalId
                    <<" material="<<materialName(action.material)
                    <<" target_quantity="<<action.gatherGoal.targetQuantity
                    <<" inventory="<<current.inventory
                    <<" position="<<current.position.x<<","<<current.position.y;
            }
            std::cout<<"\n";
            if(found!=issued.end())
                describeKnownAlternatives(simulation,seed,found->second.action);
            const auto actorId=static_cast<CharacterId>(number(line,"actor"));
            for(const auto& memory:resident(simulation,actorId).memory.entries){
                if(memory.minute!=simulation.world().minute ||memory.who!=actorId
                   ||memory.sourceCharacter!=actorId ||memory.source!=MemorySource::DirectWitness
                   ||!memory.witnessed ||memory.where!="resource-node:"+std::to_string(number(line,"source")))
                    continue;
                std::cout<<"AL1_NATURAL_FAILURE_MEMORY seed="<<seed
                    <<" actor="<<actorId<<" token="<<token<<" minute="<<memory.minute
                    <<" memory_kind="<<(memory.tags.empty() ? "none" : memory.tags[0])
                    <<" source_character="<<memory.sourceCharacter
                    <<" where="<<memory.where<<" what="<<std::quoted(memory.what)<<"\n";
            }
        }else if(line.find("AL1_CORE_RESOURCE_REPLAN actor=")!=std::string::npos){
            const auto actorId=static_cast<CharacterId>(number(line,"actor"));
            captureIssued(actorId);
            const auto recoveryToken=number(line,"pending_token");
            const auto originalToken=number(line,"failed_pending_token");
            auto found=issued.find(recoveryToken);
            assert(found!=issued.end());
            auto& receipt=found->second;
            const auto& action=receipt.action;
            const auto oldSource=number(line,"failed_source");
            assert(action.actor==actorId &&action.resourceNode==number(line,"selected_source")
                &&oldSource!=action.resourceNode &&!receipt.recovery);
            const auto failureMinute=static_cast<int>(number(line,"failure_minute"));
            assert(number(line,"decision_minute")==static_cast<std::uint64_t>(simulation.world().minute)
                &&failureMinute<=simulation.world().minute);
            const auto& actor=resident(simulation,actorId);
            const auto memory=buildResidentResourceOutcomeIndex(actor,simulation.world().minute).find(
                oldSource,"Civilization:Gather:"+std::string(materialName(action.material)));
            assert(memory.hasEvidence &&!memory.succeeded &&memory.minute==failureMinute);
            bool directReceipt=false;
            for(const auto& remembered:actor.memory.entries)
                if(remembered.minute==failureMinute &&remembered.who==actorId
                   &&remembered.sourceCharacter==actorId
                   &&remembered.source==MemorySource::DirectWitness &&remembered.witnessed
                   &&remembered.where=="resource-node:"+std::to_string(oldSource))
                    directReceipt=true;
            assert(directReceipt);
            const auto previous=issued.find(originalToken);
            if(originalToken!=0 &&previous!=issued.end()){
                const auto& before=previous->second.action;
                assert(before.actor==actorId &&before.resourceNode==oldSource);
                if(before.gatherGoal.active()){
                    assert(action.gatherGoal.active()
                        &&action.gatherGoal.goalId==before.gatherGoal.goalId
                        &&action.gatherGoal.previousToken==originalToken
                        &&action.gatherGoal.targetQuantity==before.gatherGoal.targetQuantity
                        &&action.gatherGoal.expiresMinute==before.gatherGoal.expiresMinute
                        &&action.gatherGoal.attempt==before.gatherGoal.attempt+1);
                    receipt.sameGoal=true;
                }
            }
            receipt.recovery=true;
            ++actualIssued;
            std::cout<<"AL1_NATURAL_RECOVERY_ISSUED seed="<<seed<<" "<<line
                <<" goal_id="<<action.gatherGoal.goalId
                <<" material="<<materialName(action.material)
                <<" target_quantity="<<action.gatherGoal.targetQuantity
                <<" expires_minute="<<action.gatherGoal.expiresMinute
                <<" same_goal="<<receipt.sameGoal<<" direct_failure_memory=1\n";
        }else if(line.find("AL1_CORE_GATHER_RESULT actor=")!=std::string::npos){
            const auto found=issued.find(number(line,"pending_token"));
            if(found==issued.end() ||!found->second.recovery) return;
            auto& receipt=found->second;
            const auto& action=receipt.action;
            assert(action.actor==number(line,"actor") &&action.resourceNode==number(line,"source")
                &&!receipt.completed);
            const auto actual=simulation.observeResidentCivilizationActivity(action.actor);
            const int quantity=static_cast<int>(number(line,"actual_quantity"));
            assert(actual.active &&actual.success &&actual.kind==CivilizationActivityKind::Gather
                &&actual.resourceNode==action.resourceNode &&actual.material==action.material
                &&actual.quantity==quantity &&quantity>0
                &&actual.minute==simulation.world().minute);
            const auto current=measure(simulation,action);
            assert(current.inventory-receipt.beforeTick.inventory==quantity);
            assert(contextActionNearTarget(current.position,action.targetPos,1));
            // Independent ledger state must agree with the action receipt.
            assert(!receipt.goalResult.empty());
            const int resourceBefore=static_cast<int>(number(receipt.goalResult,"resource_before"));
            const int resourceAfter=static_cast<int>(number(receipt.goalResult,"resource_after"));
            assert(resourceBefore-resourceAfter==quantity &&resourceAfter==current.stock);
            assert(static_cast<int>(number(receipt.goalResult,"inventory_after"))==current.inventory);
            const auto& memories=resident(simulation,action.actor).memory.entries;
            assert(std::count_if(memories.begin(),memories.end(),[&](const MemoryRecord& memory){
                return isReceipt(memory,action.actor,action.resourceNode,action.material,
                    simulation.world().minute);})==1);
            const bool goalAchieved=current.inventory>=action.gatherGoal.targetQuantity;
            assert((field(receipt.goalResult,"outcome")=="achieved")==goalAchieved);
            receipt.completed=true;
            ++physicallyCompleted;
            if(receipt.sameGoal) ++sameGoalCompleted;
            if(receipt.sameGoal &&goalAchieved) ++achieved;
            std::cout<<"AL1_NATURAL_RECOVERY_COMPLETED seed="<<seed<<" "<<line
                <<" goal_id="<<action.gatherGoal.goalId
                <<" target_quantity="<<action.gatherGoal.targetQuantity
                <<" result_minute="<<simulation.world().minute
                <<" inventory="<<receipt.beforeTick.inventory<<"->"<<current.inventory
                <<" source_stock="<<resourceBefore<<"->"<<current.stock
                <<" independently_observed_tick_stock="<<receipt.beforeTick.stock<<"->"<<current.stock
                <<" position="<<receipt.atIssue.position.x<<","<<receipt.atIssue.position.y
                <<"->"<<current.position.x<<","<<current.position.y
                <<" same_goal="<<receipt.sameGoal<<" goal_achieved="<<goalAchieved
                <<" direct_success_memory=1\n";
        }else if(line.find("AL1_CORE_REPLAN_CANDIDATE")!=std::string::npos
                 ||line.find("AL1_CORE_RESOURCE_FORECAST_REPLAN")!=std::string::npos
                 ||line.find("AL1_CORE_GATHER_REASSESS")!=std::string::npos){
            std::cout<<"AL1_NATURAL_PRODUCT_EVIDENCE seed="<<seed<<" "<<line<<"\n";
        }
    });

    for(int minute=0;minute<days*1440;++minute){
        for(const auto& actor:simulation.world().characters){
            captureIssued(actor.id);
            const auto pending=simulation.observePendingContextAction(actor.id);
            const auto found=issued.find(pending.token);
            if(pending.active &&found!=issued.end())
                found->second.beforeTick=measure(simulation,pending);
        }
        simulation.step();
    }
    assert(physicallyCompleted<=actualIssued &&sameGoalCompleted<=physicallyCompleted);
    std::cout<<"AL1_UNMODIFIED_NATURAL_REPLAN seed="<<seed
        <<" observed_days="<<days<<" issued="<<actualIssued
        <<" physically_completed="<<physicallyCompleted
        <<" same_goal_completed="<<sameGoalCompleted<<" goals_achieved="<<achieved
        <<" actual_gather_failures="<<actualFailures
        <<" proven_complete="<<(sameGoalCompleted>0)
        <<" acceptance="<<(sameGoalCompleted>0 ? "CHAIN_OBSERVED" : "NOT_PROVEN")<<"\n";
}

int main(int argc,char** argv)
{
    std::cout.setf(std::ios::unitbuf);
    const int days=argc>1 ? std::stoi(argv[1]) : 14;
    assert(days>=1 &&days<=365);
    if(argc>2) inspectNaturalGame(std::stoull(argv[2]),days);
    else for(const WorldSeed seed:{874213954ULL,4242001ULL,874213955ULL})
        inspectNaturalGame(seed,days);
}
