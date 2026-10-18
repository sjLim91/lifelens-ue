#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "lifelens/Simulation.h"

namespace lifelens {

// Read-only measurements of real Core outcomes. The intervention is a
// deterministic, hand-written proxy, NOT an LLM nor an assertion of learning.
struct ResidentAIPairedMetrics {
    std::uint64_t proposalsAttempted=0;
    std::uint64_t proposalsAccepted=0;
    std::uint64_t proposalsRejected=0;
    std::uint64_t firsthandOutcomesVisibleToProxy=0;
    std::uint64_t observedCivilizationPendingStarts=0;
    std::uint64_t observedSocialPendingStarts=0;
    std::uint64_t observedOtherPendingStarts=0;
    std::uint64_t civilizationSuccessEvidence=0;
    std::uint64_t civilizationFailureEvidence=0;
    std::uint64_t firsthandCivilizationMemories=0;
    std::uint64_t socialFacts=0;
    std::uint64_t knownTechniqueRecords=0;
    int endMinute=0;
    std::size_t livingResidents=0;
};

struct ResidentAIPairedReport {
    WorldSeed seed=0;
    int days=0;
    bool matchedInitialResidents=false;
    ResidentAIPairedMetrics baseline;
    ResidentAIPairedMetrics proxy;
};

inline bool residentAIHasAllowedIntent(
    const CognitiveRequest& request,CognitiveIntentKind intent)
{
    return std::find(request.allowedIntents.begin(),request.allowedIntents.end(),
                     intent)!=request.allowedIntents.end();
}

// Observe the actual pending token, not a model's proposed action.
// A pending start is NOT a successful execution.
inline void observeResidentAIPendingStarts(
    const Simulation& sim,
    ResidentAIPairedMetrics& totals,
    std::map<CharacterId,std::uint64_t>& lastSeenToken)
{
    for(const Character& actor:sim.world().characters){
        const auto pending=sim.observePendingContextAction(actor.id);
        if(!pending.active) continue;
        auto& last=lastSeenToken[actor.id];
        if(last==pending.token) continue;
        last=pending.token;
        switch(pending.kind){
            case ContextActionKind::Civilization:
                ++totals.observedCivilizationPendingStarts;
                break;
            case ContextActionKind::Social:
                ++totals.observedSocialPendingStarts;
                break;
            default:
                ++totals.observedOtherPendingStarts;
                break;
        }
    }
}

inline void collectResidentAIPairedEvidence(
    const Simulation& sim,ResidentAIPairedMetrics& totals)
{
    totals.endMinute=sim.world().minute;
    totals.socialFacts=sim.socialKnowledge().facts().size();
    constexpr const char* prefix="experienced-action-outcome:Civilization:";
    const std::string needle(prefix);
    for(const Character& actor:sim.world().characters){
        if(actor.alive) ++totals.livingResidents;
        totals.knownTechniqueRecords+=actor.civilization.knowledge.all().size();
        for(const BeliefRecord& belief:actor.beliefs.beliefs){
            if(belief.subject!=0 ||belief.proposition.compare(
                    0,needle.size(),needle)!=0) continue;
            // Count evidence, not model promises, memory entries or attempted
            // plans. The Core's evidence recorder can deduplicate a minute.
            totals.civilizationSuccessEvidence+=belief.supportCount;
            totals.civilizationFailureEvidence+=belief.contradictionCount;
        }
        for(const MemoryRecord& memory:actor.memory.entries){
            if(memory.source==MemorySource::DirectWitness
               &&memory.witnessed &&memory.sourceCharacter==actor.id
               &&memory.tags.size()==3
               &&memory.tags[0]=="action_experience"
               &&memory.tags[1].compare(0,needle.size(),needle)==0)
                ++totals.firsthandCivilizationMemories;
        }
    }
}

inline bool residentAIProxyProposal(
    Simulation& sim,
    ResidentAIPairedMetrics& stats,
    std::uint64_t slot)
{
    std::vector<CharacterId> eligible;
    for(const Character& actor:sim.world().characters){
        if(actor.alive &&!requiresDirectCare(actor.lifeStage))
            eligible.push_back(actor.id);
    }
    if(eligible.empty()) return false;
    const CharacterId actor=eligible[slot%eligible.size()];
    const auto request=sim.observeCognitiveRequest(
        actor,CognitiveTriggerKind::Reflection);
    if(request.actor!=actor ||request.allowedIntents.empty()) return false;
    CognitiveProposal proposal;
    proposal.actor=actor;
    proposal.priority=0.9;

    // Fixed policy intentionally ignores learned beliefs. This audit tests
    // Core's causal handoff and observability, NOT adaptive model quality.
    switch(slot%4){
        case 0: proposal.intent=CognitiveIntentKind::ImproveFoodSecurity; break;
        case 1: proposal.intent=CognitiveIntentKind::ImproveWaterSecurity; break;
        case 2: proposal.intent=CognitiveIntentKind::ExploreOpportunity; break;
        default: {
            proposal.intent=CognitiveIntentKind::CooperateWithResident;
            for(const CharacterId candidate:eligible){
                if(candidate!=actor){proposal.targetResident=candidate;break;}
            }
            if(proposal.targetResident==0)
                proposal.intent=CognitiveIntentKind::ImproveFoodSecurity;
            break;
        }
    }
    if(!residentAIHasAllowedIntent(request,proposal.intent)) return false;
    for(const auto& memory:request.memories){
        if(memory.source==MemorySource::DirectWitness &&memory.witnessed
           &&memory.tags.size()==3
           &&memory.tags[0]=="action_experience")
            ++stats.firsthandOutcomesVisibleToProxy;
    }
    ++stats.proposalsAttempted;
    // Never supply resources, move a resident, change Needs or forge a memory.
    const bool accepted=sim.acceptCognitiveProposal(
        request,proposal,"deterministic-audit-proxy","audit-proxy-v1");
    if(accepted) ++stats.proposalsAccepted;
    else ++stats.proposalsRejected;
    return accepted;
}

// Two truly separate New Games with identical initial seeds. The only
// intervention is accepted model-like preference events in the proxy branch.
// Core still owns all movement, survival, real execution and outcomes.
inline ResidentAIPairedReport runResidentAIPairedAudit(WorldSeed seed,int days)
{
    if(seed==0 ||days<1 ||days>1000)
        throw std::invalid_argument("seed must be positive and days in 1..1000");
    Simulation baseline(seed);
    Simulation scripted(seed);
    baseline.setupNewGame();
    scripted.setupNewGame();

    ResidentAIPairedReport report;
    report.seed=seed;
    report.days=days;
    const auto& initialBase=baseline.world().characters;
    const auto& initialProxy=scripted.world().characters;
    report.matchedInitialResidents=
        baseline.world().minute==scripted.world().minute
        && initialBase.size()==initialProxy.size();
    if(report.matchedInitialResidents){
        for(std::size_t i=0;i<initialBase.size();++i){
            if(initialBase[i].id!=initialProxy[i].id
               ||initialBase[i].name!=initialProxy[i].name
               ||initialBase[i].sex!=initialProxy[i].sex){
                report.matchedInitialResidents=false;break;
            }
        }
    }
    if(!report.matchedInitialResidents)
        throw std::runtime_error("paired New Game initialization mismatch");

    std::map<CharacterId,std::uint64_t> baseTokens,scriptTokens;
    const int totalMinutes=days*1440;
    std::uint64_t slot=0;
    for(int i=0;i<totalMinutes;++i){
        if(scripted.world().minute%15==0)
            residentAIProxyProposal(scripted,report.proxy,slot++);
        baseline.step();
        scripted.step();
        observeResidentAIPendingStarts(
            baseline,report.baseline,baseTokens);
        observeResidentAIPendingStarts(
            scripted,report.proxy,scriptTokens);
    }
    collectResidentAIPairedEvidence(baseline,report.baseline);
    collectResidentAIPairedEvidence(scripted,report.proxy);
    if(report.baseline.endMinute!=report.proxy.endMinute)
        throw std::runtime_error("paired simulation clock divergence");
    return report;
}

inline void appendResidentAIPairedMetricsJson(
    std::ostringstream& out,const ResidentAIPairedMetrics& m)
{
    out<<"{\"proposalsAttempted\":"<<m.proposalsAttempted
       <<",\"proposalsAccepted\":"<<m.proposalsAccepted
       <<",\"proposalsRejected\":"<<m.proposalsRejected
       <<",\"firsthandOutcomesVisibleToProxy\":"
       <<m.firsthandOutcomesVisibleToProxy
       <<",\"observedCivilizationPendingStarts\":"
       <<m.observedCivilizationPendingStarts
       <<",\"observedSocialPendingStarts\":"<<m.observedSocialPendingStarts
       <<",\"observedOtherPendingStarts\":"<<m.observedOtherPendingStarts
       <<",\"civilizationSuccessEvidence\":"
       <<m.civilizationSuccessEvidence
       <<",\"civilizationFailureEvidence\":"
       <<m.civilizationFailureEvidence
       <<",\"firsthandCivilizationMemories\":"
       <<m.firsthandCivilizationMemories
       <<",\"socialFacts\":"<<m.socialFacts
       <<",\"knownTechniqueRecords\":"<<m.knownTechniqueRecords
       <<",\"endMinute\":"<<m.endMinute
       <<",\"livingResidents\":"<<m.livingResidents<<"}";
}

inline std::string residentAIPairedReportJson(
    const ResidentAIPairedReport& report)
{
    std::ostringstream out;
    out<<"{\"seed\":"<<report.seed<<",\"days\":"<<report.days
       <<",\"policy\":\"fixed-deterministic-proxy-not-llm\""
       <<",\"matchedInitialResidents\":"
       <<(report.matchedInitialResidents?"true":"false")
       <<",\"baseline\":";
    appendResidentAIPairedMetricsJson(out,report.baseline);
    out<<",\"proxy\":";
    appendResidentAIPairedMetricsJson(out,report.proxy);
    out<<"}";
    return out.str();
}

} // namespace lifelens
