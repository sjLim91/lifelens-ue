#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <string>

#include "lifelens/Simulation.h"
#include "lifelens/ResidentResourceOutcomePreference.h"

using namespace lifelens;

// No test fixture, hand-issued intent, altered traits/needs, extra resources,
// fake AI output, or injected cognition. Parse only ACTUALLY emitted Core logs.
// A zero-count outcome is an honest negative observation, NEVER acceptance.
static std::uint64_t unsignedField(
    const std::string& text,const std::string& key)
{
    const auto found=text.find(key);
    assert(found!=std::string::npos);
    std::size_t at=found+key.size();
    assert(at<text.size() &&text[at]>='0' &&text[at]<='9');
    std::uint64_t value=0;
    while(at<text.size() &&text[at]>='0' &&text[at]<='9'){
        const auto digit=static_cast<std::uint64_t>(text[at++]-'0');
        assert(value<=(std::numeric_limits<std::uint64_t>::max()-digit)/10);
        value=value*10+digit;
    }
    return value;
}

struct IssuedRecovery {
    CharacterId actor=0;
    ResourceNodeId selected=0;
    bool succeeded=false;
};

static void inspectNaturalGame(WorldSeed seed,int days)
{
    Simulation simulation(seed);
    simulation.setupNewGame();
    assert(simulation.world().characters.size()==4);
    std::map<std::uint64_t,IssuedRecovery> pending;
    std::size_t logCursor=0;
    int actualIssued=0;
    int physicallyCompleted=0;
    int actualFailures=0;
    std::map<std::uint64_t,ResourceNodeId> failures;
    std::map<std::uint64_t,PendingContextActionObservation> issuedActions;
    simulation.onEvent([&](const std::string& line){
        if(line.find("AL1_CORE_GOAL_ISSUED actor=")==std::string::npos) return;
        const auto action=simulation.observePendingContextAction(
            unsignedField(line," actor="));
        if(action.active) issuedActions.emplace(action.token,action);
    });

    // Run the real simulation for the requested bounded observational period,
    // not a claim that route failure must occur. Core owns ALL choices,
    // navigation, extraction, memory creation and pending token assignment.
    for(int minute=0;minute<days*1440;++minute){
        simulation.step();
        for(const auto& actor:simulation.world().characters){
            const auto action=simulation.observePendingContextAction(actor.id);
            if(action.active) issuedActions.emplace(action.token,action);
        }
        const auto& logs=simulation.logs();
        while(logCursor<logs.size()){
            const std::string& line=logs[logCursor++];
            if(line.find("AL1_CORE_GATHER_FAILURE actor=")!=std::string::npos){
                failures.emplace(unsignedField(line," pending_token="),
                    unsignedField(line," source="));
                ++actualFailures;
                std::cout<<"AL1_NATURAL_FAILURE seed="<<seed
                    <<" result_minute="<<simulation.world().minute-1<<" "<<line<<"\n";
            }else if(line.find("AL1_CORE_RESOURCE_REPLAN actor=")
                !=std::string::npos){
                const CharacterId actorId=static_cast<CharacterId>(
                    unsignedField(line," actor="));
                const ResourceNodeId oldSource=
                    unsignedField(line," failed_source=");
                const ResourceNodeId newSource=
                    unsignedField(line," selected_source=");
                const int failedMinute=static_cast<int>(
                    unsignedField(line," failure_minute="));
                const int decisionMinute=static_cast<int>(
                    unsignedField(line," decision_minute="));
                const std::uint64_t originalToken=
                    unsignedField(line," failed_pending_token=");
                const std::uint64_t recoveryToken=
                    unsignedField(line," pending_token=");
                const bool firstNavFailure=
                    unsignedField(line," firsthand_navigation_failure=")!=0;
                assert(actorId!=0 && oldSource!=0 && newSource!=0
                    &&oldSource!=newSource &&recoveryToken!=0);
                assert(decisionMinute==simulation.world().minute-1
                    &&failedMinute<=decisionMinute);
                const auto issued=
                    simulation.observePendingContextAction(actorId);
                assert(issued.active &&issued.token==recoveryToken
                    &&issued.kind==ContextActionKind::Civilization
                    &&issued.civilizationIntent==CivilizationIntent::Gather
                    &&issued.resourceNode==newSource);
                const Character* resident=nullptr;
                for(const auto& c:simulation.world().characters)
                    if(c.id==actorId)resident=&c;
                assert(resident);
                const std::string actionKey="Civilization:Gather:"
                    +std::string(materialName(issued.material));
                const auto personal=buildResidentResourceOutcomeIndex(
                    *resident,decisionMinute).find(oldSource,actionKey);
                assert(personal.hasEvidence &&!personal.succeeded
                    &&(personal.reason==ResidentResourceOutcomeReason::AccessUnavailable
                       ||personal.reason==ResidentResourceOutcomeReason::SourceUnavailable));
                if(firstNavFailure){
                    assert(originalToken!=0);
                    bool tokenWasWitnessed=false;
                    for(const auto& memory:resident->memory.entries)
                        if(memory.minute==failedMinute
                           &&memory.who==actorId
                           &&memory.sourceCharacter==actorId
                           &&memory.source==MemorySource::DirectWitness
                           &&memory.witnessed
                           &&memory.where=="resource-node:"
                                +std::to_string(oldSource)
                           &&residentGatherRouteFailurePendingToken(memory)
                                ==originalToken)
                            tokenWasWitnessed=true;
                    assert(tokenWasWitnessed);
                }else{
                    if(originalToken!=0){
                        assert(failures.at(originalToken)==oldSource);
                        const auto& previous=issuedActions.at(originalToken);
                        assert(previous.actor==actorId &&previous.resourceNode==oldSource);
                        if(previous.gatherGoal.active()){
                            assert(issued.gatherGoal.active()
                                &&issued.gatherGoal.goalId==previous.gatherGoal.goalId
                                &&issued.gatherGoal.previousToken==originalToken
                                &&issued.gatherGoal.targetQuantity==previous.gatherGoal.targetQuantity);
                        }
                    }
                }
                assert(pending.emplace(
                    recoveryToken,IssuedRecovery{actorId,newSource,false}
                ).second);
                ++actualIssued;
                std::cout<<"AL1_NATURAL_RECOVERY_ISSUED seed="<<seed<<" "<<line<<"\n";
            }else if(line.find("AL1_CORE_GATHER_RESULT actor=")
                !=std::string::npos){
                const auto token=unsignedField(line," pending_token=");
                auto it=pending.find(token);
                if(it==pending.end()) continue; // ordinary unrelated Gather
                assert(it->second.actor
                    ==static_cast<CharacterId>(
                        unsignedField(line," actor=")));
                assert(it->second.selected
                    ==unsignedField(line," source="));
                assert(unsignedField(line," actual_quantity=")>0);
                assert(!it->second.succeeded);
                it->second.succeeded=true;
                ++physicallyCompleted;
                const Character* resident=nullptr;
                for(const auto& actor:simulation.world().characters)
                    if(actor.id==it->second.actor) resident=&actor;
                assert(resident);
                bool confirmedDirectMemory=false;
                for(const auto& memory:resident->memory.entries){
                    if(memory.source!=MemorySource::DirectWitness
                       ||!memory.witnessed
                       ||memory.sourceCharacter!=resident->id
                       ||memory.minute!=simulation.world().minute-1
                       ||memory.where!="resource-node:"
                            +std::to_string(it->second.selected)
                       ||memory.tags.size()!=3
                       ||memory.tags[0]!="action_experience"
                       ||memory.tags[2]!="success") continue;
                    confirmedDirectMemory=true;
                    break;
                }
                assert(confirmedDirectMemory);
                std::cout<<"AL1_NATURAL_RECOVERY_COMPLETED seed="<<seed<<" "<<line<<"\n";
            }
        }
    }
    assert(physicallyCompleted<=actualIssued);
    std::cout<<"AL1_UNMODIFIED_NATURAL_REPLAN seed="<<seed
             <<" observed_days="<<days
             <<" issued="<<actualIssued
             <<" physically_completed="<<physicallyCompleted
             <<" actual_gather_failures="<<actualFailures
             <<" proven_complete="<<(physicallyCompleted>0)
             <<(physicallyCompleted==0
                ? " acceptance=NOT_PROVEN" : " acceptance=CHAIN_OBSERVED")
             <<"\n";
}

int main(int argc,char** argv)
{
    const int days=argc>1 ? std::stoi(argv[1]) : 14;
    assert(days>=1 &&days<=365);
    inspectNaturalGame(874213954ULL,days);
    inspectNaturalGame(4242001ULL,days);
    return 0;
}
