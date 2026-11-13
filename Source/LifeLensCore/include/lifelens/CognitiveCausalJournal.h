#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>
#include "CognitiveDecisionJournal.h"
#include "ContextAction.h"

namespace lifelens {
inline constexpr std::size_t MaxCognitiveTraces=64;
inline constexpr std::size_t MaxCognitiveTraceSteps=24;
enum class CognitiveTraceSource { Planner, Preference, Continuation };
enum class CognitiveStepResult { Pending, Succeeded, Failed, Cancelled, Expired, RouteFailed, Interrupted };
enum class CognitiveTraceRejection { None, CoreValidation, Expired, Duplicate };
struct CognitiveLearningEvidence {
    CharacterId subject=0;
    std::string proposition;
    std::uint64_t beforeCount=0,afterCount=0;
    double beforeStance=0,afterStance=0;
    bool changed=false;
};
struct CognitiveCausalStep {
    std::uint64_t token=0,previousToken=0,exchangeFact=0;
    int issuedMinute=-1,resultMinute=-1;
    ContextActionKind kind=ContextActionKind::None;
    CognitiveTraceSource source=CognitiveTraceSource::Planner;
    std::string action,material,technique,reason;
    CharacterId target=0;
    CognitiveStepResult result=CognitiveStepResult::Pending;
    bool attempted=false;
    CognitiveLearningEvidence learning;
};
struct CognitiveCausalTrace {
    std::uint64_t id=0;
    AcceptedCognitiveDecision origin;
    bool accepted=false;
    CognitiveTraceRejection rejection=CognitiveTraceRejection::None;
    std::uint32_t omittedSteps=0;
    std::vector<CognitiveCausalStep> steps;
};
inline bool sameCognitiveOrigin(const AcceptedCognitiveDecision& a,const AcceptedCognitiveDecision& b) {
    return a.actor==b.actor &&a.minute==b.minute &&a.contextFingerprint==b.contextFingerprint
        &&a.intent==b.intent &&a.target==b.target &&a.rationaleDigest==b.rationaleDigest
        &&a.trigger==b.trigger &&a.priority==b.priority &&a.backendIdentity==b.backendIdentity
        &&a.observedMinute==b.observedMinute &&a.observedContextFingerprint==b.observedContextFingerprint;
}
inline CognitiveLearningEvidence observeCognitiveLearning(const Character& actor,CharacterId subject,const std::string& proposition) {
    CognitiveLearningEvidence e;e.subject=subject;e.proposition=proposition;
    if(const auto* b=actor.beliefs.find(subject,proposition)) {
        e.beforeCount=e.afterCount=b->supportCount+b->contradictionCount;
        e.beforeStance=e.afterStance=b->stance;
    }
    return e;
}
inline void completeCognitiveLearning(CognitiveLearningEvidence& e,const Character& actor,int minute) {
    if(e.proposition.empty())return;
    if(const auto* b=actor.beliefs.find(e.subject,e.proposition)) {
        e.afterCount=b->supportCount+b->contradictionCount;e.afterStance=b->stance;
        e.changed=b->lastUpdatedMinute==minute &&e.afterCount>e.beforeCount;
    }
}
inline bool validCognitiveCausalJournal(const std::vector<CognitiveCausalTrace>& traces,const World& world,const std::vector<AcceptedCognitiveDecision>& accepted) {
    if(traces.size()>MaxCognitiveTraces)return false;
    std::uint64_t previous=0;std::unordered_set<std::uint64_t> tokens;
    for(const auto& t:traces) {
        if(t.id<=previous ||t.origin.actor==0 ||t.origin.minute<0 ||t.origin.minute>world.minute
           ||t.origin.contextFingerprint==0 ||t.origin.backendIdentity.size()>128 ||t.origin.rationaleDigest.size()>128
           ||!std::isfinite(t.origin.priority) ||t.origin.priority<0 ||t.origin.priority>1
           ||!cognitiveCharacter(world,t.origin.actor) ||t.steps.size()>MaxCognitiveTraceSteps
           ||static_cast<unsigned>(t.rejection)>static_cast<unsigned>(CognitiveTraceRejection::Duplicate))return false;
        previous=t.id;
        std::unordered_set<std::uint64_t> actorFacts;
        if(t.accepted) {
            if(std::any_of(traces.begin(),traces.end(),[&](const auto& p){return p.id<t.id &&p.accepted &&sameCognitiveOrigin(p.origin,t.origin);})
               ||t.rejection!=CognitiveTraceRejection::None ||!validAcceptedCognitiveDecision(t.origin,world)
              ||std::none_of(accepted.begin(),accepted.end(),[&](const auto& a){return sameCognitiveOrigin(a,t.origin);}))return false;
        }else if(t.rejection==CognitiveTraceRejection::None ||!t.steps.empty())return false;
        for(const auto& s:t.steps) {
            if(s.issuedMinute<t.origin.minute ||s.issuedMinute>world.minute ||s.kind==ContextActionKind::None
               ||static_cast<unsigned>(s.kind)>static_cast<unsigned>(ContextActionKind::Trade)
               ||static_cast<unsigned>(s.source)>static_cast<unsigned>(CognitiveTraceSource::Continuation)
               ||static_cast<unsigned>(s.result)>static_cast<unsigned>(CognitiveStepResult::Interrupted)
               ||s.action.empty() ||s.action.size()>64 ||s.material.size()>64 ||s.technique.size()>64 ||s.reason.size()>128
               ||(s.token==0 ? s.exchangeFact==0 ||s.kind!=ContextActionKind::Trade ||!actorFacts.insert(s.exchangeFact).second
                               : s.exchangeFact!=0 ||!tokens.insert(s.token).second)
               ||(s.source==CognitiveTraceSource::Continuation &&s.previousToken==0)
               ||(s.previousToken!=0 &&(s.previousToken>=s.token ||(t.omittedSteps==0 &&std::none_of(t.steps.begin(),t.steps.end(),[&](const auto& p){return p.token==s.previousToken &&p.issuedMinute<=s.issuedMinute;}))))
               ||(s.result==CognitiveStepResult::Pending ? s.resultMinute!=-1 : s.resultMinute<s.issuedMinute ||s.resultMinute>world.minute)
               ||(s.result==CognitiveStepResult::Succeeded &&!s.attempted)
               ||s.learning.proposition.size()>128 ||!std::isfinite(s.learning.beforeStance) ||!std::isfinite(s.learning.afterStance)
               ||s.learning.beforeStance< -1 ||s.learning.beforeStance>1 ||s.learning.afterStance< -1 ||s.learning.afterStance>1
               ||(s.learning.changed &&(s.result==CognitiveStepResult::Pending ||!s.attempted ||s.learning.afterCount<=s.learning.beforeCount ||s.learning.proposition.empty())))return false;
        }
    }
    return true;
}
} // namespace lifelens
