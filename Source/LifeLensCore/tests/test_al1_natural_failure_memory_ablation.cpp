#include "al1_natural_witness.h"
#include "lifelens/ResidentGatherAssessment.h"
#include <optional>
using namespace lifelens;
using namespace al1_test;

int main()
{
    // M8 structural tools change legitimate untouched-world histories. The
    // former seed 874213955 no longer presents this rare partial-inventory
    // failure opportunity within 90 days. Seed 4 supplies a genuine natural
    // failure with a known alternative; all counterfactual/physical checks
    // remain, including the unchanged 90-day observation bound.
    constexpr WorldSeed seed=4;
    Simulation natural(seed);natural.setupNewGame();
    std::optional<SimulationStateSnapshot> boundary;
    CharacterId actor=0;
    PendingContextAction original;
    natural.onEvent([&](const std::string& event){
        if(boundary ||event.find("AL1_CORE_GATHER_FAILURE actor=")==std::string::npos) return;
        const auto id=number(event,"actor");
        auto snapshot=natural.captureSnapshot();
        const auto pending=snapshot.runtime.at(id).pendingContext;
        const auto& c=resident(natural,id);
        const int held=c.civilization.inventory.count(ItemKind::RawMaterial,pending.civilization.material);
        if(!pending.gatherGoal.active() ||held==0) return;
        const auto choices=assessResidentGatherAlternatives(natural.world(),c,
            snapshot.runtime.at(id).pos,pending.civilization,pending.gatherGoal.expiresMinute,
            pending.gatherGoal.targetQuantity-held);
        if(std::none_of(choices.begin(),choices.end(),[](const auto& candidate){return candidate.feasible;})) return;
        boundary=std::move(snapshot);actor=id;original=pending;
    });
    for(int n=0;n<90*1440 &&!boundary;++n) natural.step();
    assert(boundary &&"Natural failure with a feasible known alternative must be observed");
    auto ablated=*boundary;
    auto& c=*std::find_if(ablated.world.characters.begin(),ablated.world.characters.end(),
        [&](const auto& r){return r.id==actor;});
    auto it=std::find_if(c.memory.entries.begin(),c.memory.entries.end(),[&](const auto& m){
        return m.minute==ablated.world.minute &&m.who==actor &&m.sourceCharacter==actor
            &&m.where=="resource-node:"+std::to_string(original.civilization.resourceNode)
            &&m.tags==std::vector<std::string>{"action_experience",
                "Civilization:Gather:"+std::string(materialName(original.civilization.material)),"failure"};
    });
    assert(it!=c.memory.entries.end());
    const auto removed=*it;const auto index=it-c.memory.entries.begin();
    c.memory.entries.erase(it);
    auto restored=ablated;
    auto& restoredActor=*std::find_if(restored.world.characters.begin(),restored.world.characters.end(),
        [&](const auto& r){return r.id==actor;});
    restoredActor.memory.entries.insert(restoredActor.memory.entries.begin()+index,removed);
    std::vector<std::uint8_t> originalBytes,restoredBytes;std::string error;
    assert(encodeSimulationSnapshot(*boundary,originalBytes,&error));
    assert(encodeSimulationSnapshot(restored,restoredBytes,&error));
    assert(originalBytes==restoredBytes);

    Simulation learned(1),control(1);
    assert(learned.restoreSnapshot(*boundary,&error));
    assert(control.restoreSnapshot(ablated,&error));
    // Both branches resume the same synchronous failed-execution boundary.
    // The already-updated belief deduplicates that same-minute outcome: only
    // the retained individual receipt can authorize recovery. No world fact,
    // goal, position, RNG, belief or other memory differs between branches.
    learned.step();control.step();
    const auto alternative=learned.observePendingContextAction(actor);
    assert(alternative.active &&alternative.gatherGoal.previousToken==original.token);
    const auto noReceipt=control.observePendingContextAction(actor);
    assert(!noReceipt.active ||noReceipt.resourceNode!=alternative.resourceNode);
    const int before=resident(learned,actor).civilization.inventory.count(ItemKind::RawMaterial,alternative.material);
    const int stock=resource(learned,alternative.resourceNode).quantity;
    GridPos from{};assert(learned.runtimePosition(actor,from));
    bool completed=false;
    learned.onEvent([&](const std::string& event){
        if(event.find("AL1_CORE_GATHER_RESULT actor=")!=std::string::npos
           &&number(event,"pending_token")==alternative.token) completed=true;
    });
    for(int n=0;n<120 &&!completed;++n){learned.step();control.step();}
    assert(completed);
    const int after=resident(learned,actor).civilization.inventory.count(ItemKind::RawMaterial,alternative.material);
    GridPos to{};assert(learned.runtimePosition(actor,to));
    assert(after>before &&resource(learned,alternative.resourceNode).quantity<stock);
    assert(after>=original.gatherGoal.targetQuantity); // A partial harvest is not goal completion.
    assert(!sameGridPos(from,to));
    bool receipt=false;
    for(const auto& memory:resident(learned,actor).memory.entries)
        receipt|=isReceipt(memory,actor,alternative.resourceNode,alternative.material,memory.minute);
    assert(receipt);
    std::cout<<"AL1_NATURAL_FAILURE_MEMORY_ABLATION seed="<<seed<<" actor="<<actor
        <<" goal_id="<<original.gatherGoal.goalId<<" failed_token="<<original.token
        <<" retained_memory_token="<<alternative.token<<" source="<<alternative.resourceNode
        <<" inventory="<<before<<"->"<<after<<" source_stock="<<stock<<"->"
        <<resource(learned,alternative.resourceNode).quantity
        <<" removed_receipts=1 other_snapshot_bytes_identical=1 physical_completion=1"
        <<" goal_achieved="<<(after>=original.gatherGoal.targetQuantity)<<"\n";
}
