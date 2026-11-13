#include "lifelens/Simulation.h"
#include "lifelens/ResidentCognitivePreference.h"
#include <limits>
namespace lifelens {
namespace {
const char* contextParentingActionName(ParentingAction action)
{
    switch(action){
        case ParentingAction::Feed: return "Feed";
        case ParentingAction::PutToSleep: return "PutToSleep";
        case ParentingAction::Bathe: return "Bathe";
        case ParentingAction::ToiletAssist: return "ToiletAssist";
        case ParentingAction::Hold: return "Hold";
        case ParentingAction::Play: return "Play";
        case ParentingAction::Educate: return "Educate";
        case ParentingAction::Discipline: return "Discipline";
        case ParentingAction::Comfort: return "Comfort";
        case ParentingAction::HealthCare: return "HealthCare";
    }
    return "Care";
}

}

void Simulation::recordCognitiveProposal(const AcceptedCognitiveDecision& origin,bool accepted,CognitiveTraceRejection reason) {
    if(origin.actor==0 ||origin.contextFingerprint==0 ||!cognitiveCharacter(world_,origin.actor))return;
    if(!cognitiveCausalJournal_.empty() &&cognitiveCausalJournal_.back().id==std::numeric_limits<std::uint64_t>::max())return;
    CognitiveCausalTrace t;t.id=cognitiveCausalJournal_.empty()?1:cognitiveCausalJournal_.back().id+1;
    t.origin=origin;t.accepted=accepted;t.rejection=reason;
    if(!std::isfinite(t.origin.priority))t.origin.priority=0;
    t.origin.priority=std::clamp(t.origin.priority,0.0,1.0);
    t.origin.backendIdentity.resize(std::min<std::size_t>(128,t.origin.backendIdentity.size()));
    t.origin.rationaleDigest.resize(std::min<std::size_t>(128,t.origin.rationaleDigest.size()));
    cognitiveCausalJournal_.push_back(std::move(t));
    if(cognitiveCausalJournal_.size()>MaxCognitiveTraces)cognitiveCausalJournal_.erase(cognitiveCausalJournal_.begin());
}
void Simulation::recordCognitiveStep(const AcceptedCognitiveDecision& origin,const PendingContextAction& pending,CognitiveTraceSource source) {
    if(!pending.active() ||origin.actor==0 ||origin.minute>world_.minute)return;
    if(std::none_of(acceptedCognitiveDecisions_.begin(),acceptedCognitiveDecisions_.end(),[&](const auto& d){return sameCognitiveOrigin(d,origin);}))return;
    CognitiveCausalTrace* trace=nullptr;
    for(auto& t:cognitiveCausalJournal_)if(t.accepted &&sameCognitiveOrigin(t.origin,origin))trace=&t;
    // Legacy saves retain the actual accepted journal. Rehydrate only that
    // proven approval when a NEW real step is issued, never historical results.
    if(!trace){recordCognitiveProposal(origin,true,CognitiveTraceRejection::None);
        if(cognitiveCausalJournal_.empty() ||!sameCognitiveOrigin(cognitiveCausalJournal_.back().origin,origin) ||!cognitiveCausalJournal_.back().accepted)return;
        trace=&cognitiveCausalJournal_.back();}
    for(const auto& t:cognitiveCausalJournal_)for(const auto& s:t.steps)if(s.token==pending.token)return;
    CognitiveCausalStep s;s.token=pending.token;s.issuedMinute=pending.issuedMinute;s.kind=pending.kind;s.source=source;
    const auto observation=lifelens::observePendingContextAction(origin.actor,pending);s.target=observation.targetResident;
    if(pending.kind==ContextActionKind::Civilization){
        s.action=civilizationIntentName(pending.civilization.intent);s.material=materialName(pending.civilization.material);
        s.technique=techniqueName(pending.civilization.technique);
    }else if(pending.kind==ContextActionKind::Social)s.action=socialIntentName(pending.social.intent);
    else if(pending.kind==ContextActionKind::KnowledgeTeaching){s.action="Teach";s.technique=techniqueName(pending.knowledgeTeachingTechnique);}
    else if(pending.kind==ContextActionKind::Parenting)s.action=contextParentingActionName(pending.parentingAction);
    else s.action="Trade";
    if(trace->steps.size()==MaxCognitiveTraceSteps){trace->steps.erase(trace->steps.begin());++trace->omittedSteps;}
    trace->steps.push_back(std::move(s));
}
void Simulation::inheritCognitiveStep(CharacterId actor,std::uint64_t previous,const PendingContextAction& pending) {
    if(previous==0)return;
    for(auto& t:cognitiveCausalJournal_)if(t.origin.actor==actor &&t.accepted)
        for(const auto& s:t.steps)if(s.token==previous){
            const auto origin=t.origin;recordCognitiveStep(origin,pending,CognitiveTraceSource::Continuation);
            for(auto& next:cognitiveCausalJournal_)for(auto& step:next.steps)if(step.token==pending.token)step.previousToken=previous;
            return;
        }
}
void Simulation::recordPreferredCognitiveStep(CharacterId actor,const PendingContextAction& pending,CognitiveIntentKind intent,CharacterId target) {
    // This is an explicitly used positive utility contribution, not a claim
    // that the same action would be impossible without the model preference.
    if(residentCognitiveIntentPreference(world_,acceptedCognitiveDecisions_,actor,intent,target)<=0)return;
    for(auto it=acceptedCognitiveDecisions_.rbegin();it!=acceptedCognitiveDecisions_.rend();++it)if(it->actor==actor){
        if(it->intent==intent &&it->target==target)recordCognitiveStep(*it,pending,CognitiveTraceSource::Preference);
        return;
    }
}
void Simulation::finishCognitiveStep(CharacterId actor,std::uint64_t token,CognitiveStepResult result,const std::string& reason,bool attempted,const CognitiveLearningEvidence& learning) {
    if(token==0)return;
    for(auto& t:cognitiveCausalJournal_)if(t.origin.actor==actor)
        for(auto& s:t.steps)if(s.token==token &&s.result==CognitiveStepResult::Pending){
            s.result=result;s.resultMinute=world_.minute;s.reason=reason;s.attempted=attempted;s.learning=learning;return;
        }
}

void Simulation::recordCognitiveExchange(CharacterId actor,CharacterId target,std::uint64_t fact,const CognitiveLearningEvidence& learning) {
    if(fact==0)return;
    for(auto it=acceptedCognitiveDecisions_.rbegin();it!=acceptedCognitiveDecisions_.rend();++it)if(it->actor==actor){
        if(it->intent!=CognitiveIntentKind::TradeWithResident ||it->target!=target)return;
        const auto origin=*it;
        CognitiveCausalTrace* trace=nullptr;
        for(auto& t:cognitiveCausalJournal_)if(t.accepted &&sameCognitiveOrigin(t.origin,origin))trace=&t;
        if(!trace){recordCognitiveProposal(origin,true,CognitiveTraceRejection::None);
        if(cognitiveCausalJournal_.empty() ||!sameCognitiveOrigin(cognitiveCausalJournal_.back().origin,origin) ||!cognitiveCausalJournal_.back().accepted)return;
        trace=&cognitiveCausalJournal_.back();}
        for(const auto& s:trace->steps)if(s.exchangeFact==fact)return;
        CognitiveCausalStep s;s.exchangeFact=fact;s.target=target;s.kind=ContextActionKind::Trade;
        s.action="Exchange";s.issuedMinute=s.resultMinute=world_.minute;s.source=CognitiveTraceSource::Preference;
        s.result=CognitiveStepResult::Succeeded;s.reason="registered-local-exchange";s.attempted=true;s.learning=learning;
        if(trace->steps.size()==MaxCognitiveTraceSteps){trace->steps.erase(trace->steps.begin());++trace->omittedSteps;}
        trace->steps.push_back(std::move(s));return;
    }
}

void Simulation::reconcileCognitiveSteps() {
    for(auto& t:cognitiveCausalJournal_)for(auto& s:t.steps)if(s.result==CognitiveStepResult::Pending){
        const auto r=runtime_.find(t.origin.actor);
        if(r==runtime_.end() ||r->second.pendingContext.token!=s.token ||!r->second.pendingContext.active()){
            // No execution callback: record closure, never invent success,
            // failure, an exact execution time, or learned experience.
            s.result=CognitiveStepResult::Cancelled;s.resultMinute=world_.minute;s.reason="closed-without-execution-evidence";
        }
    }
}
}
