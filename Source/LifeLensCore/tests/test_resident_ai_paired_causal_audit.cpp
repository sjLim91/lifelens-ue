#include <cassert>
#include <string>

#include "lifelens/ResidentAIPairedCausalAudit.h"

using namespace lifelens;

// Deliberately simulate an observed token vanishing without supplying a
// completion event. The audit must never call this success or failure.
static void verifyStrictTokenProvenance()
{
    Simulation sim(525252);
    sim.setupNewGame();
    const auto& world=sim.world();
    assert(world.characters.size()>=3);
    const auto actor=world.characters[0].id;
    const auto target=world.characters[1].id;
    const auto other=world.characters[2].id;
    AcceptedCognitiveDecision chosen;
    chosen.actor=actor;
    chosen.target=target;
    chosen.intent=CognitiveIntentKind::TeachKnowledge;
    chosen.trigger=CognitiveTriggerKind::Reflection;
    chosen.minute=world.minute;
    chosen.contextFingerprint=123456;
    chosen.priority=0.9;
    chosen.backendIdentity="test-audit";
    chosen.rationaleDigest="test-evidence";
    std::vector<AcceptedCognitiveDecision> journal;
    assert(appendAcceptedCognitiveDecision(journal,chosen,world));

    PendingContextActionObservation teaching;
    teaching.actor=actor;
    teaching.active=true;
    teaching.token=913;
    teaching.issuedMinute=world.minute;
    teaching.kind=ContextActionKind::KnowledgeTeaching;
    teaching.targetResident=target;
    assert(opportunityTokenHasMatchingAcceptedThought(
        world,journal,actor,teaching));

    OpportunityContextLifecycleObserver observer;
    OpportunityContextLifecycleCounts counts;
    observer.observe(actor,teaching,world,journal,counts);
    observer.observe(actor,teaching,world,journal,counts);
    assert(counts.observedStarts[0]==1);
    assert(counts.temporalIntentTargetMatches[0]==1);
    auto unrelated=teaching;
    unrelated.targetResident=other;
    assert(!opportunityTokenHasMatchingAcceptedThought(
        world,journal,actor,unrelated));
    // No terminal Core evidence: disappearance must remain UNCLASSIFIED.
    observer.observe(actor,PendingContextActionObservation{},
        world,journal,counts);
    assert(counts.noLongerPendingUnclassified[0]==1);
    assert(counts.replacedByAnotherToken[0]==0);

    auto trade=teaching;
    trade.kind=ContextActionKind::Trade;
    trade.token=914;
    observer.observe(actor,trade,world,journal,counts);
    assert(counts.observedStarts[2]==1);
    assert(counts.temporalIntentTargetMatches[2]==0);
    auto parenting=teaching;
    parenting.kind=ContextActionKind::Parenting;
    parenting.token=915;
    observer.observe(actor,parenting,world,journal,counts);
    assert(counts.replacedByAnotherToken[2]==1);
    assert(counts.noLongerPendingUnclassified[2]==1);
    assert(counts.observedStarts[1]==1);
    assert(counts.temporalIntentTargetMatches[1]==0);
    observer.observe(actor,PendingContextActionObservation{},
        world,journal,counts);
    assert(counts.noLongerPendingUnclassified[1]==1);

    // A newer unrelated decision supersedes the older target preference.
    auto newer=chosen;
    newer.intent=CognitiveIntentKind::ExploreOpportunity;
    newer.target=0;
    journal.push_back(newer);
    assert(!opportunityTokenHasMatchingAcceptedThought(
        world,journal,actor,teaching));
    // No attempt to "infer" MigrateHousehold completion from a context
    // token; household migration has a separate Core pathway.
}

int main()
{
    verifyStrictTokenProvenance();
    const auto first=runResidentAIPairedAudit(4242001,1);
    assert(first.seed==4242001);
    assert(first.days==1);
    assert(first.matchedInitialResidents);
    assert(first.baseline.proposalsAttempted==0);
    assert(first.baseline.proposalsAccepted==0);
    assert(first.baseline.proposalsRejected==0);
    assert(first.proxy.proposalsAttempted>0);
    assert(first.proxy.proposalsAccepted>0);
    assert(first.proxy.proposalsAttempted==
           first.proxy.proposalsAccepted+first.proxy.proposalsRejected);
    assert(first.baseline.endMinute==first.proxy.endMinute);
    assert(first.baseline.livingResidents<=4);
    assert(first.proxy.livingResidents<=4);
    const std::string output=residentAIPairedReportJson(first);
    assert(output.find("fixed-deterministic-proxy-not-llm")!=std::string::npos);
    assert(output.find("\"matchedInitialResidents\":true")!=std::string::npos);
    assert(output.find("\"civilizationSuccessEvidence\":")!=std::string::npos);
    // Repeating the identical paired scenario reproduces every numeric
    // observation, including actual Core evidence and pending starts.
    const auto same=runResidentAIPairedAudit(4242001,1);
    assert(residentAIPairedReportJson(same)==output);

    // A different initial world has its own identity and independently
    // produces valid scripted inputs without affecting the baseline branch.
    const auto another=runResidentAIPairedAudit(874213954,1);
    assert(another.matchedInitialResidents);
    assert(another.baseline.proposalsAccepted==0);
    assert(another.proxy.proposalsAccepted>0);
    assert(another.seed!=first.seed);

    // The additional intervention tests ONLY the four newly connected
    // cognition intents. Accepted intents are not completed actions.
    const auto focused=runResidentAIPairedAudit(
        4242001,1,ResidentAIPairedPolicy::CoreOpportunityFour);
    assert(focused.matchedInitialResidents);
    assert(focused.policy==ResidentAIPairedPolicy::CoreOpportunityFour);
    assert(focused.baseline.proposalsAttempted==0);
    assert(focused.proxy.proposalsAttempted>0);
    assert(focused.proxy.proposalsAccepted>0);
    assert(focused.proxy.proposalsAttempted==
           focused.proxy.proposalsAccepted+focused.proxy.proposalsRejected);
    const auto focusedJson=residentAIPairedReportJson(focused);
    assert(focusedJson.find("\"proxySelection\":\"four-core-opportunities\"")
        !=std::string::npos);
    for(const char* intent:CognitiveOpportunityIntentNames){
        assert(focusedJson.find(intent)!=std::string::npos);
    }
    assert(focusedJson.find("\"noEligibleTarget\":")!=std::string::npos);
    assert(focused.proxy.opportunityProposalsAttempted[0]>0);
    assert(focused.proxy.opportunityProposalsAttempted[2]>0);
    assert(focused.proxy.opportunityProposalsAttempted[3]>0);
    assert(focusedJson.find("\"observedTradePendingStarts\":")
        !=std::string::npos);
    assert(focusedJson.find("\"observedParentingPendingStarts\":")
        !=std::string::npos);
    assert(focusedJson.find("\"observedKnowledgeTeachingPendingStarts\":")
        !=std::string::npos);
    assert(focusedJson.find("\"tokenLifecycle\":")!=std::string::npos);
    assert(focusedJson.find("\"temporalIntentTargetMatches\":")!=std::string::npos);
    assert(focusedJson.find("\"noLongerPendingUnclassified\":")!=std::string::npos);
    assert(residentAIPairedReportJson(runResidentAIPairedAudit(
        4242001,1,ResidentAIPairedPolicy::CoreOpportunityFour))
        ==focusedJson);
    const auto focusedOther=runResidentAIPairedAudit(
        874213954,1,ResidentAIPairedPolicy::CoreOpportunityFour);
    assert(focusedOther.seed!=focused.seed);
    assert(focusedOther.matchedInitialResidents);

    bool invalidRejected=false;
    try{ (void)runResidentAIPairedAudit(0,1); }
    catch(const std::invalid_argument&){invalidRejected=true;}
    assert(invalidRejected);
    invalidRejected=false;
    try{ (void)runResidentAIPairedAudit(1,1001); }
    catch(const std::invalid_argument&){invalidRejected=true;}
    assert(invalidRejected);
    return 0;
}
