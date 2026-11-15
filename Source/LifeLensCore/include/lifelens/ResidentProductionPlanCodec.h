#pragma once
#include "ResidentProductionPlan.h"
#include <unordered_set>

namespace lifelens {
inline constexpr char ProductionTailMagic[]={'L','L','P','R','D','E','N','D'};
template<class Snapshot> bool hasProductionPlans(const Snapshot& s) {
    for(const auto& r:s.runtime)if(r.second.productionPlan.id)return true;
    return false;
}
template<class Snapshot> bool validProductionPlans(const Snapshot& s) {
    std::unordered_set<std::uint64_t> tokens;
    for(const auto& entry:s.runtime) {
        const auto& p=entry.second.productionPlan;
        if(!p.id) {
            if(p.actor ||p.target!=TechniqueId::None ||p.createdMinute!=-1 ||p.expiresMinute!=-1
               ||p.finishedMinute!=-1 ||p.targetStock ||p.cognitiveFingerprint ||p.thoughtMinute!=-1
               ||p.priority!=0 ||!p.steps.empty() ||p.state!=ProductionPlanState::Active
               ||p.reason!=ProductionPlanReason::None)return false;
            continue;
        }
        if(p.actor!=entry.first ||!productionToolTechnique(p.target) ||p.steps.empty()
           ||p.steps.size()>MaxProductionSteps ||p.id!=p.steps.front().token
           ||p.createdMinute<0 ||p.createdMinute>s.world.minute
           ||p.expiresMinute<=p.createdMinute
           ||static_cast<std::int64_t>(p.expiresMinute)-p.createdMinute>ProductionPlanMinutes
           ||p.targetStock<=0 ||p.targetStock>100000
           ||!std::isfinite(p.priority) ||p.priority<0.14 ||p.priority>1
           ||static_cast<unsigned>(p.state)>static_cast<unsigned>(ProductionPlanState::Abandoned)
           ||static_cast<unsigned>(p.reason)>static_cast<unsigned>(ProductionPlanReason::NoFeasibleStep)
           ||(p.cognitiveFingerprint==0 ? p.thoughtMinute!=-1 :p.thoughtMinute<0 ||p.thoughtMinute>p.createdMinute)
           ||(p.open() ? p.finishedMinute!=-1 :p.finishedMinute<p.createdMinute ||p.finishedMinute>s.world.minute))return false;
        if(std::none_of(s.world.characters.begin(),s.world.characters.end(),[&](const auto& c){return c.id==p.actor;}))return false;
        if(p.open() &&p.cognitiveFingerprint &&std::none_of(s.acceptedCognitiveDecisions.begin(),s.acceptedCognitiveDecisions.end(),[&](const auto& a){
            return a.actor==p.actor &&a.minute==p.thoughtMinute &&a.contextFingerprint==p.cognitiveFingerprint
                &&a.intent==CognitiveIntentKind::CraftUsefulTools;
        }))return false;
        std::uint64_t previous=0;int previousMinute=p.createdMinute;
        for(std::size_t i=0;i<p.steps.size();++i) {
            const auto& step=p.steps[i];
            if(step.token<=previous ||step.token>=s.nextContextActionToken ||!tokens.insert(step.token).second
               ||step.issuedMinute<previousMinute ||step.issuedMinute>s.world.minute
               ||step.issuedMinute>=p.expiresMinute
               ||(i==0 &&step.issuedMinute!=p.createdMinute)
               ||(step.intent!=CivilizationIntent::Gather &&step.intent!=CivilizationIntent::Craft
                  &&step.intent!=CivilizationIntent::Experiment)
               ||static_cast<unsigned>(step.technique)>static_cast<unsigned>(TechniqueId::PrimitiveWrap)
               ||static_cast<unsigned>(step.material)>static_cast<unsigned>(MaterialKind::Bronze)
               ||static_cast<unsigned>(step.result)>static_cast<unsigned>(CognitiveStepResult::Interrupted)
               ||static_cast<unsigned>(step.reason)>static_cast<unsigned>(ProductionPlanReason::NoFeasibleStep)
               ||step.stockBefore<0 ||step.stockAfter<0 ||step.stockBefore>100000 ||step.stockAfter>100000
               ||(step.result==CognitiveStepResult::Succeeded &&!step.attempted))return false;
            if(step.intent==CivilizationIntent::Gather &&(!step.source ||step.material==MaterialKind::Unknown))return false;
            if(step.intent!=CivilizationIntent::Gather &&!productionToolTechnique(step.technique))return false;
            if(step.result==CognitiveStepResult::Pending) {
                const auto& pending=entry.second.pendingContext;
                if(!p.open() ||i+1!=p.steps.size() ||step.finishedMinute!=-1 ||step.attempted
                   ||pending.token!=step.token ||pending.kind!=ContextActionKind::Civilization
                   ||pending.issuedMinute!=step.issuedMinute ||pending.civilization.intent!=step.intent
                   ||pending.civilization.technique!=step.technique ||pending.civilization.material!=step.material
                   ||pending.civilization.resourceNode!=step.source)return false;
            }else if(step.finishedMinute<step.issuedMinute ||step.finishedMinute>s.world.minute)return false;
            previous=step.token;previousMinute=step.finishedMinute<0?step.issuedMinute:step.finishedMinute;
        }
        if(p.state==ProductionPlanState::Completed) {
            const auto& last=p.steps.back();
            if(last.result!=CognitiveStepResult::Succeeded ||last.technique!=p.target
               ||(last.intent!=CivilizationIntent::Craft &&last.intent!=CivilizationIntent::Experiment) ||last.stockAfter<p.targetStock
               ||last.finishedMinute!=p.finishedMinute ||p.reason!=ProductionPlanReason::None)return false;
        }
    }
    return true;
}
template<class W,class S> void writeProductionPlans(W& w,const S& s) {
    w.u32(1);std::vector<CharacterId> actors;
    for(const auto& e:s.runtime)if(e.second.productionPlan.id)actors.push_back(e.first);
    std::sort(actors.begin(),actors.end());w.u32(static_cast<std::uint32_t>(actors.size()));
    for(auto actor:actors) {
        const auto& p=s.runtime.at(actor).productionPlan;
        w.u64(p.actor);w.u64(p.id);w.enumeration(p.target);w.i32(p.createdMinute);w.i32(p.expiresMinute);
        w.i32(p.finishedMinute);w.i32(p.targetStock);w.u64(p.cognitiveFingerprint);w.i32(p.thoughtMinute);
        w.real(p.priority);w.enumeration(p.state);w.enumeration(p.reason);w.u32(static_cast<std::uint32_t>(p.steps.size()));
        for(const auto& step:p.steps) {
            w.u64(step.token);w.i32(step.issuedMinute);w.i32(step.finishedMinute);w.enumeration(step.intent);
            w.enumeration(step.technique);w.enumeration(step.material);w.u64(step.source);
            w.enumeration(step.result);w.enumeration(step.reason);w.i32(step.stockBefore);w.i32(step.stockAfter);
            w.i32(step.from.x);w.i32(step.from.y);w.i32(step.to.x);w.i32(step.to.y);w.boolean(step.attempted);
        }
    }
}
template<class R,class S> bool readProductionPlans(R& r,S& s) {
    std::uint32_t version=0,n=0;
    if(!r.u32(version) ||version!=1 ||!r.count(n) ||!n ||n>s.runtime.size())return false;
    CharacterId previous=0;
    for(std::uint32_t i=0;i<n;++i) {
        ResidentProductionPlan p;std::uint32_t steps=0;
        if(!r.u64(p.actor) ||p.actor<=previous ||!s.runtime.count(p.actor) ||!r.u64(p.id)
           ||!r.enumeration(p.target) ||!r.i32(p.createdMinute) ||!r.i32(p.expiresMinute)
           ||!r.i32(p.finishedMinute) ||!r.i32(p.targetStock) ||!r.u64(p.cognitiveFingerprint) ||!r.i32(p.thoughtMinute)
           ||!r.real(p.priority) ||!r.enumeration(p.state) ||!r.enumeration(p.reason)
           ||!r.count(steps) ||!steps ||steps>MaxProductionSteps)return false;
        previous=p.actor;
        for(std::uint32_t j=0;j<steps;++j) {
            ResidentProductionStep step;
            if(!r.u64(step.token) ||!r.i32(step.issuedMinute) ||!r.i32(step.finishedMinute) ||!r.enumeration(step.intent)
               ||!r.enumeration(step.technique) ||!r.enumeration(step.material) ||!r.u64(step.source)
               ||!r.enumeration(step.result) ||!r.enumeration(step.reason) ||!r.i32(step.stockBefore) ||!r.i32(step.stockAfter)
               ||!r.i32(step.from.x) ||!r.i32(step.from.y) ||!r.i32(step.to.x) ||!r.i32(step.to.y) ||!r.boolean(step.attempted))return false;
            p.steps.push_back(step);
        }
        s.runtime.at(p.actor).productionPlan=std::move(p);
    }
    return validProductionPlans(s);
}
} // namespace lifelens
