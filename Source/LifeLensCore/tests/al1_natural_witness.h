#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

// Test-side observers only. The natural run calls setupNewGame and step;
// no goals, memories, positions, resources or execution results are injected.
namespace al1_test {
using namespace lifelens;

inline std::string field(const std::string& event,const std::string& key)
{
    const auto at=event.find(" "+key+"=");
    if(at==std::string::npos) return {};
    const auto begin=at+key.size()+2;
    const auto end=event.find(' ',begin);
    return event.substr(begin,end==std::string::npos ? end : end-begin);
}

inline std::uint64_t number(const std::string& event,const std::string& key)
{
    const auto value=field(event,key);
    assert(!value.empty());
    return std::stoull(value);
}

inline const Character& resident(const Simulation& sim,CharacterId id)
{
    const auto it=std::find_if(sim.world().characters.begin(),
        sim.world().characters.end(),[&](const Character& c){return c.id==id;});
    assert(it!=sim.world().characters.end());
    return *it;
}

inline const ResourceNode& resource(const Simulation& sim,ResourceNodeId id)
{
    const auto* node=findCivilizationResourceNodeSpatial(sim.world(),id);
    assert(node!=nullptr);
    return *node;
}

inline bool isReceipt(const MemoryRecord& memory,CharacterId actor,
                      ResourceNodeId source,MaterialKind material,int minute)
{
    return memory.who==actor &&memory.sourceCharacter==actor
        &&memory.source==MemorySource::DirectWitness &&memory.witnessed
        &&memory.minute==minute
        &&memory.where=="resource-node:"+std::to_string(source)
        &&memory.tags==std::vector<std::string>{"action_experience",
            "Civilization:Gather:"+std::string(materialName(material)),"success"};
}

struct GatherReceipt {
    PendingContextActionObservation issued;
    GridPos positionAtIssue{};
    GridPos positionAtResult{};
    Needs needsAtIssue{};
    double curiosity=0.0;
    int resultMinute=-1;
    int quantity=0;
    int inventoryBefore=0;
    int inventoryAfter=0;
    int sourceBefore=0;
    int sourceAfter=0;
    std::string goalIssue;
    std::string goalResult;
};

struct NaturalLoop {
    WorldSeed seed=0;
    GatherReceipt original;
    GatherReceipt learned;
    ResourceNodeId unlearnedSource=0;
    int rememberedMinute=-1;
};

// Collect actual issue and physical-completion facts by the exact token.
// Bounds are deliberately finite; not finding the chain fails acceptance.
inline NaturalLoop observeNaturalLoop(WorldSeed seed,bool requireGoals=false)
{
    Simulation sim(seed);
    sim.setupNewGame();
    assert(sim.world().characters.size()==4);
    std::map<std::uint64_t,GatherReceipt> issued;
    std::map<std::tuple<CharacterId,ResourceNodeId,int>,GatherReceipt> completed;
    struct Learned { ResourceNodeId native=0; int memoryMinute=-1; };
    std::map<std::uint64_t,Learned> learned;
    struct Before { std::uint64_t token=0; int inventory=0,stock=0; };
    std::map<std::uint64_t,Before> atIssue;
    std::size_t logsSeen=0;

    const auto captureIssued=[&](CharacterId actorId){
        const auto p=sim.observePendingContextAction(actorId);
        if(!p.active ||p.kind!=ContextActionKind::Civilization
           ||p.civilizationIntent!=CivilizationIntent::Gather
           ||issued.count(p.token)) return;
        const auto& actor=resident(sim,actorId);
        GatherReceipt receipt;
        receipt.issued=p;
        receipt.needsAtIssue=actor.needs;
        receipt.curiosity=actor.personality.curiosity;
        assert(sim.runtimePosition(actor.id,receipt.positionAtIssue));
        issued.emplace(p.token,receipt);
        atIssue.emplace(p.token,Before{p.token,
            actor.civilization.inventory.count(ItemKind::RawMaterial,p.material),
            resource(sim,p.resourceNode).quantity});
    };
    // Critical replanning can issue and finish an adjacent Gather in one
    // tick. Observe the real pending state at Core's synchronous issue event;
    // a tick-boundary sampler must not invent or silently miss that token.
    sim.onEvent([&](const std::string& event){
        if(event.find("AL1_CORE_GOAL_ISSUED actor=")!=std::string::npos
           ||event.find("AL1_CORE_NATIVE_GATHER_LEARNED_TOKEN actor=")
                !=std::string::npos)
            captureIssued(number(event,"actor"));
    });

    for(int tick=0;tick<14*1440;++tick){
        const int beforeMinute=sim.world().minute;
        std::map<CharacterId,Before> before;
        for(const auto& actor:sim.world().characters){
            const auto p=sim.observePendingContextAction(actor.id);
            if(p.active &&p.kind==ContextActionKind::Civilization
               &&p.civilizationIntent==CivilizationIntent::Gather){
                before.emplace(actor.id,Before{p.token,
                    actor.civilization.inventory.count(ItemKind::RawMaterial,p.material),
                    resource(sim,p.resourceNode).quantity});
            }
        }
        sim.step();
        assert(sim.world().minute==beforeMinute+1);
        for(const auto& actor:sim.world().characters) captureIssued(actor.id);

        // Result evaluation may precede the ordinary Gather log in the same
        // tick. First attach all goal results, then verify physical receipts.
        for(std::size_t i=logsSeen;i<sim.logs().size();++i){
            const auto& event=sim.logs()[i];
            if(event.find("AL1_CORE_GOAL_ISSUED actor=")!=std::string::npos){
                const auto it=issued.find(number(event,"pending_token"));
                if(it!=issued.end()) it->second.goalIssue=event;
            }
            if(event.find("AL1_CORE_GOAL_RESULT actor=")!=std::string::npos){
                const auto it=issued.find(number(event,"pending_token"));
                if(it!=issued.end()) it->second.goalResult=event;
            }
            if(event.find("AL1_CORE_NATIVE_GATHER_LEARNED_TOKEN actor=")
                   !=std::string::npos){
                const auto token=number(event,"pending_token");
                const auto it=issued.find(token);
                assert(it!=issued.end());
                assert(it->second.issued.actor==number(event,"actor"));
                assert(it->second.issued.resourceNode==number(event,"learned_source"));
                assert(it->second.issued.issuedMinute==beforeMinute);
                learned.emplace(token,Learned{number(event,"native_source"),
                    static_cast<int>(number(event,"direct_success_minute"))});
            }
        }
        while(logsSeen<sim.logs().size()){
            const auto& event=sim.logs()[logsSeen++];
            if(event.find("AL1_CORE_GATHER_RESULT actor=")==std::string::npos)
                continue;
            const auto token=number(event,"pending_token");
            const auto it=issued.find(token);
            // An older unlinked immediate action has no issue observation.
            // It cannot serve as evidence for this full token-linked chain.
            if(it==issued.end()) continue;
            GatherReceipt receipt=it->second;
            const auto& p=receipt.issued;
            assert(p.actor==number(event,"actor"));
            assert(p.resourceNode==number(event,"source"));
            const auto previous=before.find(p.actor);
            const bool alreadyPending=previous!=before.end()
                &&previous->second.token==token;
            assert(alreadyPending ||p.issuedMinute==beforeMinute);
            const Before measured=alreadyPending ? previous->second : atIssue.at(token);
            const auto actual=sim.observeResidentCivilizationActivity(p.actor);
            assert(actual.active &&actual.success
                &&actual.kind==CivilizationActivityKind::Gather
                &&actual.resourceNode==p.resourceNode &&actual.material==p.material
                &&actual.quantity==static_cast<int>(number(event,"actual_quantity"))
                &&actual.quantity>0 &&actual.minute==beforeMinute);
            receipt.resultMinute=actual.minute;
            receipt.quantity=actual.quantity;
            receipt.inventoryBefore=measured.inventory;
            receipt.inventoryAfter=resident(sim,p.actor).civilization.inventory.count(
                ItemKind::RawMaterial,p.material);
            receipt.sourceBefore=measured.stock;
            receipt.sourceAfter=resource(sim,p.resourceNode).quantity;
            assert(sim.runtimePosition(p.actor,receipt.positionAtResult));
            assert(contextActionNearTarget(receipt.positionAtResult,p.targetPos,1));
            assert(receipt.inventoryAfter-receipt.inventoryBefore==receipt.quantity);
            const auto& memories=resident(sim,p.actor).memory.entries;
            assert(std::count_if(memories.begin(),memories.end(),
                [&](const MemoryRecord& memory){return isReceipt(memory,p.actor,
                    p.resourceNode,p.material,actual.minute);})==1);
            completed.emplace(std::make_tuple(p.actor,p.resourceNode,actual.minute),receipt);

            const auto change=learned.find(token);
            if(change==learned.end()) continue;
            const auto first=completed.find(std::make_tuple(p.actor,
                p.resourceNode,change->second.memoryMinute));
            assert(first!=completed.end());
            assert(first->second.issued.token!=token);
            assert(first->second.resultMinute<p.issuedMinute);
            if(requireGoals &&(!first->second.issued.gatherGoal.active()
                ||!receipt.issued.gatherGoal.active())) continue;
            // In these acceptance witnesses the source also visibly loses
            // real stock across the tick. A simultaneous regeneration is not
            // misreported as an exactly measured external world delta.
            if(receipt.sourceBefore-receipt.sourceAfter<receipt.quantity
               ||first->second.sourceBefore-first->second.sourceAfter
                    <first->second.quantity) continue;
            return NaturalLoop{seed,first->second,receipt,
                change->second.native,change->second.memoryMinute};
        }
    }
    std::cerr<<"AL1_NATURAL_LOOP seed="<<seed<<" acceptance=NOT_PROVEN\n";
    assert(false &&"Both untouched New Games must complete an actual learned Gather");
    return {};
}

inline void printReceipt(const char* stage,const GatherReceipt& receipt)
{
    const auto& p=receipt.issued;
    std::cout<<" stage="<<stage<<" actor="<<p.actor
        <<" goal_id="<<p.gatherGoal.goalId<<" token="<<p.token
        <<" material="<<materialName(p.material)<<" source="<<p.resourceNode
        <<" issue_minute="<<p.issuedMinute<<" result_minute="<<receipt.resultMinute
        <<" quantity="<<receipt.quantity
        <<" inventory="<<receipt.inventoryBefore<<"->"<<receipt.inventoryAfter
        <<" source_stock="<<receipt.sourceBefore<<"->"<<receipt.sourceAfter
        <<" position="<<receipt.positionAtIssue.x<<","<<receipt.positionAtIssue.y
        <<"->"<<receipt.positionAtResult.x<<","<<receipt.positionAtResult.y
        <<" hunger="<<receipt.needsAtIssue.hunger
        <<" thirst="<<receipt.needsAtIssue.thirst
        <<" fatigue="<<receipt.needsAtIssue.sleep
        <<" curiosity="<<receipt.curiosity;
}

inline std::vector<std::uint8_t> bytes(const SimulationStateSnapshot& snapshot)
{
    std::vector<std::uint8_t> encoded;
    std::string error;
    const bool ok=encodeSimulationSnapshot(snapshot,encoded,&error);
    if(!ok) std::cerr<<error<<"\n";
    assert(ok &&error.empty());
    return encoded;
}
} // namespace al1_test
