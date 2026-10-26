#pragma once

#include <string>

#include "ActionExperienceLearning.h"
#include "CoreContextOutcomeEvidence.h"

namespace lifelens {

// Personal trial evidence comes only from a real Core context execution.
// These fixed keys bound belief growth per resident; target and token remain
// provenance in the individual memory, not separate permanent belief keys.
inline const char* coreContextExperienceKey(ContextActionKind kind)
{
    switch(kind){
        case ContextActionKind::KnowledgeTeaching: return "CoreContext:Teaching";
        case ContextActionKind::Parenting: return "CoreContext:Parenting";
        case ContextActionKind::Trade: return "CoreContext:Trade";
        default: return nullptr;
    }
}

inline bool consistentCoreContextExperience(const CoreContextOutcomeEvent& event)
{
    switch(event.kind){
        case ContextActionKind::KnowledgeTeaching:
            return event.code==CoreContextOutcomeCode::TeachingAdvanced
                ||event.code==CoreContextOutcomeCode::TeachingComprehensionFailed
                ||event.code==CoreContextOutcomeCode::TeachingClarityFailed
                ||event.code==CoreContextOutcomeCode::TeachingRejected;
        case ContextActionKind::Parenting:
            return event.code==CoreContextOutcomeCode::ParentingPerformed
                ||event.code==CoreContextOutcomeCode::ParentingRejected;
        case ContextActionKind::Trade:
            return event.code==CoreContextOutcomeCode::TradeExchanged
                ||event.code==CoreContextOutcomeCode::TradeExchangeRejected;
        default: return false;
    }
}

// Called by Simulation only after the actual action's authoritative outcome.
// Trade RETURN is transport, not another exchange success. No model statement,
// cancelled pending token, or mere proposal grants a lived experience.
inline void rememberCoreContextExperience(
    Character& actor,const CoreContextOutcomeEvent& event)
{
    if(!actor.alive ||actor.id==0 ||event.actor!=actor.id
       ||event.target==0 ||event.target==actor.id
       ||event.token==0 ||event.minute<0
       ||!consistentCoreContextExperience(event)) return;
    const bool success=coreContextOutcomeActualSuccess(event.code);
    if(!success &&!coreContextOutcomeActualFailure(event.code)) return;
    const char* key=coreContextExperienceKey(event.kind);
    if(key==nullptr) return;

    // A Core completion token represents ONE physical attempt. An
    // acknowledgement delivered at another minute must never teach twice.
    const std::string tokenSource="core-context-token:"
        +std::to_string(event.token);
    for(const MemoryRecord& m:actor.memory.entries)
        if(m.where==tokenSource &&m.sourceCharacter==actor.id
           &&m.source==MemorySource::DirectWitness &&m.witnessed
           &&m.tags.size()==3 &&m.tags[0]=="action_experience"
           &&m.tags[1]==key) return;

    const std::string proposition=actionExperienceProposition(key);
    const BeliefRecord* prior=actor.beliefs.find(0,proposition);
    const std::size_t priorCount=prior
        ?prior->supportCount+prior->contradictionCount:0;
    // Same action and same minute are not independent evidence; snapshot
    // stores this guard even if an external completion acknowledgement retries.
    rememberActionOutcome(actor,key,event.minute,success,true);
    const BeliefRecord* recorded=actor.beliefs.find(0,proposition);
    if(recorded==nullptr
       ||recorded->supportCount+recorded->contradictionCount==priorCount)
        return;

    const std::string resultTag=success?"success":"failure";
    for(MemoryRecord& memory:actor.memory.entries){
        if(memory.minute!=event.minute ||memory.tags.size()!=3
           ||memory.tags[0]!="action_experience"
           ||memory.tags[1]!=key ||memory.tags[2]!=resultTag
           ||memory.sourceCharacter!=actor.id
           ||memory.source!=MemorySource::DirectWitness) continue;
        memory.who=event.target;
        memory.where="core-context-token:"+std::to_string(event.token);
        memory.what="실제 행동 "+std::string(key)+" 대상 "
            +std::to_string(event.target)+" 결과: "
            +coreContextOutcomeCodeName(event.code);
        return;
    }
}


    
// Immediate C5/C6 local barter has no PendingContextAction token. It must
// never masquerade as a completed context token. The ONLY provenance for this
// different Core execution path is an actual registered exchange fact.
// This is called after executeMutualExchange succeeds and the Core has added
// that fact. Both physically present adults experienced the exchange, so both
// may learn; no model proposal or attempted exchange is counted as success.
inline void rememberVerifiedLocalExchangeExperience(
    Character& resident,CharacterId counterpart,int minute,
    std::uint64_t registeredExchangeFactId)
{
    if(!resident.alive ||resident.id==0 ||counterpart==0
       ||counterpart==resident.id ||minute<0
       ||registeredExchangeFactId==0) return;
    constexpr const char* action="CoreContext:Trade";
    const std::string factSource="core-local-exchange-fact:"
        +std::to_string(registeredExchangeFactId);
    for(const MemoryRecord& m:resident.memory.entries)
        if(m.where==factSource &&m.sourceCharacter==resident.id
           &&m.source==MemorySource::DirectWitness &&m.witnessed
           &&m.tags.size()==3 &&m.tags[0]=="action_experience"
           &&m.tags[1]==action) return;
    const std::string proposition=actionExperienceProposition(action);
    const BeliefRecord* before=resident.beliefs.find(0,proposition);
    const std::size_t beforeCount=before
        ?before->supportCount+before->contradictionCount:0;
    rememberActionOutcome(resident,action,minute,true,true);
    const BeliefRecord* after=resident.beliefs.find(0,proposition);
    if(after==nullptr
       ||after->supportCount+after->contradictionCount==beforeCount) return;
    for(MemoryRecord& memory:resident.memory.entries){
        if(memory.minute!=minute ||memory.tags.size()!=3
           ||memory.tags[0]!="action_experience"
           ||memory.tags[1]!=action ||memory.tags[2]!="success"
           ||memory.source!=MemorySource::DirectWitness
           ||memory.sourceCharacter!=resident.id) continue;
        memory.who=counterpart;
        memory.where="core-local-exchange-fact:"
            +std::to_string(registeredExchangeFactId);
        memory.what="실제 지역 교역 대상 "+std::to_string(counterpart)
            +" 물품 교환 완료 (Core exchange fact)";
        return;
    }
}

} // namespace lifelens
