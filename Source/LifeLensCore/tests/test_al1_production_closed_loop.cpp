#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "lifelens/Simulation.h"
#include "lifelens/ResidentCivilizationOutcomeLearning.h"

using namespace lifelens;

// This test never injects a CognitiveProposal, accepted decision, inventory,
// goal, character trait, pending action, location or environmental fixture.
// It witnesses the ordinary production New Game -> Simulation::step() path.
struct Witness {
    std::map<ResourceNodeId,int> residentIssuedGather;
    std::map<ResourceNodeId,std::set<std::uint64_t>> sourcePendingTokens;
    std::set<std::pair<ResourceNodeId,int>> observedOutcomes;
    ResourceNodeId completedSource=0;
    int completedMinute=-1;
    bool laterChoice=false;
};

int main()
{
    bool witnessedAnyFullChain=false;
    for(const WorldSeed seed:{874213954ULL,4242001ULL}){
        Simulation sim(seed);
        sim.setupNewGame();
        assert(sim.world().characters.size()==4);
        std::map<CharacterId,Witness> perResident;
        CharacterId actorWithChain=0;
        ResourceNodeId sourceWithChain=0;
        int recordedMinute=-1;

        // 14 days is a bounded integration witness, NOT a 365/1000-day audit.
        for(int tick=0;tick<14*1440;++tick){
            sim.step();
            for(const auto& actor:sim.world().characters){
                auto& witness=perResident[actor.id];
                const auto pending=sim.observePendingContextAction(actor.id);
                if(pending.active &&pending.kind==ContextActionKind::Civilization){
                    if(pending.civilizationIntent==CivilizationIntent::Gather
                       &&pending.resourceNode!=0)
                        witness.residentIssuedGather.emplace(
                            pending.resourceNode,sim.world().minute);
                    if(pending.civilizationIntent==CivilizationIntent::Gather
                       &&pending.resourceNode!=0 &&pending.token!=0)
                        witness.sourcePendingTokens[pending.resourceNode]
                            .insert(pending.token);
                    if(witness.completedMinute>=0
                       &&sim.world().minute>witness.completedMinute
                       &&(pending.civilizationIntent!=CivilizationIntent::Gather
                          ||pending.resourceNode!=witness.completedSource)){
                        witness.laterChoice=true;
                        actorWithChain=actor.id;
                        sourceWithChain=witness.completedSource;
                        recordedMinute=witness.completedMinute;
                    }
                }

                const auto actual=sim.observeResidentCivilizationActivity(actor.id);
                if(!actual.active ||actual.kind!=CivilizationActivityKind::Gather
                   ||actual.quantity<=0 ||actual.resourceNode==0
                   ||!actual.success ||actual.minute<0)
                    continue;
                if(witness.residentIssuedGather.find(actual.resourceNode)
                    ==witness.residentIssuedGather.end())
                    continue;
                if(!witness.observedOutcomes.insert(
                       {actual.resourceNode,actual.minute}).second)
                    continue;
                const std::string key=std::string("Civilization:Gather:")
                    +materialName(actual.material);
                const std::string source="resource-node:"
                    +std::to_string(actual.resourceNode);
                bool personallyRecorded=false;
                for(const auto& memory:actor.memory.entries)
                    if(memory.who==actor.id
                       &&memory.sourceCharacter==actor.id
                       &&memory.source==MemorySource::DirectWitness
                       &&memory.witnessed &&memory.minute==actual.minute
                       &&memory.where==source
                       &&memory.tags.size()==3
                       &&memory.tags[0]=="action_experience"
                       &&memory.tags[1]==key
                       &&memory.tags[2]=="success")
                        personallyRecorded=true;
                // A firsthand memory alone does not prove which pending
                // Core command actually extracted the resource. The token
                // must match the real issued Gather command and the live
                // Core harvest result log, including actor, source and amount.
                bool sameIssuedTokenExecuted=false;
                const auto issued=witness.sourcePendingTokens.find(
                    actual.resourceNode);
                if(issued!=witness.sourcePendingTokens.end()){
                    for(const auto token:issued->second){
                        const std::string resultMarker=
                            "AL1_CORE_GATHER_RESULT actor="
                            +std::to_string(actor.id)
                            +" pending_token="+std::to_string(token)
                            +" source="+std::to_string(actual.resourceNode)
                            +" actual_quantity="+std::to_string(actual.quantity);
                        const bool matched=std::any_of(
                            sim.logs().begin(),sim.logs().end(),
                            [&](const std::string& log){
                                return log.find(resultMarker)!=std::string::npos;
                            });
                        if(matched){sameIssuedTokenExecuted=true;break;}
                    }
                }
                if(personallyRecorded &&sameIssuedTokenExecuted){
                    witness.completedSource=actual.resourceNode;
                    witness.completedMinute=actual.minute;
                }
            }
            if(actorWithChain!=0) break;
        }
        std::cout<<"AL1_PRODUCTION_WITNESS seed="<<seed
                 <<" resident="<<actorWithChain
                 <<" source="<<sourceWithChain
                 <<" outcome_minute="<<recordedMinute
                 <<" followup="<<(actorWithChain!=0)
                 <<" matched_core_gather_token="<<(actorWithChain!=0)<<"\n";
        witnessedAnyFullChain=witnessedAnyFullChain||actorWithChain!=0;
    }
    // This establishes an actual issued Core token -> physically executed
    // Gather -> firsthand memory -> another production decision, NOT the
    // causal effect of that memory on preference
    // and NOT completed AL-1 acceptance.
    assert(witnessedAnyFullChain);
    return 0;
}
