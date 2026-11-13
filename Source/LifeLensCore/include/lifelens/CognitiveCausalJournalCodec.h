#pragma once
#include "CognitiveCausalJournal.h"
namespace lifelens {
template<class W> void writeCognitiveCausalJournal(W& w,const std::vector<CognitiveCausalTrace>& traces) {
    w.u32(static_cast<std::uint32_t>(traces.size()));
    for(const auto& t:traces) {
        w.u64(t.id);w.u32(t.accepted?1:0);w.enumeration(t.rejection);w.u32(t.omittedSteps);
        const auto& o=t.origin;
        w.u64(o.actor);w.i32(o.minute);w.enumeration(o.trigger);w.u64(o.contextFingerprint);
        w.string(o.backendIdentity);w.enumeration(o.intent);w.real(o.priority);w.u64(o.target);
        w.string(o.rationaleDigest);w.i32(o.observedMinute);w.u64(o.observedContextFingerprint);
        w.u32(static_cast<std::uint32_t>(t.steps.size()));
        for(const auto& s:t.steps) {
            w.u64(s.token);w.u64(s.previousToken);w.u64(s.exchangeFact);w.i32(s.issuedMinute);w.i32(s.resultMinute);
            w.enumeration(s.kind);w.enumeration(s.source);w.string(s.action);w.string(s.material);w.string(s.technique);w.string(s.reason);
            w.u64(s.target);w.enumeration(s.result);w.u32(s.attempted?1:0);
            const auto& l=s.learning;w.u64(l.subject);w.string(l.proposition);w.u64(l.beforeCount);w.u64(l.afterCount);
            w.real(l.beforeStance);w.real(l.afterStance);w.u32(l.changed?1:0);
        }
    }
}
template<class R> bool readCognitiveCausalJournal(R& r,std::vector<CognitiveCausalTrace>& out) {
    std::uint32_t count=0;if(!r.count(count)||count>MaxCognitiveTraces)return false;
    std::vector<CognitiveCausalTrace> traces;
    for(std::uint32_t i=0;i<count;++i) {
        CognitiveCausalTrace t;auto& o=t.origin;std::uint32_t accepted=0,steps=0;
        if(!r.u64(t.id)||!r.u32(accepted)||accepted>1||!r.enumeration(t.rejection)||!r.u32(t.omittedSteps)
          ||!r.u64(o.actor)||!r.i32(o.minute)||!r.enumeration(o.trigger)||!r.u64(o.contextFingerprint)
          ||!r.string(o.backendIdentity)||!r.enumeration(o.intent)||!r.real(o.priority)||!r.u64(o.target)
          ||!r.string(o.rationaleDigest)||!r.i32(o.observedMinute)||!r.u64(o.observedContextFingerprint)
          ||!r.count(steps)||steps>MaxCognitiveTraceSteps)return false;
        t.accepted=accepted==1;
        for(std::uint32_t j=0;j<steps;++j) {
            CognitiveCausalStep s;auto& l=s.learning;std::uint32_t attempted=0,changed=0;
            if(!r.u64(s.token)||!r.u64(s.previousToken)||!r.u64(s.exchangeFact)||!r.i32(s.issuedMinute)||!r.i32(s.resultMinute)
              ||!r.enumeration(s.kind)||!r.enumeration(s.source)||!r.string(s.action)||!r.string(s.material)||!r.string(s.technique)||!r.string(s.reason)
              ||!r.u64(s.target)||!r.enumeration(s.result)||!r.u32(attempted)||attempted>1
              ||!r.u64(l.subject)||!r.string(l.proposition)||!r.u64(l.beforeCount)||!r.u64(l.afterCount)
              ||!r.real(l.beforeStance)||!r.real(l.afterStance)||!r.u32(changed)||changed>1)return false;
            s.attempted=attempted==1;l.changed=changed==1;t.steps.push_back(std::move(s));
        }
        traces.push_back(std::move(t));
    }
    out=std::move(traces);return true;
}
}
