#include "al1_natural_witness.h"

using namespace lifelens;
using namespace al1_test;

static void verifyGoalReceipt(const GatherReceipt& receipt)
{
    const auto& action=receipt.issued;
    const auto& goal=action.gatherGoal;
    assert(validResidentGatherGoal(goal));
    assert(goal.actor==action.actor &&goal.token==action.token
        &&goal.material==action.material &&goal.issuedMinute==action.issuedMinute);
    assert(!receipt.goalIssue.empty() &&!receipt.goalResult.empty());
    assert(number(receipt.goalIssue,"goal_id")==goal.goalId
        &&number(receipt.goalIssue,"pending_token")==action.token
        &&number(receipt.goalIssue,"source")==action.resourceNode
        &&number(receipt.goalIssue,"inventory_at_issue")
            ==static_cast<std::uint64_t>(goal.inventoryAtIssue)
        &&number(receipt.goalIssue,"target_quantity")
            ==static_cast<std::uint64_t>(goal.targetQuantity));
    // The already existing unified selection records the competing physical
    // goal and both utilities; the test never asks a model to invent a goal.
    assert(!field(receipt.goalIssue,"competing_physical_goal").empty());
    assert(!field(receipt.goalIssue,"competing_physical_utility").empty());
    assert(!field(receipt.goalIssue,"selected_utility").empty());
    assert(!field(receipt.goalIssue,"hunger").empty()
        &&!field(receipt.goalIssue,"thirst").empty()
        &&!field(receipt.goalIssue,"curiosity").empty());

    assert(number(receipt.goalResult,"actor")==action.actor
        &&number(receipt.goalResult,"goal_id")==goal.goalId
        &&number(receipt.goalResult,"pending_token")==action.token
        &&number(receipt.goalResult,"source")==action.resourceNode);
    assert(number(receipt.goalResult,"inventory_before")
        ==static_cast<std::uint64_t>(receipt.inventoryBefore));
    assert(number(receipt.goalResult,"inventory_after")
        ==static_cast<std::uint64_t>(receipt.inventoryAfter));
    // The execution boundary measures exact extraction; the independently
    // sampled world tick additionally verifies an actual visible stock loss.
    const auto stockBefore=number(receipt.goalResult,"resource_before");
    const auto stockAfter=number(receipt.goalResult,"resource_after");
    assert(stockBefore>=stockAfter
        &&stockBefore-stockAfter==static_cast<std::uint64_t>(receipt.quantity));
    assert(receipt.sourceBefore-receipt.sourceAfter>=receipt.quantity);
    const std::string expected=receipt.inventoryAfter>=goal.targetQuantity
        ? "achieved" : "progress-unmet";
    assert(field(receipt.goalResult,"outcome")==expected);
}


/**
 * Reproduce the learned native decision from a REAL end-of-tick binary save.
 * A memory-dependent source selection that vanishes after loading cannot
 * close AL-1, even if an uninterrupted New Game happened to pass.
 *
 * Neither branch injects a goal, inventory, memory, resource, model answer,
 * or Core event. Both advance via production Simulation::step().
 */
static void verifyLearnedActionAfterSaveLoad(
    WorldSeed seed,const NaturalLoop& witness)
{
    Simulation uninterrupted(seed);
    uninterrupted.setupNewGame();
    const int decisionMinute=witness.learned.issued.issuedMinute;
    const int resultMinute=witness.learned.resultMinute;
    assert(decisionMinute>=0 &&resultMinute>=decisionMinute
        &&resultMinute-decisionMinute<180);
    while(uninterrupted.world().minute<decisionMinute)
        uninterrupted.step();
    assert(uninterrupted.world().minute==decisionMinute);

    const auto encoded=bytes(uninterrupted.captureSnapshot());
    SimulationStateSnapshot decoded;
    std::string error;
    assert(decodeSimulationSnapshot(encoded,decoded,&error) &&error.empty());
    assert(bytes(decoded)==encoded);

    Simulation restored(1);
    assert(restored.restoreSnapshot(decoded,&error) &&error.empty());
    assert(bytes(restored.captureSnapshot())==encoded);

    // The same learned Gather must be actually issued/executed after reload,
    // not merely retained in the snapshot's memory or a read-only utility.
    for(int minute=decisionMinute;minute<=resultMinute;++minute){
        uninterrupted.step();
        restored.step();
    }
    const auto originalBytes=bytes(uninterrupted.captureSnapshot());
    assert(bytes(restored.captureSnapshot())==originalBytes);

    const auto& action=witness.learned.issued;
    const std::string issuedMarker=
        "AL1_CORE_NATIVE_GATHER_LEARNED_TOKEN actor="
        +std::to_string(action.actor)
        +" pending_token="+std::to_string(action.token);
    const std::string executedMarker=
        "AL1_CORE_GATHER_RESULT actor="+std::to_string(action.actor)
        +" pending_token="+std::to_string(action.token)
        +" source="+std::to_string(action.resourceNode)
        +" actual_quantity="+std::to_string(witness.learned.quantity);
    const auto hasLog=[&](const Simulation& sim,const std::string& marker){
        return std::any_of(sim.logs().begin(),sim.logs().end(),
            [&](const std::string& event){
                return event.find(marker)!=std::string::npos;
            });
    };
    assert(hasLog(restored,issuedMarker));
    assert(hasLog(restored,executedMarker));
    const auto& actor=resident(restored,action.actor);
    assert(std::any_of(actor.memory.entries.begin(),actor.memory.entries.end(),
        [&](const MemoryRecord& memory){
            return isReceipt(memory,action.actor,action.resourceNode,
                action.material,resultMinute);
        }));
    assert(resource(restored,action.resourceNode).quantity
        ==witness.learned.sourceAfter);
    std::cout<<"AL1_NATIVE_SAVE_RESUME seed="<<seed
        <<" actor="<<action.actor
        <<" learned_token="<<action.token
        <<" source="<<action.resourceNode
        <<" decision_minute="<<decisionMinute
        <<" result_minute="<<resultMinute
        <<" quantity="<<witness.learned.quantity
        <<" binary_roundtrip=1"
        <<" identical_production_replay=1"
        <<" actual_learned_harvest=1"
        <<" firsthand_result_memory=1\\n";
}

int main()
{
    std::cout.setf(std::ios::unitbuf);
    for(const WorldSeed seed:{874213954ULL,4242001ULL}){
        const auto witness=observeNaturalLoop(seed,true);
        verifyGoalReceipt(witness.original);
        verifyGoalReceipt(witness.learned);
        verifyLearnedActionAfterSaveLoad(seed,witness);
        assert(witness.original.issued.actor==witness.learned.issued.actor);
        assert(witness.original.resultMinute==witness.rememberedMinute);
        assert(witness.original.issued.resourceNode==witness.learned.issued.resourceNode);
        assert(witness.unlearnedSource!=witness.learned.issued.resourceNode);
        std::cout<<"AL1_NATURAL_GOAL_CHAIN seed="<<seed;
        printReceipt("first-experience",witness.original);
        printReceipt("learned-reexecution",witness.learned);
        std::cout<<" memory_minute="<<witness.rememberedMinute
            <<" without_memory_native_source="<<witness.unlearnedSource
            <<" injected_goals=0 injected_resources=0 injected_memories=0"
            <<" model_inference=unconnected"
            <<" acceptance=GOAL_TOKEN_WORLD_MEMORY_REEXECUTION_OBSERVED\n";
        // Failure/recovery and the actual memory-ablation contrast are
        // separate mandatory gates, never silently inferred from this one.
    }
}
