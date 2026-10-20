#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "lifelens/Simulation.h"
#include "lifelens/ResidentAIContextLifecycleAudit.h"

namespace lifelens {

// Read-only measurements of real Core outcomes. The intervention is a
// deterministic, hand-written proxy, NOT an LLM nor an assertion of learning.
enum class ResidentAIPairedPolicy {
    OriginalFour,
    CoreOpportunityFour,
    CatalogFourteen
};
inline constexpr std::size_t FullCognitiveCatalogCount=14;
inline constexpr std::size_t CognitiveOpportunityIntentCount=4;
inline constexpr const char* CognitiveOpportunityIntentNames[]={
    "TeachKnowledge","CareForDependent","TradeWithResident","MigrateHousehold"
};
inline std::size_t cognitiveOpportunityIndex(CognitiveIntentKind intent)
{
    switch(intent){
        case CognitiveIntentKind::TeachKnowledge: return 0;
        case CognitiveIntentKind::CareForDependent: return 1;
        case CognitiveIntentKind::TradeWithResident: return 2;
        case CognitiveIntentKind::MigrateHousehold: return 3;
        default: return CognitiveOpportunityIntentCount;
    }
}
struct ResidentAIPairedMetrics {
    std::uint64_t proposalsAttempted=0;
    std::uint64_t proposalsAccepted=0;
    std::uint64_t proposalsRejected=0;
    std::uint64_t firsthandOutcomesVisibleToProxy=0;
    std::uint64_t observedCivilizationPendingStarts=0;
    std::uint64_t observedSocialPendingStarts=0;
    std::uint64_t observedOtherPendingStarts=0;
    std::uint64_t observedKnowledgeTeachingPendingStarts=0;
    std::uint64_t observedTradePendingStarts=0;
    std::uint64_t observedParentingPendingStarts=0;
    OpportunityContextLifecycleCounts opportunityTokenLifecycle{};
    OpportunityAcceptedOutcomeCounts acceptedOutcomeEvidence{};
    // These counters come exclusively from Core's actual execution paths,
    // never inferred from a disappearing pending token or accepted AI goal.
    std::array<std::uint64_t,ObservedOpportunityContextCount>
        verifiedCoreSuccesses{};
    std::array<std::uint64_t,ObservedOpportunityContextCount>
        verifiedCoreFailures{};
    std::uint64_t verifiedTradeJourneyReturns=0;
    // Proposals are not actions. These counts show input acceptances only,
    // never credit successful instruction, childcare, exchange or migration.
    std::array<std::uint64_t,CognitiveOpportunityIntentCount>
        opportunityProposalsAttempted{};
    std::array<std::uint64_t,CognitiveOpportunityIntentCount>
        opportunityProposalsAccepted{};
    std::array<std::uint64_t,CognitiveOpportunityIntentCount>
        opportunityNoEligibleTarget{};
    // Selection/availability/acceptance cannot be mistaken for execution.
    std::array<std::uint64_t,FullCognitiveCatalogCount>
        catalogSelected{},catalogNotOffered{},catalogNoEligibleTarget{},
        catalogSubmitted{},catalogAccepted{},catalogRejected{};
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
    ResidentAIPairedPolicy policy=ResidentAIPairedPolicy::OriginalFour;
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
        switch(pending.kind){
            case ContextActionKind::KnowledgeTeaching:
                ++totals.observedKnowledgeTeachingPendingStarts;
                break;
            case ContextActionKind::Trade:
                ++totals.observedTradePendingStarts;
                break;
            case ContextActionKind::Parenting:
                ++totals.observedParentingPendingStarts;
                break;
            default: break;
        }
    }
}

inline void observeResidentAIVerifiedCoreOutcome(
    const CoreContextOutcomeEvent& event,
    ResidentAIPairedMetrics& totals)
{
    if(event.actor==0 ||event.token==0 ||event.minute<0) return;
    const auto index=observedOpportunityContextIndex(event.kind);
    if(index>=ObservedOpportunityContextCount) return;
    if(coreContextOutcomeActualSuccess(event.code))
        ++totals.verifiedCoreSuccesses[index];
    else if(coreContextOutcomeActualFailure(event.code))
        ++totals.verifiedCoreFailures[index];
    else if(event.code==CoreContextOutcomeCode::TradeReturned)
        ++totals.verifiedTradeJourneyReturns;
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
    std::uint64_t slot,
    ResidentAIPairedPolicy policy=ResidentAIPairedPolicy::OriginalFour)
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
    if(request.actor!=actor) return false;
    CognitiveProposal proposal;
    proposal.actor=actor;
    proposal.priority=0.9;

    // Fixed policies intentionally ignore learned beliefs. Each input is a
    // controlled preference, never a command to issue or complete an action.
    const auto fullCatalog=cognitiveStrategicIntentCatalog();
    const bool catalogPolicy=policy==ResidentAIPairedPolicy::CatalogFourteen;
    const std::size_t catalogSlot=
        catalogPolicy?static_cast<std::size_t>(slot%fullCatalog.size()):0;
    if(catalogPolicy){
        proposal.intent=fullCatalog[catalogSlot];
        ++stats.catalogSelected[catalogSlot];
    }else if(policy==ResidentAIPairedPolicy::CoreOpportunityFour){
        switch(slot%4){
            case 0: proposal.intent=CognitiveIntentKind::TeachKnowledge; break;
            case 1: proposal.intent=CognitiveIntentKind::CareForDependent; break;
            case 2: proposal.intent=CognitiveIntentKind::TradeWithResident; break;
            default: proposal.intent=CognitiveIntentKind::MigrateHousehold; break;
        }
        if(cognitiveIntentTargetsResident(proposal.intent)){
            for(const auto& candidate:sim.world().characters){
                if(!candidate.alive ||candidate.id==actor) continue;
                if(proposal.intent==CognitiveIntentKind::CareForDependent
                   &&!isDependentStage(candidate.lifeStage)) continue;
                proposal.targetResident=candidate.id;
                break;
            }
        }
    }else{
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
    }
    // Audit never invents a peer or bypasses an actual eligibility gate.
    if(catalogPolicy &&cognitiveIntentTargetsResident(proposal.intent)){
        for(const auto& candidate:sim.world().characters){
            if(!candidate.alive ||candidate.id==actor) continue;
            if(proposal.intent==CognitiveIntentKind::CareForDependent
               &&!isDependentStage(candidate.lifeStage)) continue;
            proposal.targetResident=candidate.id;
            break;
        }
    }
    const std::size_t chosenOpportunity=
        cognitiveOpportunityIndex(proposal.intent);
    if(cognitiveIntentTargetsResident(proposal.intent)
       &&proposal.targetResident==0){
        if(chosenOpportunity<CognitiveOpportunityIntentCount)
            ++stats.opportunityNoEligibleTarget[chosenOpportunity];
        if(catalogPolicy) ++stats.catalogNoEligibleTarget[catalogSlot];
        return false;
    }
    if(!residentAIHasAllowedIntent(request,proposal.intent)){
        if(catalogPolicy) ++stats.catalogNotOffered[catalogSlot];
        return false;
    }
    for(const auto& memory:request.memories){
        if(memory.source==MemorySource::DirectWitness &&memory.witnessed
           &&memory.tags.size()==3
           &&memory.tags[0]=="action_experience")
            ++stats.firsthandOutcomesVisibleToProxy;
    }
    ++stats.proposalsAttempted;
    if(catalogPolicy) ++stats.catalogSubmitted[catalogSlot];
    const std::size_t opportunity=cognitiveOpportunityIndex(proposal.intent);
    if(opportunity<CognitiveOpportunityIntentCount)
        ++stats.opportunityProposalsAttempted[opportunity];
    // Never supply resources, move a resident, change Needs or forge a memory.
    const bool accepted=sim.acceptCognitiveProposal(
        request,proposal,"deterministic-audit-proxy","audit-proxy-v1");
    if(accepted){
        ++stats.proposalsAccepted;
        if(catalogPolicy) ++stats.catalogAccepted[catalogSlot];
        if(opportunity<CognitiveOpportunityIntentCount)
            ++stats.opportunityProposalsAccepted[opportunity];
    }else{
        ++stats.proposalsRejected;
        if(catalogPolicy) ++stats.catalogRejected[catalogSlot];
    }
    return accepted;
}

// Two truly separate New Games with identical initial seeds. The only
// intervention is accepted model-like preference events in the proxy branch.
// Core still owns all movement, survival, real execution and outcomes.
inline ResidentAIPairedReport runResidentAIPairedAudit(
    WorldSeed seed,int days,
    ResidentAIPairedPolicy policy=ResidentAIPairedPolicy::OriginalFour)
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
    report.policy=policy;
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

    OpportunityAcceptedOutcomeObserver baselineLinked,proxyLinked;
    baseline.onCoreContextOutcome(
        [&](const CoreContextOutcomeEvent& event){
            observeResidentAIVerifiedCoreOutcome(event,report.baseline);
            baselineLinked.observeOutcome(
                event,report.baseline.acceptedOutcomeEvidence);
        });
    scripted.onCoreContextOutcome(
        [&](const CoreContextOutcomeEvent& event){
            observeResidentAIVerifiedCoreOutcome(event,report.proxy);
            proxyLinked.observeOutcome(
                event,report.proxy.acceptedOutcomeEvidence);
        });
    std::map<CharacterId,std::uint64_t> baseTokens,scriptTokens;
    OpportunityContextLifecycleObserver baselineLifecycle,proxyLifecycle;
    const int totalMinutes=days*1440;
    std::uint64_t slot=0;
    for(int i=0;i<totalMinutes;++i){
        if(scripted.world().minute%15==0)
            residentAIProxyProposal(scripted,report.proxy,slot++,policy);
        baseline.step();
        scripted.step();
        observeResidentAIPendingStarts(
            baseline,report.baseline,baseTokens);
        observeResidentAIPendingStarts(
            scripted,report.proxy,scriptTokens);
        baselineLifecycle.observeSimulation(
            baseline,report.baseline.opportunityTokenLifecycle);
        proxyLifecycle.observeSimulation(
            scripted,report.proxy.opportunityTokenLifecycle);
        // Register only tokens actually seen pending after this step.
        // Immediate in-step completions have no observed token and must
        // never be credited to a proposal.
        baselineLinked.observeSimulation(baseline);
        proxyLinked.observeSimulation(scripted);
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
       <<",\"observedKnowledgeTeachingPendingStarts\":"<<m.observedKnowledgeTeachingPendingStarts
       <<",\"observedTradePendingStarts\":"<<m.observedTradePendingStarts
       <<",\"observedParentingPendingStarts\":"<<m.observedParentingPendingStarts
       <<",\"civilizationSuccessEvidence\":"
       <<m.civilizationSuccessEvidence
       <<",\"civilizationFailureEvidence\":"
       <<m.civilizationFailureEvidence
       <<",\"firsthandCivilizationMemories\":"
       <<m.firsthandCivilizationMemories
       <<",\"socialFacts\":"<<m.socialFacts
       <<",\"knownTechniqueRecords\":"<<m.knownTechniqueRecords
       <<",\"endMinute\":"<<m.endMinute
       <<",\"livingResidents\":"<<m.livingResidents
       <<",\"opportunityProposalInputs\":{";
    for(std::size_t i=0;i<CognitiveOpportunityIntentCount;++i){
        if(i) out<<",";
        out<<"\""<<CognitiveOpportunityIntentNames[i]<<"\":{"
           <<"\"attempted\":"<<m.opportunityProposalsAttempted[i]
           <<",\"accepted\":"<<m.opportunityProposalsAccepted[i]
           <<",\"noEligibleTarget\":"<<m.opportunityNoEligibleTarget[i]
           <<"}";
    }
    out<<"},\"catalogIntentInputs\":{";
    const auto catalog=cognitiveStrategicIntentCatalog();
    for(std::size_t i=0;i<catalog.size();++i){
        if(i) out<<",";
        out<<"\""<<cognitiveIntentName(catalog[i])<<"\":{"
           <<"\"selected\":"<<m.catalogSelected[i]
           <<",\"notOffered\":"<<m.catalogNotOffered[i]
           <<",\"noEligibleTarget\":"<<m.catalogNoEligibleTarget[i]
           <<",\"submitted\":"<<m.catalogSubmitted[i]
           <<",\"accepted\":"<<m.catalogAccepted[i]
           <<",\"rejected\":"<<m.catalogRejected[i]<<"}";
    }
    out<<"},\"verifiedCoreOutcomes\":{";
    for(std::size_t i=0;i<ObservedOpportunityContextCount;++i){
        if(i) out<<",";
        out<<"\""<<ObservedOpportunityContextNames[i]<<"\":{"
           <<"\"success\":"<<m.verifiedCoreSuccesses[i]
           <<",\"failure\":"<<m.verifiedCoreFailures[i]<<"}";
    }
    out<<",\"tradeJourneyReturns\":"
       <<m.verifiedTradeJourneyReturns
       <<"},\"tokenLifecycle\":{";
    for(std::size_t i=0;i<ObservedOpportunityContextCount;++i){
        if(i) out<<",";
        out<<"\""<<ObservedOpportunityContextNames[i]<<"\":{"
           <<"\"observedStarts\":"
           <<m.opportunityTokenLifecycle.observedStarts[i]
           <<",\"temporalIntentTargetMatches\":"
           <<m.opportunityTokenLifecycle.temporalIntentTargetMatches[i]
           <<",\"noLongerPendingUnclassified\":"
           <<m.opportunityTokenLifecycle.noLongerPendingUnclassified[i]
           <<",\"replacedByAnotherToken\":"
           <<m.opportunityTokenLifecycle.replacedByAnotherToken[i]
           <<"}";
    }
    out<<"},\"acceptedIntentToVerifiedOutcome\":{";
    for(std::size_t i=0;i<ObservedOpportunityContextCount;++i){
        if(i) out<<",";
        out<<"\""<<ObservedOpportunityContextNames[i]<<"\":{"
           <<"\"temporallyMatchedSuccess\":"
           <<m.acceptedOutcomeEvidence.matchedSuccesses[i]
           <<",\"temporallyMatchedFailure\":"
           <<m.acceptedOutcomeEvidence.matchedFailures[i]
           <<",\"observedWithoutAcceptedIntent\":"
           <<m.acceptedOutcomeEvidence.observedWithoutMatch[i]
           <<",\"noObservedPendingToken\":"
           <<m.acceptedOutcomeEvidence.noObservedToken[i]
           <<"}";
    }
    out<<"}}";
}

inline std::string residentAIPairedReportJson(
    const ResidentAIPairedReport& report)
{
    std::ostringstream out;
    out<<"{\"seed\":"<<report.seed<<",\"days\":"<<report.days
       <<",\"policy\":\"fixed-deterministic-proxy-not-llm\""
       <<",\"proxySelection\":\""
       <<(report.policy==ResidentAIPairedPolicy::CoreOpportunityFour
             ?"four-core-opportunities"
              :report.policy==ResidentAIPairedPolicy::CatalogFourteen
              ?"all-fourteen-core-catalog":"original-four")<<"\""
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
