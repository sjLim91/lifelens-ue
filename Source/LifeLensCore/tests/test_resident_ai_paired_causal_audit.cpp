#include <cassert>
#include <string>

#include "lifelens/ResidentAIPairedCausalAudit.h"
#include "lifelens/ResidentCivilizationOutcomeLearning.h"

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

    // Only a LIVE observed token + matching last accepted actor/target
    // plus an actual Core terminal event qualifies for temporal attribution.
    OpportunityAcceptedOutcomeObserver matchedObserver;
    OpportunityAcceptedOutcomeCounts evidenceLinks;
    matchedObserver.observePending(actor,teaching,world,journal);
    matchedObserver.observeOutcome(
        {actor,target,teaching.token,world.minute,
         ContextActionKind::KnowledgeTeaching,
         CoreContextOutcomeCode::TeachingAdvanced},evidenceLinks);
    assert(evidenceLinks.matchedSuccesses[0]==1);
    // Duplicate callback or never-observed token is NOT another matched result.
    matchedObserver.observeOutcome(
        {actor,target,teaching.token,world.minute,
         ContextActionKind::KnowledgeTeaching,
         CoreContextOutcomeCode::TeachingAdvanced},evidenceLinks);
    assert(evidenceLinks.matchedSuccesses[0]==1);
    assert(evidenceLinks.noObservedToken[0]==1);
    auto otherPerson=teaching;
    otherPerson.token=934;
    otherPerson.targetResident=other;
    matchedObserver.observePending(actor,otherPerson,world,journal);
    matchedObserver.observeOutcome(
        {actor,other,otherPerson.token,world.minute,
         ContextActionKind::KnowledgeTeaching,
         CoreContextOutcomeCode::TeachingComprehensionFailed},evidenceLinks);
    assert(evidenceLinks.observedWithoutMatch[0]==1);
    matchedObserver.observeOutcome(
        {actor,target,999,world.minute,
         ContextActionKind::KnowledgeTeaching,
         CoreContextOutcomeCode::TeachingRejected},evidenceLinks);
    assert(evidenceLinks.noObservedToken[0]==2);
    // A return from a real trade does not imply an additional exchange.
    matchedObserver.observeOutcome(
        {actor,target,1000,world.minute,
         ContextActionKind::Trade,CoreContextOutcomeCode::TradeReturned},
         evidenceLinks);
    assert(evidenceLinks.noObservedToken[2]==0);

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

// Audit inputs are defensive: impossible channel/time provenance cannot
// become a successful completed action, even if actor/target/token match.
static void verifyOutcomeAuditGuards()
{
    Simulation sim(525252);
    sim.setupNewGame();
    const auto& world=sim.world();
    const auto actor=world.characters[0].id,target=world.characters[1].id;
    AcceptedCognitiveDecision thought;
    thought.actor=actor;
    thought.target=target;
    thought.intent=CognitiveIntentKind::TeachKnowledge;
    thought.trigger=CognitiveTriggerKind::Reflection;
    thought.minute=world.minute;
    thought.contextFingerprint=123456;
    thought.priority=0.9;
    thought.backendIdentity="test-audit";
    thought.rationaleDigest="test-evidence";
    std::vector<AcceptedCognitiveDecision> journal;
    assert(appendAcceptedCognitiveDecision(journal,thought,world));
    PendingContextActionObservation pending;
    pending.actor=actor;
    pending.targetResident=target;
    pending.active=true;
    pending.kind=ContextActionKind::KnowledgeTeaching;
    pending.token=200;
    pending.issuedMinute=world.minute;
    OpportunityAcceptedOutcomeObserver observer;
    OpportunityAcceptedOutcomeCounts counts;
    observer.observePending(actor,pending,world,journal);
    observer.observeOutcome({actor,target,pending.token,world.minute,
        pending.kind,CoreContextOutcomeCode::ParentingPerformed},counts);
    assert(counts.matchedSuccesses[0]==0);
    observer.observeOutcome({actor,target,pending.token,world.minute-1,
        pending.kind,CoreContextOutcomeCode::TeachingAdvanced},counts);
    assert(counts.matchedSuccesses[0]==0 &&counts.noObservedToken[0]==1);
    observer.observeOutcome({actor,target,pending.token,world.minute+1,
        pending.kind,CoreContextOutcomeCode::TeachingAdvanced},counts);
    assert(counts.matchedSuccesses[0]==0 &&counts.noObservedToken[0]==2);
    observer.observeOutcome({actor,target,pending.token,world.minute,
        pending.kind,CoreContextOutcomeCode::TeachingAdvanced},counts);
    assert(counts.matchedSuccesses[0]==1);
    observer.observeOutcome({actor,target,pending.token,world.minute,
        pending.kind,CoreContextOutcomeCode::TeachingAdvanced},counts);
    assert(counts.matchedSuccesses[0]==1);
    auto invalid=pending;
    invalid.token=201;
    invalid.issuedMinute=world.minute+1;
    assert(!opportunityTokenHasMatchingAcceptedThought(world,journal,actor,invalid));
    observer.observePending(actor,invalid,world,journal);
    observer.observeOutcome({actor,target,invalid.token,world.minute+1,
        invalid.kind,CoreContextOutcomeCode::TeachingAdvanced},counts);
    assert(counts.matchedSuccesses[0]==1);
    OpportunityContextLifecycleObserver lifecycle;
    OpportunityContextLifecycleCounts starts;
    lifecycle.observe(actor,invalid,world,journal,starts);
    invalid=pending;
    invalid.actor=target; // a target's observation cannot belong to this actor
    lifecycle.observe(actor,invalid,world,journal,starts);
    assert(starts.observedStarts[0]==0);

    ResidentAIPairedMetrics metrics;
    const CoreContextOutcomeEvent success{1,2,100,60,
        ContextActionKind::KnowledgeTeaching,CoreContextOutcomeCode::TeachingAdvanced};
    observeResidentAIVerifiedCoreOutcome(success,metrics);
    observeResidentAIVerifiedCoreOutcome(success,metrics); // duplicate callback
    auto contradicted=success;
    contradicted.code=CoreContextOutcomeCode::TeachingRejected;
    observeResidentAIVerifiedCoreOutcome(contradicted,metrics); // same terminal token
    assert(metrics.verifiedCoreSuccesses[0]==1 &&metrics.verifiedCoreFailures[0]==0);
    assert(metrics.residents.at(1).verifiedCoreSuccesses[0]==1);
    assert(metrics.residents.find(2)==metrics.residents.end()); // target did not act
    auto independent=contradicted;
    independent.actor=3;
    observeResidentAIVerifiedCoreOutcome(independent,metrics);
    assert(metrics.verifiedCoreFailures[0]==1);
    assert(metrics.residents.at(1).verifiedCoreFailures[0]==0);
    assert(metrics.residents.at(3).verifiedCoreFailures[0]==1);
    auto mismatch=success;
    mismatch.actor=4;
    mismatch.kind=ContextActionKind::Parenting;
    observeResidentAIVerifiedCoreOutcome(mismatch,metrics);
    assert(metrics.residents.find(4)==metrics.residents.end());
    auto returning=success;
    returning.kind=ContextActionKind::Trade;
    returning.code=CoreContextOutcomeCode::TradeReturned;
    observeResidentAIVerifiedCoreOutcome(returning,metrics);
    observeResidentAIVerifiedCoreOutcome(returning,metrics);
    assert(metrics.verifiedTradeJourneyReturns==1);
    assert(metrics.verifiedCoreSuccesses[2]==0); // return is not exchange
    auto missingTarget=success;
    missingTarget.actor=4;
    missingTarget.target=0;
    missingTarget.token=101;
    observeResidentAIVerifiedCoreOutcome(missingTarget,metrics);
    assert(metrics.residents.find(4)==metrics.residents.end());
    missingTarget.code=CoreContextOutcomeCode::TeachingRejected;
    observeResidentAIVerifiedCoreOutcome(missingTarget,metrics);
    observeResidentAIVerifiedCoreOutcome(missingTarget,metrics);
    assert(metrics.verifiedCoreFailures[0]==2);
    assert(metrics.residents.at(4).verifiedCoreFailures[0]==1);
}

// Exercise the real writer after an actual Core resource mutation. Belief
// proposition prefixes are not memory tag prefixes; confusing them silently
// reports zero firsthand memories even when real learning occurred.
static void verifyRealCivilizationMemoryProjection()
{
    Simulation sim(525252);
    sim.setupNewGame();
    auto& world=sim.world();
    assert(world.characters.size()>=2);
    for(auto& resident:world.characters){
        resident.memory.entries.clear();
        resident.beliefs.beliefs.clear();
    }
    auto& actor=world.characters[0];
    auto& other=world.characters[1];
    GridPos position{};
    assert(sim.runtimePosition(actor.id,position));
    ResourceNodeId source=1;
    while(findCivilizationResource(world,source)!=nullptr) ++source;
    world.resourceNodes.push_back(
        {source,MaterialKind::Stone,4,4,false,0,position});
    CivilizationUtilityDecision gather;
    gather.intent=CivilizationIntent::Gather;
    gather.resourceNode=source;
    gather.material=MaterialKind::Stone;
    gather.quantity=1;
    const int before=actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Stone);
    const auto result=executeCivilizationDecisionAtPosition(
        world,actor,gather,position);
    assert(result.executed &&result.success &&result.event.quantity>0);
    assert(actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Stone)>before);
    rememberCivilizationAttemptOutcome(
        actor,gather,world.minute,result.success);
    assert(!actor.memory.entries.empty());
    const auto firsthand=actor.memory.entries.back();
    assert(firsthand.tags[1]=="Civilization:Gather:Stone");
    assert(firsthand.sourceCharacter==actor.id);
    // A copy carried by somebody else is not that person's own attempt.
    other.memory.add(firsthand);
    auto told=firsthand;
    told.source=MemorySource::ToldByOther;
    actor.memory.add(told);
    auto inferred=firsthand;
    inferred.source=MemorySource::Inferred;
    actor.memory.add(inferred);
    ResidentAIPairedMetrics metrics;
    collectResidentAIPairedEvidence(sim,metrics);
    assert(metrics.firsthandCivilizationMemories==1);
    assert(metrics.residents.at(actor.id).firsthandCivilizationMemories==1);
    assert(metrics.residents.at(other.id).firsthandCivilizationMemories==0);
    assert(metrics.civilizationSuccessEvidence==1);
    assert(metrics.residents.at(actor.id).civilizationSuccessEvidence==1);
    assert(metrics.residents.at(other.id).civilizationSuccessEvidence==0);
    // The writer itself deduplicates the same action/minute. Re-observing
    // does not turn its one actual attempt into multiple retained outcomes.
    rememberCivilizationAttemptOutcome(actor,gather,world.minute,result.success);
    ResidentAIPairedMetrics again;
    collectResidentAIPairedEvidence(sim,again);
    assert(again.firsthandCivilizationMemories==1);
    assert(again.civilizationSuccessEvidence==1);
}

static void verifyPersonalProjection(const ResidentAIPairedMetrics& metrics)
{
    std::uint64_t attempted=0,accepted=0,rejected=0,civilization=0,social=0,other=0;
    std::uint64_t successEvidence=0,failureEvidence=0,memories=0,techniques=0,returns=0;
    std::size_t living=0;
    std::array<std::uint64_t,ObservedOpportunityContextCount> successes{},failures{};
    assert(metrics.residents.size()>=4);
    for(const auto& entry:metrics.residents){
        assert(entry.first!=0);
        const auto& actor=entry.second;
        assert(actor.proposalsAttempted==actor.proposalsAccepted+actor.proposalsRejected);
        attempted+=actor.proposalsAttempted;
        accepted+=actor.proposalsAccepted;
        rejected+=actor.proposalsRejected;
        civilization+=actor.civilizationPendingStarts;
        social+=actor.socialPendingStarts;
        other+=actor.otherPendingStarts;
        successEvidence+=actor.civilizationSuccessEvidence;
        failureEvidence+=actor.civilizationFailureEvidence;
        memories+=actor.firsthandCivilizationMemories;
        techniques+=actor.knownTechniqueRecords;
        returns+=actor.verifiedTradeJourneyReturns;
        if(actor.alive) ++living;
        for(std::size_t i=0;i<ObservedOpportunityContextCount;++i){
            successes[i]+=actor.verifiedCoreSuccesses[i];
            failures[i]+=actor.verifiedCoreFailures[i];
        }
    }
    assert(attempted==metrics.proposalsAttempted);
    assert(accepted==metrics.proposalsAccepted);
    assert(rejected==metrics.proposalsRejected);
    assert(civilization==metrics.observedCivilizationPendingStarts);
    assert(social==metrics.observedSocialPendingStarts);
    assert(other==metrics.observedOtherPendingStarts);
    assert(successEvidence==metrics.civilizationSuccessEvidence);
    assert(failureEvidence==metrics.civilizationFailureEvidence);
    assert(memories==metrics.firsthandCivilizationMemories);
    assert(techniques==metrics.knownTechniqueRecords);
    assert(returns==metrics.verifiedTradeJourneyReturns);
    assert(living==metrics.livingResidents);
    assert(successes==metrics.verifiedCoreSuccesses);
    assert(failures==metrics.verifiedCoreFailures);
}

int main()
{
    verifyOutcomeAuditGuards();
    verifyRealCivilizationMemoryProjection();
    // A real Core result is authoritative; a proposed intent, pending token
    // disappearing, or an attempted double ACK is never counted as success.
    ResidentAIPairedMetrics verified;
    const auto evidence=[&](ContextActionKind kind,
                            CoreContextOutcomeCode code,
                            std::uint64_t token=11){
        observeResidentAIVerifiedCoreOutcome(
            CoreContextOutcomeEvent{1,2,token,60,kind,code},verified);
    };
    evidence(ContextActionKind::Parenting,
        CoreContextOutcomeCode::ParentingPerformed);
    evidence(ContextActionKind::KnowledgeTeaching,
        CoreContextOutcomeCode::TeachingAdvanced);
    evidence(ContextActionKind::KnowledgeTeaching,
        CoreContextOutcomeCode::TeachingComprehensionFailed,12);
    evidence(ContextActionKind::Trade,
        CoreContextOutcomeCode::TradeExchanged);
    evidence(ContextActionKind::Trade,
        CoreContextOutcomeCode::TradeExchangeRejected,12);
    evidence(ContextActionKind::Trade,
        CoreContextOutcomeCode::TradeReturned);
    evidence(ContextActionKind::Trade,
        CoreContextOutcomeCode::TradeExchanged,0); // Invalid empty token.
    assert(verified.verifiedCoreSuccesses[0]==1);
    assert(verified.verifiedCoreFailures[0]==1);
    assert(verified.verifiedCoreSuccesses[1]==1);
    assert(verified.verifiedCoreFailures[1]==0);
    assert(verified.verifiedCoreSuccesses[2]==1);
    assert(verified.verifiedCoreFailures[2]==1);
    assert(verified.verifiedTradeJourneyReturns==1);
    std::ostringstream json;
    appendResidentAIPairedMetricsJson(json,verified);
    assert(json.str().find("\"verifiedCoreOutcomes\":")!=std::string::npos);
    assert(json.str().find("\"tradeJourneyReturns\":1")!=std::string::npos);
    verifyStrictTokenProvenance();
    const auto first=runResidentAIPairedAudit(4242001,1);
    assert(first.seed==4242001);
    assert(first.days==1);
    assert(first.matchedInitialResidents);
    verifyPersonalProjection(first.baseline);
    verifyPersonalProjection(first.proxy);
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
    assert(output.find("\"residents\":{")!=std::string::npos);
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
    assert(focusedJson.find("\"acceptedIntentToVerifiedOutcome\":")
        !=std::string::npos);
    assert(focusedJson.find("\"noObservedPendingToken\":")
        !=std::string::npos);
    assert(focusedJson.find("\"temporalIntentTargetMatches\":")!=std::string::npos);
    assert(focusedJson.find("\"noLongerPendingUnclassified\":")!=std::string::npos);
    assert(residentAIPairedReportJson(runResidentAIPairedAudit(
        4242001,1,ResidentAIPairedPolicy::CoreOpportunityFour))
        ==focusedJson);
    const auto focusedOther=runResidentAIPairedAudit(
        874213954,1,ResidentAIPairedPolicy::CoreOpportunityFour);
    assert(focusedOther.seed!=focused.seed);
    assert(focusedOther.matchedInitialResidents);

    // Coverage for the entire publicly advertised Core cognitive catalog.
    // These 14 proxy selections are diagnostic INPUTS, not successful
    // executed plans. Unsupported/untargetable cases retain their reasons.
    const auto all=runResidentAIPairedAudit(
        4242001,1,ResidentAIPairedPolicy::CatalogFourteen);
    assert(all.policy==ResidentAIPairedPolicy::CatalogFourteen);
    assert(all.matchedInitialResidents);
    assert(all.baseline.proposalsAccepted==0);
    const auto fourteen=cognitiveStrategicIntentCatalog();
    assert(fourteen.size()==FullCognitiveCatalogCount);
    std::uint64_t selected=0,submitted=0,accepted=0,rejected=0;
    for(std::size_t i=0;i<fourteen.size();++i){
        assert(all.proxy.catalogSelected[i]>0);
        selected+=all.proxy.catalogSelected[i];
        submitted+=all.proxy.catalogSubmitted[i];
        accepted+=all.proxy.catalogAccepted[i];
        rejected+=all.proxy.catalogRejected[i];
        assert(all.proxy.catalogSubmitted[i]
            ==all.proxy.catalogAccepted[i]
                +all.proxy.catalogRejected[i]);
        // Distinguish Core validator denial from an otherwise valid
        // proposal rejected at the journal/commit stage.
        assert(all.proxy.catalogRejected[i]
            ==all.proxy.catalogRejectedByValidator[i]
                +all.proxy.catalogRejectedAfterValidator[i]);
        assert(all.proxy.catalogSelected[i]
            ==all.proxy.catalogNotOffered[i]
                +all.proxy.catalogNoEligibleTarget[i]
                +all.proxy.catalogSubmitted[i]);
    }
    // Every catalog input stage belongs to exactly one resident. An actor
    // rejected for lacking a target still receives the real audit counter;
    // never invent a per-person completed action for a global count.
    for(std::size_t i=0;i<fourteen.size();++i){
        std::uint64_t sumSelected=0,sumSubmitted=0,sumAccepted=0,
            sumRejected=0,sumNotOffered=0,sumNoTarget=0,
            sumValidator=0,sumCommit=0;
        for(const auto& pair:all.proxy.residents){
            const auto& person=pair.second;
            sumSelected+=person.catalogSelected[i];
            sumNotOffered+=person.catalogNotOffered[i];
            sumNoTarget+=person.catalogNoEligibleTarget[i];
            sumSubmitted+=person.catalogSubmitted[i];
            sumAccepted+=person.catalogAccepted[i];
            sumRejected+=person.catalogRejected[i];
            sumValidator+=person.catalogRejectedByValidator[i];
            sumCommit+=person.catalogRejectedAfterValidator[i];
        }
        assert(sumSelected==all.proxy.catalogSelected[i]);
        assert(sumNotOffered==all.proxy.catalogNotOffered[i]);
        assert(sumNoTarget==all.proxy.catalogNoEligibleTarget[i]);
        assert(sumSubmitted==all.proxy.catalogSubmitted[i]);
        assert(sumAccepted==all.proxy.catalogAccepted[i]);
        assert(sumRejected==all.proxy.catalogRejected[i]);
        assert(sumValidator==all.proxy.catalogRejectedByValidator[i]);
        assert(sumCommit==all.proxy.catalogRejectedAfterValidator[i]);
    }
    assert(selected>0);
    assert(submitted==all.proxy.proposalsAttempted);
    assert(accepted==all.proxy.proposalsAccepted);
    assert(rejected==all.proxy.proposalsRejected);
    const std::string allJson=residentAIPairedReportJson(all);
    assert(allJson.find("\"all-fourteen-core-catalog\"")
        !=std::string::npos);
    assert(allJson.find("\"catalogIntentInputs\":{")
        !=std::string::npos);
    assert(allJson.find("\"rejectedByValidator\":")
        !=std::string::npos);
    assert(allJson.find("\"rejectedAfterValidator\":")
        !=std::string::npos);
    assert(allJson.find("\"residents\":{")!=std::string::npos);
    for(const auto intent:fourteen)
        assert(allJson.find(std::string("\"")+cognitiveIntentName(intent)
            +"\":{")!=std::string::npos);
    assert(residentAIPairedReportJson(runResidentAIPairedAudit(
        4242001,1,ResidentAIPairedPolicy::CatalogFourteen))==allJson);

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
