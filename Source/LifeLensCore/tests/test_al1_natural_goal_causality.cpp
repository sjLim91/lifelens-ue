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

int main()
{
    std::cout.setf(std::ios::unitbuf);
    for(const WorldSeed seed:{874213954ULL,4242001ULL}){
        const auto witness=observeNaturalLoop(seed,true);
        verifyGoalReceipt(witness.original);
        verifyGoalReceipt(witness.learned);
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
