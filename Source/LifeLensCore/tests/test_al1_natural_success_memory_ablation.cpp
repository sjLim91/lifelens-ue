#include "al1_natural_witness.h"
#include "lifelens/ResidentResourceOutcomePreference.h"

using namespace lifelens;
using namespace al1_test;

// Find a completed memory-dependent action in an untouched New Game, then
// replay that world to immediately BEFORE its decision. An arbitrary first
// harvest may never be reused, and a changed utility score is not acceptance.
static void inspectSeed(WorldSeed seed)
{
    const NaturalLoop natural=observeNaturalLoop(seed,true);
    const auto actorId=natural.learned.issued.actor;
    const auto source=natural.learned.issued.resourceNode;
    const auto material=natural.learned.issued.material;
    const int decisionMinute=natural.learned.issued.issuedMinute;

    Simulation untouched(seed);
    untouched.setupNewGame();
    while(untouched.world().minute<decisionMinute) untouched.step();
    assert(untouched.world().minute==decisionMinute);
    const auto actual=untouched.captureSnapshot();
    auto noReceipt=actual;
    auto actor=std::find_if(noReceipt.world.characters.begin(),
        noReceipt.world.characters.end(),[&](const Character& c){return c.id==actorId;});
    assert(actor!=noReceipt.world.characters.end());
    auto& memories=actor->memory.entries;
    const auto selected=std::find_if(memories.begin(),memories.end(),
        [&](const MemoryRecord& m){return isReceipt(m,actorId,source,
            material,natural.rememberedMinute);});
    assert(selected!=memories.end());
    assert(std::count_if(memories.begin(),memories.end(),
        [&](const MemoryRecord& m){return isReceipt(m,actorId,source,
            material,natural.rememberedMinute);})==1);
    const std::size_t removedIndex=static_cast<std::size_t>(selected-memories.begin());
    const MemoryRecord removed=*selected;
    const auto previousSize=memories.size();
    memories.erase(selected);
    assert(memories.size()+1==previousSize);

    // Reinserting precisely this record at the same index must restore every
    // serialized byte: World, RNG, runtime/goal tokens, beliefs, inventory,
    // personality, position and every other resident are otherwise identical.
    auto reinserted=noReceipt;
    auto restoredActor=std::find_if(reinserted.world.characters.begin(),
        reinserted.world.characters.end(),[&](const Character& c){return c.id==actorId;});
    restoredActor->memory.entries.insert(
        restoredActor->memory.entries.begin()+removedIndex,removed);
    assert(bytes(reinserted)==bytes(actual));
    assert(bytes(noReceipt)!=bytes(actual));
    assert(noReceipt.world.rng==actual.world.rng);

    const std::string key="Civilization:Gather:"+std::string(materialName(material));
    const auto withMemory=buildResidentResourceOutcomeIndex(
        resident(untouched,actorId),decisionMinute).find(source,key);
    const auto withoutMemory=buildResidentResourceOutcomeIndex(
        *actor,decisionMinute).find(source,key);
    assert(withMemory.hasEvidence &&withMemory.succeeded
        &&withMemory.minute==natural.rememberedMinute);

    Simulation replay(seed),ablated(seed);
    std::string error;
    assert(replay.restoreSnapshot(actual,&error) &&error.empty());
    assert(ablated.restoreSnapshot(noReceipt,&error) &&error.empty());
    PendingContextActionObservation retainedChoice,ablatedChoice;
    struct IssueState { int inventory=0,stock=0; };
    IssueState retainedAtIssue,ablatedAtIssue;
    const auto observeIssue=[&](Simulation& sim,
                               PendingContextActionObservation& choice,
                               IssueState& state){
        sim.onEvent([&,simulation=&sim,observed=&choice,atIssue=&state](const std::string& event){
            if(event.find("AL1_CORE_GOAL_ISSUED actor=")==std::string::npos
               &&event.find("AL1_CORE_NATIVE_GATHER_LEARNED_TOKEN actor=")
                    ==std::string::npos) return;
            if(number(event,"actor")!=actorId) return;
            const auto p=simulation->observePendingContextAction(actorId);
            if(!p.active ||p.issuedMinute!=decisionMinute) return;
            *observed=p;
            atIssue->inventory=resident(*simulation,actorId).civilization.inventory.count(
                ItemKind::RawMaterial,p.material);
            atIssue->stock=resource(*simulation,p.resourceNode).quantity;
        });
    };
    observeIssue(replay,retainedChoice,retainedAtIssue);
    observeIssue(ablated,ablatedChoice,ablatedAtIssue);
    const auto retainedLogStart=replay.logs().size();
    const auto ablatedLogStart=ablated.logs().size();
    replay.step();
    ablated.step();
    // The event observers also retain an adjacent command that completed
    // within this very tick; a missing end-of-tick pending is not a no-op.
    assert(retainedChoice.active &&ablatedChoice.active);
    assert(retainedChoice.kind==ContextActionKind::Civilization
        &&ablatedChoice.kind==ContextActionKind::Civilization);
    assert(retainedChoice.civilizationIntent==CivilizationIntent::Gather
        &&ablatedChoice.civilizationIntent==CivilizationIntent::Gather);
    assert(retainedChoice.token==natural.learned.issued.token
        &&retainedChoice.resourceNode==source);
    const bool differentIssuedChoice=
        retainedChoice.resourceNode!=ablatedChoice.resourceNode;
    std::cout<<"AL1_NATURAL_SUCCESS_MEMORY_ABLATION seed="<<seed
        <<" actor="<<actorId<<" real_success_minute="<<natural.rememberedMinute
        <<" decision_minute="<<decisionMinute<<" removed_records=1"
        <<" otherwise_identical_snapshot=1"
        <<" retained_source="<<retainedChoice.resourceNode
        <<" ablated_source="<<ablatedChoice.resourceNode
        <<" retained_token="<<retainedChoice.token
        <<" ablated_token="<<ablatedChoice.token
        <<" learned_preference="<<withMemory.utilityAdjustment
        <<" without_memory_preference="<<withoutMemory.utilityAdjustment
        <<" different_issued_choice="<<differentIssuedChoice<<"\n";
    assert(differentIssuedChoice);

    struct Completion { bool done=false; int minute=-1; int quantity=0; };
    Completion retainedResult,ablatedResult;
    const auto recordResult=[&](Simulation& sim,
                           const PendingContextActionObservation& issued,
                           Completion& result,int inventoryBefore,int stockBefore,
                           int minute,std::size_t logStart){
        for(std::size_t i=logStart;i<sim.logs().size();++i){
            const auto& event=sim.logs()[i];
            if(event.find("AL1_CORE_GATHER_RESULT actor=")==std::string::npos
               ||number(event,"pending_token")!=issued.token) continue;
            assert(number(event,"actor")==actorId
                &&number(event,"source")==issued.resourceNode);
            const int quantity=static_cast<int>(number(event,"actual_quantity"));
            assert(quantity>0);
            const auto& current=resident(sim,actorId);
            assert(current.civilization.inventory.count(ItemKind::RawMaterial,
                issued.material)-inventoryBefore==quantity);
            assert(stockBefore-resource(sim,issued.resourceNode).quantity>=quantity);
            GridPos position{};
            assert(sim.runtimePosition(actorId,position)
                &&contextActionNearTarget(position,issued.targetPos,1));
            assert(std::any_of(current.memory.entries.begin(),current.memory.entries.end(),
                [&](const MemoryRecord& m){return isReceipt(m,actorId,
                    issued.resourceNode,issued.material,minute);}));
            result={true,minute,quantity};
        }
    };
    recordResult(replay,retainedChoice,retainedResult,retainedAtIssue.inventory,
        retainedAtIssue.stock,decisionMinute,retainedLogStart);
    recordResult(ablated,ablatedChoice,ablatedResult,ablatedAtIssue.inventory,
        ablatedAtIssue.stock,decisionMinute,ablatedLogStart);
    const auto advance=[&](Simulation& sim,
                           const PendingContextActionObservation& issued,
                           Completion& result){
        if(result.done) return;
        const int inventoryBefore=resident(sim,actorId).civilization.inventory.count(
            ItemKind::RawMaterial,issued.material);
        const int stockBefore=resource(sim,issued.resourceNode).quantity;
        const int minute=sim.world().minute;
        const auto logStart=sim.logs().size();
        sim.step();
        recordResult(sim,issued,result,inventoryBefore,stockBefore,minute,logStart);
    };
    for(int minute=0;minute<120 &&(!retainedResult.done ||!ablatedResult.done);++minute){
        advance(replay,retainedChoice,retainedResult);
        advance(ablated,ablatedChoice,ablatedResult);
    }
    std::cout<<"AL1_MEMORY_ABLATION_PHYSICAL_RESULTS seed="<<seed
        <<" actor="<<actorId
        <<" retained_completed="<<retainedResult.done
        <<" retained_result_minute="<<retainedResult.minute
        <<" retained_quantity="<<retainedResult.quantity
        <<" ablated_completed="<<ablatedResult.done
        <<" ablated_result_minute="<<ablatedResult.minute
        <<" ablated_quantity="<<ablatedResult.quantity
        <<" acceptance="<<(retainedResult.done &&ablatedResult.done
            ? "ACTUAL_MEMORY_CAUSALITY_AND_RESULTS" : "NOT_PROVEN")<<"\n";
    assert(retainedResult.done &&ablatedResult.done);
    assert(retainedResult.minute==natural.learned.resultMinute
        &&retainedResult.quantity==natural.learned.quantity);
}

int main()
{
    std::cout.setf(std::ios::unitbuf);
    inspectSeed(874213954ULL);
    inspectSeed(4242001ULL);
}
