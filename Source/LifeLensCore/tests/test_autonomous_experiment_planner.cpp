#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

#include "lifelens/AutonomousExperimentPlanner.h"
#include "lifelens/ResidentCivilizationOutcomeLearning.h"
#include "lifelens/ResidentCivilizationOutcomeLearning.h"

using namespace lifelens;

static CognitiveRequest observe(
    const World& world,
    const RelationshipBook& relationships,
    CharacterId actor)
{
    const Character* character=cognitiveCharacter(world,actor);
    assert(character!=nullptr);
    return buildCognitiveRequest(
        *character,relationships,world.minute,CognitiveTriggerKind::Reflection,
        cognitiveStrategicIntentCatalog(),
        DefaultCognitiveMemoryContextLimit,
        DefaultCognitiveBeliefContextLimit,
        DefaultCognitiveRelationshipContextLimit);
}

static AutonomousGoalCandidate trustGoal(
    const CognitiveRequest& request,CharacterId target)
{
    AutonomousGoalCandidate goal;
    goal.actor=request.actor;
    goal.createdMinute=request.minute;
    goal.expiresMinute=request.minute+1440;
    goal.priority=0.7;
    goal.evidence={{AutonomousGoalEvidenceKind::Related,0}};

    AutonomousGoalPredicate trust;
    trust.metric=AutonomousGoalMetric::RelationshipTrust;
    trust.targetResident=target;
    trust.comparison=AutonomousGoalCompare::AtLeast;
    trust.threshold=0.85;
    goal.desiredState={trust};
    return goal;
}

static AutonomousGoalCandidate restGoal(const CognitiveRequest& request)
{
    AutonomousGoalCandidate goal;
    goal.actor=request.actor;
    goal.createdMinute=request.minute;
    goal.expiresMinute=request.minute+1440;
    goal.priority=0.7;
    goal.evidence={{AutonomousGoalEvidenceKind::ActorNeed,0}};
    AutonomousGoalPredicate sleep;
    sleep.metric=AutonomousGoalMetric::ActorNeed;
    sleep.need=AutonomousNeedKind::Sleep;
    sleep.comparison=AutonomousGoalCompare::AtMost;
    sleep.threshold=0.20;
    goal.desiredState={sleep};
    return goal;
}

int main()
{
    World world(45921);
    world.characters.clear();
    Character actor;
    actor.id=11;
    actor.needs.sleep=0.54;
    actor.needs.hunger=0.20;
    actor.needs.thirst=0.20;
    actor.needs.bladder=0.20;
    actor.needs.hygiene=0.20;
    actor.personality.sociability=0.9;
    Character neighbor;
    neighbor.id=22;
    world.characters={actor,neighbor};

    RelationshipBook relationships;
    auto& relation=relationships.getOrCreate(11,22);
    relation.trust=0.40;
    relation.affection=0.75;
    relation.comfort=0.70;
    relation.conflict=0.25;
    assert(scoreRepairIntent(world.characters.front(),22,relationships)>0.0);
    assert(scoreApproachIntent(world,world.characters.front(),22,relationships)>0.0);

    const auto request=observe(world,relationships,11);
    const auto goal=trustGoal(request,22);
    assert(validateAutonomousGoalCandidate(world,request,goal).accepted);
    const auto initialRng=world.rng;
    const int initialMinute=world.minute;
    const double initialTrust=relation.trust;
    const double initialHunger=world.characters[0].needs.hunger;

    // Competing methods for the SAME existing, valid Core social intention.
    const auto options=composeAutonomousExperimentOptions(
        world,relationships,request,goal);
    assert(options.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(options.actor==11 &&options.minute==world.minute);
    assert(options.contextFingerprint==cognitiveRequestFingerprint(request));
    assert(options.alternatives.size()==2);
    assert(options.alternatives[0].preferenceScore
        >=options.alternatives[1].preferenceScore);
    bool foundApproach=false,foundRepair=false;
    for(const auto& candidate:options.alternatives){
        assert(candidate.steps.size()==1);
        const auto& step=candidate.steps.front();
        assert(step.kind==AutonomousAffordanceKind::ExistingSocialIntent);
        assert(step.target==22);
        assert(step.goalPredicateIndex==0);
        assert(candidate.effortUnits<=MaxAutonomousExperimentEffortUnits);
        assert(candidate.riskProxy<=0.45);
        assert(candidate.preferenceScore>0.0);
        if(step.socialIntent==SocialIntent::Approach)
            foundApproach=true;
        else if(step.socialIntent==SocialIntent::Repair)
            foundRepair=true;
        else assert(false);
    }
    assert(foundApproach &&foundRepair);
    // A previously issued proposal cannot become executable merely because
    // it once appeared in a candidate list. Revalidate the exact steps,
    // costs, context and Core facts before a later dispatcher's own checks.
    const auto selected=options.alternatives.front();
    assert(revalidateAutonomousExperimentChoice(
        world,relationships,request,goal,options,selected)
        ==AutonomousExperimentRevalidation::CurrentCandidate);
    auto forgedOption=selected;
    forgedOption.effortUnits++;
    assert(revalidateAutonomousExperimentChoice(
        world,relationships,request,goal,options,forgedOption)
        ==AutonomousExperimentRevalidation::NoLongerAdmissible);
    auto forgedUtility=selected;
    forgedUtility.preferenceScore=std::numeric_limits<double>::quiet_NaN();
    assert(revalidateAutonomousExperimentChoice(
        world,relationships,request,goal,options,forgedUtility)
        ==AutonomousExperimentRevalidation::NoLongerAdmissible);
    World laterWorld=world;
    ++laterWorld.minute;
    assert(revalidateAutonomousExperimentChoice(
        laterWorld,relationships,request,goal,options,selected)
        ==AutonomousExperimentRevalidation::StaleContext);
    auto successRelationships=relationships;
    successRelationships.getOrCreate(11,22).trust=0.95;
    const auto satisfiedRequest=observe(world,successRelationships,11);
    // Old receipt is now stale, and a fresh query must observe true success.
    assert(revalidateAutonomousExperimentChoice(
        world,successRelationships,satisfiedRequest,goal,options,selected)
        ==AutonomousExperimentRevalidation::StaleContext);
    assert(composeAutonomousExperimentOptions(
        world,successRelationships,satisfiedRequest,
        trustGoal(satisfiedRequest,22)).status
        ==AutonomousExperimentStatus::AlreadySatisfied);
    AutonomousExperimentBudget noRisks;
    noRisks.maxRiskProxy=0.01;
    assert(revalidateAutonomousExperimentChoice(
        world,relationships,request,goal,options,selected,noRisks)
        ==AutonomousExperimentRevalidation::NoLongerAdmissible);
    noRisks.maxSteps=0;
    assert(revalidateAutonomousExperimentChoice(
        world,relationships,request,goal,options,selected,noRisks)
        ==AutonomousExperimentRevalidation::InvalidBudget);

    // Identical input, seed, context and budget must replay bit-for-bit
    // in candidate ordering without touching RNG or world state.
    for(int repeat=0;repeat<20;++repeat){
        const auto replay=composeAutonomousExperimentOptions(
            world,relationships,request,goal);
        assert(replay.status==options.status);
        assert(replay.contextFingerprint==options.contextFingerprint);
        assert(replay.alternatives.size()==options.alternatives.size());
        for(std::size_t i=0;i<options.alternatives.size();++i){
            assert(identicalAutonomousExperimentSteps(
                replay.alternatives[i].steps,options.alternatives[i].steps));
            assert(replay.alternatives[i].preferenceScore
                ==options.alternatives[i].preferenceScore);
        }
    }
    assert(world.rng==initialRng);
    assert(world.minute==initialMinute);
    assert(world.characters[0].needs.hunger==initialHunger);
    assert(relationships.find(11,22)->trust==initialTrust);

    // Budget limits are real fail-closed selection gates, not magic resource
    // deposits or time advances. Repair requires 2 abstract effort units.
    AutonomousExperimentBudget budget;
    budget.maxEffortUnits=1;
    auto constrained=composeAutonomousExperimentOptions(
        world,relationships,request,goal,budget);
    assert(constrained.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(constrained.alternatives.size()==1);
    assert(constrained.alternatives[0].steps[0].socialIntent
        ==SocialIntent::Approach);

    budget=AutonomousExperimentBudget{};
    budget.maxRiskProxy=0.01;
    constrained=composeAutonomousExperimentOptions(
        world,relationships,request,goal,budget);
    assert(constrained.status==AutonomousExperimentStatus::NoSafeCandidate);
    assert(constrained.alternatives.empty());

    budget=AutonomousExperimentBudget{};
    budget.maxAlternatives=1;
    constrained=composeAutonomousExperimentOptions(
        world,relationships,request,goal,budget);
    assert(constrained.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(constrained.alternatives.size()==1);

    budget=AutonomousExperimentBudget{};
    budget.maxRiskProxy=std::numeric_limits<double>::quiet_NaN();
    assert(composeAutonomousExperimentOptions(
        world,relationships,request,goal,budget).status
        ==AutonomousExperimentStatus::InvalidBudget);
    budget=AutonomousExperimentBudget{};
    budget.maxSteps=0;
    assert(composeAutonomousExperimentOptions(
        world,relationships,request,goal,budget).status
        ==AutonomousExperimentStatus::InvalidBudget);
    budget=AutonomousExperimentBudget{};
    budget.maxAlternatives=MaxAutonomousExperimentAlternatives+1;
    assert(composeAutonomousExperimentOptions(
        world,relationships,request,goal,budget).status
        ==AutonomousExperimentStatus::InvalidBudget);

    // A rising-but-not-yet-urgent body need must defer optional experiment
    // methods. It must not block a genuine Core Sleep relief candidate.
    World tiredWorld=world;
    tiredWorld.characters.front().needs.sleep=0.56;
    const auto tiredRequest=observe(tiredWorld,relationships,11);
    const auto tiredSocial=trustGoal(tiredRequest,22);
    assert(composeAutonomousExperimentOptions(
        tiredWorld,relationships,tiredRequest,tiredSocial).status
        ==AutonomousExperimentStatus::NoSafeCandidate);
    const auto tiredRest=restGoal(tiredRequest);
    const auto recovery=composeAutonomousExperimentOptions(
        tiredWorld,relationships,tiredRequest,tiredRest);
    assert(recovery.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(recovery.alternatives.size()==1);
    assert(recovery.alternatives[0].steps.front().physicalGoal==Goal::Sleep);
    assert(tiredWorld.minute==world.minute);
    assert(tiredWorld.rng==world.rng);

    // The SAME resident learns personal self-mood from a real prior Core
    // social completion, already persisted as an actor-specific belief.
    // This differentiates safe ways to pursue one goal WITHOUT changing
    // resources, target permissions, the world RNG or physical outcomes.
    const auto preferenceFor=[](
        const AutonomousExperimentOptions& candidates,SocialIntent intent){
        for(const auto& option:candidates.alternatives){
            if(option.steps.size()==1
               &&option.steps.front().socialIntent==intent)
                return option.preferenceScore;
        }
        return -1.0;
    };
    const double neutralApproach=preferenceFor(options,SocialIntent::Approach);
    const double neutralRepair=preferenceFor(options,SocialIntent::Repair);
    assert(neutralApproach>0.0 &&neutralRepair>0.0);
    World benefited=world;
    auto& positiveBelief=benefited.characters.front().beliefs.getOrCreate(
        22,socialMoodExperienceProposition(SocialIntent::Approach));
    positiveBelief.applyEvidence(true,2.0,benefited.minute-1);
    const auto benefitedRequest=observe(benefited,relationships,11);
    const auto benefitedOptions=composeAutonomousExperimentOptions(
        benefited,relationships,benefitedRequest,trustGoal(benefitedRequest,22));
    assert(benefitedOptions.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(preferenceFor(benefitedOptions,SocialIntent::Approach)
        >neutralApproach);
    assert(std::abs(preferenceFor(benefitedOptions,SocialIntent::Repair)
        -neutralRepair)<1e-12);

    World harmed=world;
    auto& negativeBelief=harmed.characters.front().beliefs.getOrCreate(
        22,socialMoodExperienceProposition(SocialIntent::Approach));
    negativeBelief.applyEvidence(false,2.0,harmed.minute-1);
    const auto harmedRequest=observe(harmed,relationships,11);
    const auto harmedOptions=composeAutonomousExperimentOptions(
        harmed,relationships,harmedRequest,trustGoal(harmedRequest,22));
    assert(harmedOptions.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(preferenceFor(harmedOptions,SocialIntent::Approach)
        <neutralApproach);
    assert(std::abs(preferenceFor(harmedOptions,SocialIntent::Repair)
        -neutralRepair)<1e-12);
    assert(world.rng==initialRng);
    assert(world.characters.front().beliefs.beliefs.empty());
    // Another resident's experience is not credited to this resident.
    World otherMind=world;
    otherMind.characters[1].beliefs.getOrCreate(
        11,socialMoodExperienceProposition(SocialIntent::Approach))
        .applyEvidence(false,2.0,otherMind.minute-1);
    const auto otherRequest=observe(otherMind,relationships,11);
    const auto otherOptions=composeAutonomousExperimentOptions(
        otherMind,relationships,otherRequest,trustGoal(otherRequest,22));
    assert(std::abs(preferenceFor(otherOptions,SocialIntent::Approach)
        -neutralApproach)<1e-12);

    // Without an actual conflict, Core has no Repair affordance.
    relation.conflict=0.0;
    const auto oneMethod=composeAutonomousExperimentOptions(
        world,relationships,observe(world,relationships,11),
        trustGoal(observe(world,relationships,11),22));
    assert(oneMethod.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(oneMethod.alternatives.size()==1);
    assert(oneMethod.alternatives[0].steps[0].socialIntent
        ==SocialIntent::Approach);
    relation.conflict=0.25;

    // Goal proposals forged from a different physical/mental context fail,
    // even if the caller claims to be the same resident.
    auto forged=request;
    forged.needs.hunger+=0.01;
    assert(composeAutonomousExperimentOptions(
        world,relationships,forged,goal).status
        ==AutonomousExperimentStatus::RejectedContext);
    forged=request;
    CognitiveMemoryEvidence invented;
    invented.who=22;
    invented.what="invented success";
    forged.memories.push_back(invented);
    assert(composeAutonomousExperimentOptions(
        world,relationships,forged,goal).status
        ==AutonomousExperimentStatus::RejectedContext);

    world.minute+=1;
    assert(composeAutonomousExperimentOptions(
        world,relationships,request,goal).status
        ==AutonomousExperimentStatus::RejectedContext);
    world.minute-=1;

    // Urgent survival still overrides social exploration.
    world.characters[0].needs.hunger=0.99;
    const auto urgent=observe(world,relationships,11);
    const auto suppressed=composeAutonomousExperimentOptions(
        world,relationships,urgent,trustGoal(urgent,22));
    assert(suppressed.status==AutonomousExperimentStatus::NoSafeCandidate);
    assert(suppressed.alternatives.empty());
    world.characters[0].needs.hunger=initialHunger;

    // Physical goals stay bound to existing provisions, never creating food.
    const auto restRequest=observe(world,relationships,11);
    const auto sleep=composeAutonomousExperimentOptions(
        world,relationships,restRequest,restGoal(restRequest));
    assert(sleep.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(sleep.alternatives.size()==1);
    assert(sleep.alternatives[0].steps[0].physicalGoal==Goal::Sleep);
    assert(sleep.alternatives[0].steps[0].target==0);

    // A resident's current observed material goal can be compared as ONE
    // read-only Core Gather alternative. Actual extraction is not a model
    // effect, and the node identity must participate in candidate equality.
    auto localRequest=observe(world,relationships,11);
    const auto localSource=observeLocalResourceProximity(
        world,11,world.resourceNodes.front().pos,3);
    assert(localSource.actor==11 &&!localSource.nearbyResources.empty());
    localRequest.localProximityObserved=true;
    localRequest.localObserverPosition=localSource.observerPosition;
    localRequest.localRadiusCells=localSource.radiusCells;
    localRequest.localResources=localSource.nearbyResources;
    AutonomousGoalCandidate materialGoal;
    materialGoal.actor=11;
    materialGoal.createdMinute=world.minute;
    materialGoal.expiresMinute=world.minute+1440;
    materialGoal.priority=0.7;
    AutonomousGoalPredicate raw;
    raw.metric=AutonomousGoalMetric::ActorRawMaterialCount;
    raw.material=localRequest.localResources[0].material;
    raw.comparison=AutonomousGoalCompare::AtLeast;
    raw.threshold=1.0;
    materialGoal.desiredState={raw};
    materialGoal.evidence={{
        AutonomousGoalEvidenceKind::ObservedLocalResource,0
    }};
    const auto realCandidate=composeAutonomousExperimentOptions(
        world,relationships,localRequest,materialGoal);
    assert(realCandidate.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(realCandidate.alternatives.size()==1);
    const auto& gather=realCandidate.alternatives.front().steps;
    assert(gather.size()==1);
    assert(gather[0].kind==AutonomousAffordanceKind::ExistingCivilizationGather);
    assert(gather[0].resourceNode==localRequest.localResources[0].nodeId);
    // Actual personal outcomes change only THIS resident's nonbinding
    // read-only experiment preference; they never conjure a harvested item.
    assert(realCandidate.alternatives[0].preferenceScore==0.0);
    CivilizationUtilityDecision observedGather;
    observedGather.intent=CivilizationIntent::Gather;
    observedGather.material=gather[0].material;
    observedGather.resourceNode=gather[0].resourceNode;
    World learnedGather=world;
    rememberCivilizationAttemptOutcome(
        learnedGather.characters.front(),observedGather,world.minute,true);
    auto learnedRequest=observe(learnedGather,relationships,11);
    learnedRequest.localProximityObserved=true;
    learnedRequest.localObserverPosition=localRequest.localObserverPosition;
    learnedRequest.localRadiusCells=localRequest.localRadiusCells;
    learnedRequest.localResources=localRequest.localResources;
    const auto learnedOptions=composeAutonomousExperimentOptions(
        learnedGather,relationships,learnedRequest,materialGoal);
    assert(learnedOptions.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(learnedOptions.alternatives.size()==1);
    assert(learnedOptions.alternatives[0].preferenceScore
        >realCandidate.alternatives[0].preferenceScore);
    World failedGather=world;
    rememberCivilizationAttemptOutcome(
        failedGather.characters.front(),observedGather,world.minute,false,
        ResidentGatherFailureCause::ResourceAccessUnavailable);
    auto failedRequest=observe(failedGather,relationships,11);
    failedRequest.localProximityObserved=true;
    failedRequest.localObserverPosition=localRequest.localObserverPosition;
    failedRequest.localRadiusCells=localRequest.localRadiusCells;
    failedRequest.localResources=localRequest.localResources;
    const auto failedOptions=composeAutonomousExperimentOptions(
        failedGather,relationships,failedRequest,materialGoal);
    assert(failedOptions.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(failedOptions.alternatives.size()==1);
    assert(failedOptions.alternatives[0].preferenceScore
        <realCandidate.alternatives[0].preferenceScore);
    assert(world.rng==initialRng);
    assert(world.characters.front().memory.entries.empty());
    assert(learnedGather.resourceNodes.front().quantity
        ==world.resourceNodes.front().quantity);
    // Another physically present Core source is chosen when this actor
    // experienced a true ACCESS failure at the closest resource.
    World reroute=world;
    auto alternativeNode=reroute.resourceNodes.front();
    alternativeNode.id=std::numeric_limits<ResourceNodeId>::max()-4;
    alternativeNode.quantity=10;
    // Limit the receipt to these two actual nodes; unrelated generated
    // resources cannot exhaust the 8-evidence goal budget in this test.
    reroute.resourceNodes={reroute.resourceNodes.front(),alternativeNode};
    CivilizationUtilityDecision inaccessible;
    inaccessible.intent=CivilizationIntent::Gather;
    inaccessible.material=alternativeNode.material;
    inaccessible.resourceNode=reroute.resourceNodes.front().id;
    rememberCivilizationAttemptOutcome(
        reroute.characters.front(),inaccessible,reroute.minute,false,
        ResidentGatherFailureCause::ResourceAccessUnavailable);
    auto rerouteRequest=observe(reroute,relationships,11);
    const auto rerouteSensed=observeLocalResourceProximity(
        reroute,11,reroute.resourceNodes.front().pos,3);
    assert(rerouteSensed.nearbyResources.size()>=2);
    rerouteRequest.localProximityObserved=true;
    rerouteRequest.localObserverPosition=rerouteSensed.observerPosition;
    rerouteRequest.localRadiusCells=rerouteSensed.radiusCells;
    rerouteRequest.localResources=rerouteSensed.nearbyResources;
    auto rerouteGoal=materialGoal;
    rerouteGoal.evidence.clear();
    for(std::size_t i=0;i<rerouteRequest.localResources.size();++i)
        if(rerouteRequest.localResources[i].material
            ==rerouteGoal.desiredState.front().material)
            rerouteGoal.evidence.push_back({
                AutonomousGoalEvidenceKind::ObservedLocalResource,i});
    assert(validateAutonomousGoalCandidate(
        reroute,rerouteRequest,rerouteGoal).accepted);
    const auto rerouted=draftAutonomousAffordancePlan(
        reroute,relationships,rerouteRequest,rerouteGoal);
    assert(rerouted.status==AutonomousAffordancePlanStatus::CandidateOnly);
    assert(rerouted.proposedSteps.size()==1);
    assert(rerouted.proposedSteps.front().resourceNode==alternativeNode.id);
    assert(reroute.resourceNodes.front().quantity
        ==world.resourceNodes.front().quantity);
    assert(reroute.resourceNodes.back().quantity==10);
    assert(reroute.characters[1].memory.entries.empty());
    assert(reroute.rng==initialRng);

    // M2-08: a more distant (but genuinely observed) resource consumes
    // more hypothetical exploration effort and risk budget. No Core action
    // occurs, and small budgets reject the speculative alternative.
    World distantWorld=world;
    const auto nearPos=distantWorld.resourceNodes.front().pos;
    auto distantNode=distantWorld.resourceNodes.front();
    distantNode.id=std::numeric_limits<ResourceNodeId>::max()-1;
    distantNode.pos={nearPos.x+4,nearPos.y};
    distantNode.quantity=10;
    distantWorld.resourceNodes.push_back(distantNode);
    auto distantRequest=observe(distantWorld,relationships,11);
    const auto distantView=observeLocalResourceProximity(
        distantWorld,11,nearPos,6);
    const auto distantFact=std::find_if(
        distantView.nearbyResources.begin(),
        distantView.nearbyResources.end(),
        [&](const ObservedLocalResource& fact){
            return fact.nodeId==distantNode.id;
        });
    assert(distantFact!=distantView.nearbyResources.end());
    distantRequest.localProximityObserved=true;
    distantRequest.localObserverPosition=distantView.observerPosition;
    distantRequest.localRadiusCells=distantView.radiusCells;
    distantRequest.localResources={*distantFact};
    auto distantGoal=materialGoal;
    distantGoal.desiredState.front().material=distantNode.material;
    distantGoal.evidence={{AutonomousGoalEvidenceKind::ObservedLocalResource,0}};
    const auto distantOptions=composeAutonomousExperimentOptions(
        distantWorld,relationships,distantRequest,distantGoal);
    assert(distantOptions.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(distantOptions.alternatives.size()==1);
    assert(distantOptions.alternatives.front().effortUnits==3);
    assert(distantOptions.alternatives.front().riskProxy>realCandidate.alternatives.front().riskProxy);
    AutonomousExperimentBudget tooLittleEffort;
    tooLittleEffort.maxEffortUnits=2;
    const auto blockedEffort=composeAutonomousExperimentOptions(
        distantWorld,relationships,distantRequest,distantGoal,tooLittleEffort);
    assert(blockedEffort.status==AutonomousExperimentStatus::NoSafeCandidate);
    AutonomousExperimentBudget tooMuchRisk;
    tooMuchRisk.maxRiskProxy=0.05;
    const auto blockedRisk=composeAutonomousExperimentOptions(
        distantWorld,relationships,distantRequest,distantGoal,tooMuchRisk);
    assert(blockedRisk.status==AutonomousExperimentStatus::NoSafeCandidate);
    auto differentNode=gather;
    differentNode[0].resourceNode+=1;
    assert(!identicalAutonomousExperimentSteps(gather,differentNode));
    auto emptySource=world;
    for(auto& node:emptySource.resourceNodes){
        if(node.id==gather[0].resourceNode) node.quantity=0;
    }
    const auto noInventedStock=composeAutonomousExperimentOptions(
        emptySource,relationships,localRequest,materialGoal);
    assert(noInventedStock.status==AutonomousExperimentStatus::RejectedGoal);
    assert(noInventedStock.alternatives.empty());

    // Completed desired state never triggers unnecessary experiments.
    world.characters[0].needs.sleep=0.1;
    const auto rested=observe(world,relationships,11);
    assert(composeAutonomousExperimentOptions(
        world,relationships,rested,restGoal(rested)).status
        ==AutonomousExperimentStatus::AlreadySatisfied);

    // Unknown actor and a resident's death cannot yield executable options.
    assert(composeAutonomousExperimentOptions(
        world,relationships,observe(world,relationships,22),
        trustGoal(rested,22)).alternatives.empty());
    world.characters[1].alive=false;
    const auto afterDeath=observe(world,relationships,11);
    assert(composeAutonomousExperimentOptions(
        world,relationships,afterDeath,trustGoal(afterDeath,22))
        .alternatives.empty());

    assert(world.rng==initialRng);
    assert(world.minute==initialMinute);
    assert(relationships.find(11,22)->trust==initialTrust);
    return 0;
}
