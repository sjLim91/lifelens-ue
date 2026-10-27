#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>

#include "lifelens/Simulation.h"
#include "lifelens/ResidentResourceOutcomePreference.h"

using namespace lifelens;

// An observational experiment, not a scripted memory for the actor:
//  * the source event comes from an UNMODIFIED New Game Core simulation;
//  * the two shadow replays begin with identical authoritative World state,
//    RNG, runtime, social state and all other residents' memories;
//  * the ONLY controlled ablation is one real firsthand-success receipt.
// Any later difference in actual Core pending actions can then be attributed
// to that particular remembered experience. Zero differences is NOT proof
// of effective choice adaptation, and must be printed as NOT_PROVEN.
static void inspectSeed(WorldSeed seed)
{
    Simulation natural(seed);
    natural.setupNewGame();
    assert(natural.world().characters.size()==4);

    CharacterId actorId=0;
    ResourceNodeId source=0;
    int eventMinute=-1;
    std::string gatherKey;
    SimulationStateSnapshot actual;

    for(int tick=0;tick<3*1440 &&actorId==0;++tick){
        natural.step();
        for(const auto& actor:natural.world().characters){
            const auto event=natural.observeResidentCivilizationActivity(actor.id);
            if(!event.active ||event.kind!=CivilizationActivityKind::Gather
               ||event.quantity<=0 ||!event.success
               ||event.resourceNode==0
               ||event.minute!=natural.world().minute-1) continue;
            const std::string key="Civilization:Gather:"
                +std::string(materialName(event.material));
            const std::string where="resource-node:"
                +std::to_string(event.resourceNode);
            bool direct=false;
            for(const auto& memory:actor.memory.entries)
                if(memory.who==actor.id
                   &&memory.sourceCharacter==actor.id
                   &&memory.source==MemorySource::DirectWitness
                   &&memory.witnessed
                   &&memory.minute==event.minute
                   &&memory.where==where
                   &&memory.tags.size()==3
                   &&memory.tags[0]=="action_experience"
                   &&memory.tags[1]==key
                   &&memory.tags[2]=="success"){
                    direct=true;
                    break;
                }
            if(!direct) continue;
            actorId=actor.id;
            source=event.resourceNode;
            eventMinute=event.minute;
            gatherKey=key;
            actual=natural.captureSnapshot();
            break;
        }
    }
    assert(actorId!=0 &&source!=0 &&eventMinute>=0);

    const auto sourceActor=std::find_if(
        actual.world.characters.begin(),actual.world.characters.end(),
        [&](const Character& c){return c.id==actorId;});
    assert(sourceActor!=actual.world.characters.end());
    const auto originalIndex=buildResidentResourceOutcomeIndex(
        *sourceActor,actual.world.minute);
    const auto remembered=originalIndex.find(source,gatherKey);
    assert(remembered.hasEvidence &&remembered.succeeded
        &&remembered.minute==eventMinute);

    // Counterfactual starts at exactly the point immediately after a REAL
    // completed physical Gather; only the actor's single firsthand success
    // memory for this actual source and minute is withheld.
    SimulationStateSnapshot noReceipt=actual;
    auto noActor=std::find_if(
        noReceipt.world.characters.begin(),noReceipt.world.characters.end(),
        [&](const Character& c){return c.id==actorId;});
    assert(noActor!=noReceipt.world.characters.end());
    const std::string where="resource-node:"+std::to_string(source);
    auto& entries=noActor->memory.entries;
    const auto oldSize=entries.size();
    entries.erase(std::remove_if(entries.begin(),entries.end(),
        [&](const MemoryRecord& memory){
            return memory.minute==eventMinute
                &&memory.who==actorId
                &&memory.sourceCharacter==actorId
                &&memory.source==MemorySource::DirectWitness
                &&memory.witnessed
                &&memory.where==where
                &&memory.tags.size()==3
                &&memory.tags[0]=="action_experience"
                &&memory.tags[1]==gatherKey
                &&memory.tags[2]=="success";
        }),entries.end());
    assert(entries.size()+1==oldSize);
    for(const auto& unaltered:actual.world.characters)
        if(unaltered.id!=actorId){
            const auto parallel=std::find_if(
                noReceipt.world.characters.begin(),noReceipt.world.characters.end(),
                [&](const Character& c){return c.id==unaltered.id;});
            assert(parallel!=noReceipt.world.characters.end());
            assert(parallel->memory.entries.size()==unaltered.memory.entries.size());
        }
    assert(noReceipt.world.rng==actual.world.rng);
    const auto ablatedIndex=buildResidentResourceOutcomeIndex(
        *noActor,noReceipt.world.minute);
    const auto without=ablatedIndex.find(source,gatherKey);
    assert(!without.hasEvidence ||without.minute<eventMinute);
    assert(!without.hasEvidence
        ||without.utilityAdjustment<remembered.utilityAdjustment);

    Simulation replay(seed);
    Simulation ablated(seed);
    std::string error;
    assert(replay.restoreSnapshot(actual,&error) &&error.empty());
    assert(ablated.restoreSnapshot(noReceipt,&error) &&error.empty());

    bool differentIssuedAction=false;
    int firstDifferenceMinute=-1;
    int matchedMinutes=0;
    std::size_t observedLogs=0;
    int observedLearnedTokens=0;
    int executedLearnedTokens=0;
    std::map<std::uint64_t,ResourceNodeId> reportedTokens;
    std::set<std::uint64_t> executedTokens;
    const std::string marker="AL1_CORE_SUCCESS_MEMORY_CHOICE actor="
        +std::to_string(actorId);
    constexpr int CompareMinutes=1440; // full one-day firsthand-memory horizon
    for(int m=0;m<CompareMinutes;++m){
        replay.step();
        ablated.step();
        assert(replay.world().minute==ablated.world().minute);
        const auto& observed=replay.logs();
        while(observedLogs<observed.size()){
            const auto& message=observed[observedLogs++];
            if(message.find(marker)!=std::string::npos){
                const auto parse=[&](const std::string& field){
                    const std::size_t at=message.find(field);
                    assert(at!=std::string::npos);
                    return std::stoull(message.substr(at+field.size()));
                };
                const auto token=parse(" pending_token=");
                const auto selected=parse(" selected_source=");
                assert(token>0 &&selected>0);
                const auto pending=replay.observePendingContextAction(actorId);
                // Same-tick genuinely issued token, not a model proposal.
                assert(pending.active &&pending.token==token
                    &&pending.kind==ContextActionKind::Civilization
                    &&pending.civilizationIntent==CivilizationIntent::Gather
                    &&pending.resourceNode==selected);
                assert(reportedTokens.emplace(token,selected).second);
                ++observedLearnedTokens;
            }
            if(message.find("AL1_CORE_GATHER_RESULT actor="
                +std::to_string(actorId))!=std::string::npos){
                for(const auto& [token,selected]:reportedTokens){
                    if(message.find(" pending_token="+std::to_string(token)
                        +" source="+std::to_string(selected))
                            !=std::string::npos
                       &&executedTokens.insert(token).second)
                        ++executedLearnedTokens;
                }
            }
        }
        const auto a=replay.observePendingContextAction(actorId);
        const auto b=ablated.observePendingContextAction(actorId);
        const bool different=
            a.active!=b.active
            ||(a.active &&b.active
                &&(a.kind!=b.kind
                   ||a.civilizationIntent!=b.civilizationIntent
                   ||a.resourceNode!=b.resourceNode
                   ||a.material!=b.material));
        if(different){
            differentIssuedAction=true;
            firstDifferenceMinute=replay.world().minute;
            break;
        }
        ++matchedMinutes;
    }
    std::cout<<"AL1_NATURAL_SUCCESS_MEMORY_ABLATION seed="<<seed
             <<" actor="<<actorId
             <<" source="<<source
             <<" real_success_minute="<<eventMinute
             <<" direct_memory=1"
             <<" learned_preference="<<remembered.utilityAdjustment
             <<" without_memory_preference="<<without.utilityAdjustment
             <<" different_issued_choice="<<differentIssuedAction
             <<" first_difference_minute="<<firstDifferenceMinute
             <<" compared_minutes="<<matchedMinutes
             <<" actual_learned_token_count="<<observedLearnedTokens
             <<" physically_completed_learned_tokens="<<executedLearnedTokens
             <<" acceptance="
             <<(differentIssuedAction
                    ?"ACTUAL_CHOICE_CAUSALITY_OBSERVED":"NOT_PROVEN")
             <<"\n";
}
int main()
{
    inspectSeed(874213954ULL);
    inspectSeed(4242001ULL);
    return 0;
}
