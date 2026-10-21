#include <cassert>
#include <cstdint>

#include "lifelens/AutonomousAffordancePlanner.h"

using namespace lifelens;

static CognitiveRequest currentRequest(
    const World& world,
    const RelationshipBook& relationships,
    CharacterId who)
{
    const Character* c=cognitiveCharacter(world,who);
    assert(c!=nullptr);
    return buildCognitiveRequest(
        *c,relationships,world.minute,CognitiveTriggerKind::Reflection,
        cognitiveStrategicIntentCatalog(),
        DefaultCognitiveMemoryContextLimit,
        DefaultCognitiveBeliefContextLimit,
        DefaultCognitiveRelationshipContextLimit);
}

static AutonomousGoalCandidate goalFor(const CognitiveRequest& request)
{
    AutonomousGoalCandidate goal;
    goal.actor=request.actor;
    goal.createdMinute=request.minute;
    goal.expiresMinute=request.minute+1440;
    goal.priority=0.7;
    goal.evidence={{AutonomousGoalEvidenceKind::ActorNeed,0}};

    AutonomousGoalPredicate rest;
    rest.metric=AutonomousGoalMetric::ActorNeed;
    rest.need=AutonomousNeedKind::Sleep;
    rest.comparison=AutonomousGoalCompare::AtMost;
    rest.threshold=0.2;
    goal.desiredState={rest};
    return goal;
}

int main()
{
    World world(45921);
    world.characters.clear();
    Character resident;
    resident.id=11;
    resident.needs.sleep=0.54;
    resident.needs.hunger=0.3;
    resident.needs.thirst=0.2;
    resident.needs.bladder=0.2;
    resident.needs.hygiene=0.2;
    resident.personality.sociability=0.9;

    Character neighbor;
    neighbor.id=22;
    world.characters={resident,neighbor};
    RelationshipBook relationships;
    relationships.getOrCreate(11,22).trust=0.25;

    auto request=currentRequest(world,relationships,11);
    auto goal=goalFor(request);
    const auto rngBefore=world.rng;
    const int minuteBefore=world.minute;
    const auto sleepBefore=world.characters[0].needs.sleep;

    auto plan=draftAutonomousAffordancePlan(
        world,relationships,request,goal);
    assert(plan.status==AutonomousAffordancePlanStatus::CandidateOnly);
    assert(plan.reason==AutonomousAffordanceReason::None);
    assert(plan.actor==11);
    assert(plan.minute==world.minute);
    assert(plan.contextFingerprint==cognitiveRequestFingerprint(request));
    assert(plan.proposedSteps.size()==1);
    assert(plan.proposedSteps[0].kind==
        AutonomousAffordanceKind::ExistingPhysicalGoal);
    assert(plan.proposedSteps[0].physicalGoal==Goal::Sleep);
    assert(plan.proposedSteps[0].target==0);
    assert(plan.proposedSteps[0].goalPredicateIndex==0);

    // Two individually valid Sleep thresholds do NOT authorize two
    // unobserved Sleep results in one plan. Core must provide new observed
    // need state before the resident considers another attempt.
    auto duplicateSleep=goalFor(request);
    auto stricterSleep=duplicateSleep.desiredState.front();
    stricterSleep.threshold=0.10;
    duplicateSleep.desiredState.push_back(stricterSleep);
    assert(validateAutonomousGoalCandidate(
        world,request,duplicateSleep).accepted);
    const auto noDoubleSleep=draftAutonomousAffordancePlan(
        world,relationships,request,duplicateSleep);
    assert(noDoubleSleep.status==AutonomousAffordancePlanStatus::Unsupported);
    assert(noDoubleSleep.reason==AutonomousAffordanceReason::UnsupportedMetric);
    assert(noDoubleSleep.proposedSteps.empty());
    assert(world.characters.front().needs.sleep==0.54);

    const auto replay=draftAutonomousAffordancePlan(
        world,relationships,request,goal);
    assert(replay.status==plan.status);
    assert(replay.contextFingerprint==plan.contextFingerprint);
    assert(replay.proposedSteps.size()==plan.proposedSteps.size());
    assert(replay.proposedSteps[0].physicalGoal==
           plan.proposedSteps[0].physicalGoal);

    // The Core may now supply bounded LOCAL proximity facts in one
    // resident's thought. They are informational, not direct execution
    // rights. Existing safe Need/Social goal drafting must still work.
    auto sensed=request;
    sensed.localProximityObserved=true;
    sensed.localObserverPosition={3,-2};
    sensed.localRadiusCells=6;
    sensed.localResources.push_back({
        512,MaterialKind::Stone,{4,-2},1,
        ObservedResourceAbundance::Present
    });
    const auto sensedPlan=draftAutonomousAffordancePlan(
        world,relationships,sensed,goal);
    assert(sensedPlan.status==AutonomousAffordancePlanStatus::CandidateOnly);
    assert(sensedPlan.proposedSteps.size()==plan.proposedSteps.size());
    assert(sensedPlan.proposedSteps.front().physicalGoal==Goal::Sleep);
    assert(sensedPlan.contextFingerprint==cognitiveRequestFingerprint(sensed));
    assert(sensedPlan.contextFingerprint!=plan.contextFingerprint);
    const auto sameSensedPlan=draftAutonomousAffordancePlan(
        world,relationships,sensed,goal);
    assert(sameSensedPlan.contextFingerprint==sensedPlan.contextFingerprint);
    auto forgedNeeds=sensed;
    forgedNeeds.needs.sleep=0.0;
    assert(draftAutonomousAffordancePlan(
        world,relationships,forgedNeeds,goal).status
        ==AutonomousAffordancePlanStatus::RejectedContext);
    auto inventedHistory=sensed;
    CognitiveMemoryEvidence invented;
    invented.what="I already learned an impossible technology";
    inventedHistory.memories.push_back(invented);
    assert(draftAutonomousAffordancePlan(
        world,relationships,inventedHistory,goal).status
        ==AutonomousAffordancePlanStatus::RejectedContext);

    // Valid social aspirations compose with physical needs without granting
    // any direct power to increase a relationship value.
    AutonomousGoalPredicate trust;
    trust.metric=AutonomousGoalMetric::RelationshipTrust;
    trust.targetResident=22;
    trust.comparison=AutonomousGoalCompare::AtLeast;
    trust.threshold=0.60;
    goal.desiredState.push_back(trust);
    goal.evidence.push_back({AutonomousGoalEvidenceKind::Related,0});
    assert(validateAutonomousGoalCandidate(world,request,goal).accepted);

    const auto combined=draftAutonomousAffordancePlan(
        world,relationships,request,goal);
    assert(combined.status==AutonomousAffordancePlanStatus::CandidateOnly);
    assert(combined.proposedSteps.size()==2);
    assert(combined.proposedSteps[0].physicalGoal==Goal::Sleep);
    assert(combined.proposedSteps[1].kind==
        AutonomousAffordanceKind::ExistingSocialIntent);
    assert(combined.proposedSteps[1].target==22);
    assert(combined.proposedSteps[1].socialIntent==SocialIntent::Approach
        ||combined.proposedSteps[1].socialIntent==SocialIntent::Repair);
    assert(combined.proposedSteps[1].goalPredicateIndex==1);
    assert(relationships.find(11,22)->trust==0.25);

    // Urgent survival cannot be demoted below a social aspiration.
    world.characters[0].needs.hunger=0.99;
    request=currentRequest(world,relationships,11);
    auto urgentGoal=goalFor(request);
    urgentGoal.desiredState={trust};
    urgentGoal.evidence={{AutonomousGoalEvidenceKind::Related,0}};
    const auto urgentPlan=draftAutonomousAffordancePlan(
        world,relationships,request,urgentGoal);
    assert(urgentPlan.status==AutonomousAffordancePlanStatus::Blocked);
    assert(urgentPlan.reason==AutonomousAffordanceReason::UrgentSurvival);
    assert(urgentPlan.proposedSteps.empty());

    // Context must always be recomputed from the authoritative current
    // character and relationship books, including memory provenance.
    auto forged=request;
    forged.needs.hunger-=0.1;
    const auto rejected=draftAutonomousAffordancePlan(
        world,relationships,forged,urgentGoal);
    assert(rejected.status==AutonomousAffordancePlanStatus::RejectedContext);
    assert(rejected.reason==AutonomousAffordanceReason::StaleOrForgedContext);
    assert(rejected.proposedSteps.empty());
    world.minute+=1;
    assert(draftAutonomousAffordancePlan(
        world,relationships,request,urgentGoal).status
        ==AutonomousAffordancePlanStatus::RejectedContext);
    world.minute-=1;

    // A missing food source cannot be "planned into existence". Provision
    // must exist in actual Core inventory before even suggesting Eat.
    request=currentRequest(world,relationships,11);
    AutonomousGoalPredicate food;
    food.metric=AutonomousGoalMetric::ActorNeed;
    food.need=AutonomousNeedKind::Hunger;
    food.comparison=AutonomousGoalCompare::AtMost;
    food.threshold=0.2;
    auto foodGoal=goalFor(request);
    foodGoal.desiredState={food};
    const auto noFood=draftAutonomousAffordancePlan(
        world,relationships,request,foodGoal);
    assert(noFood.status==AutonomousAffordancePlanStatus::Blocked);
    assert(noFood.reason==AutonomousAffordanceReason::MissingProvision);
    assert(noFood.proposedSteps.empty());

    world.characters[0].civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,1,0.5,1.0
    });
    request=currentRequest(world,relationships,11);
    foodGoal=goalFor(request);
    foodGoal.desiredState={food};
    const auto withFood=draftAutonomousAffordancePlan(
        world,relationships,request,foodGoal);
    assert(withFood.status==AutonomousAffordancePlanStatus::CandidateOnly);
    assert(withFood.proposedSteps.size()==1);
    assert(withFood.proposedSteps[0].physicalGoal==Goal::Eat);
    assert(world.characters[0].civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==1);

    // Evaluate a normal low-risk material goal independently of the
    // deliberate 0.99 hunger emergency tested above. The real Core should
    // always block optional collection while survival is urgent.
    world.characters[0].needs.hunger=0.30;
    request=currentRequest(world,relationships,11);
    // Material goals require an observed resource location and verified
    // Gather/Explore route, not an imaginary item creation primitive.
    auto materialGoal=foodGoal;
    AutonomousGoalPredicate material;
    material.metric=AutonomousGoalMetric::ActorRawMaterialCount;
    material.material=MaterialKind::Stone;
    material.comparison=AutonomousGoalCompare::AtLeast;
    material.threshold=5;
    materialGoal.desiredState={material};
    auto unavailable=draftAutonomousAffordancePlan(
        world,relationships,request,materialGoal);
    assert(unavailable.status==AutonomousAffordancePlanStatus::Unsupported);
    assert(unavailable.reason==AutonomousAffordanceReason::UnsupportedMetric);
    assert(unavailable.proposedSteps.empty());

    // One personally sensed source may justify a GATHER CANDIDATE for
    // a new material goal, but does not execute it or grant an inventory item.
    world.characters[0].needs.hunger=0.30;
    const ResourceNode actual=world.resourceNodes.front();
    auto sensedRequest=currentRequest(world,relationships,11);
    const auto proximity=observeLocalResourceProximity(
        world,11,actual.pos,3);
    assert(proximity.actor==11 &&!proximity.nearbyResources.empty());
    sensedRequest.localProximityObserved=true;
    sensedRequest.localObserverPosition=proximity.observerPosition;
    sensedRequest.localRadiusCells=proximity.radiusCells;
    sensedRequest.localResources=proximity.nearbyResources;
    AutonomousGoalPredicate rawCount;
    rawCount.metric=AutonomousGoalMetric::ActorRawMaterialCount;
    rawCount.material=sensedRequest.localResources[0].material;
    rawCount.comparison=AutonomousGoalCompare::AtLeast;
    rawCount.threshold=1;
    auto sensedGoal=goalFor(sensedRequest);
    sensedGoal.desiredState={rawCount};
    sensedGoal.evidence={{AutonomousGoalEvidenceKind::ObservedLocalResource,0}};
    const auto sensedGather=draftAutonomousAffordancePlan(
        world,relationships,sensedRequest,sensedGoal);
    assert(sensedGather.status==AutonomousAffordancePlanStatus::CandidateOnly);
    assert(sensedGather.proposedSteps.size()==1);
    assert(sensedGather.proposedSteps[0].kind
        ==AutonomousAffordanceKind::ExistingCivilizationGather);
    assert(sensedGather.proposedSteps[0].material==rawCount.material);
    assert(sensedGather.proposedSteps[0].resourceNode
        ==sensedRequest.localResources[0].nodeId);
    assert(sensedGather.contextFingerprint
        ==cognitiveRequestFingerprint(sensedRequest));
    const auto noSense=currentRequest(world,relationships,11);
    assert(draftAutonomousAffordancePlan(
        world,relationships,noSense,sensedGoal).status
        ==AutonomousAffordancePlanStatus::RejectedGoal);
    auto forgedPerception=sensedRequest;
    forgedPerception.localResources[0].nodeId=999999;
    assert(draftAutonomousAffordancePlan(
        world,relationships,forgedPerception,sensedGoal).status
        ==AutonomousAffordancePlanStatus::RejectedGoal);
    auto laterWorld=world;
    for(auto& node:laterWorld.resourceNodes)
        if(node.id==sensedRequest.localResources[0].nodeId)
            node.quantity=0;
    assert(draftAutonomousAffordancePlan(
        laterWorld,relationships,sensedRequest,sensedGoal).status
        ==AutonomousAffordancePlanStatus::RejectedGoal);
    // A strong survival emergency outranks speculative material pursuits.
    auto emergencyWorld=world;
    emergencyWorld.characters.front().needs.hunger=0.99;
    auto emergencyRequest=currentRequest(emergencyWorld,relationships,11);
    emergencyRequest.localProximityObserved=true;
    emergencyRequest.localObserverPosition=proximity.observerPosition;
    emergencyRequest.localRadiusCells=proximity.radiusCells;
    emergencyRequest.localResources=proximity.nearbyResources;
    assert(draftAutonomousAffordancePlan(
        emergencyWorld,relationships,emergencyRequest,sensedGoal).status
        ==AutonomousAffordancePlanStatus::Blocked);
    world.characters[0].needs.hunger=0.99;
    // The earlier nonurgent material fixture intentionally changed the
    // actor's hunger; rebuild this actual Core request before validating
    // unrelated self-harm / no-op goal routes.
    request=currentRequest(world,relationships,11);

    // Never implement a physiological self-harm increase as a normal
    // goal-to-action route, despite it being a well-typed B3 predicate.
    auto harmful=foodGoal;
    harmful.desiredState[0].comparison=AutonomousGoalCompare::AtLeast;
    harmful.desiredState[0].threshold=1.0;
    unavailable=draftAutonomousAffordancePlan(
        world,relationships,request,harmful);
    assert(unavailable.status==AutonomousAffordancePlanStatus::Unsupported);
    assert(unavailable.reason==AutonomousAffordanceReason::UnsupportedDirection);

    // No-op goals have no claimed action or effects.
    auto satisfied=goalFor(request);
    satisfied.desiredState[0].threshold=0.95;
    const auto already=draftAutonomousAffordancePlan(
        world,relationships,request,satisfied);
    assert(already.status==AutonomousAffordancePlanStatus::AlreadySatisfied);
    assert(already.proposedSteps.empty());

    const auto outOfContext=draftAutonomousAffordancePlan(
        world,relationships,currentRequest(world,relationships,22),foodGoal);
    assert(outOfContext.status==AutonomousAffordancePlanStatus::RejectedGoal);
    assert(outOfContext.proposedSteps.empty());

    assert(world.rng==rngBefore);
    assert(world.minute==minuteBefore);
    assert(world.characters[0].needs.sleep==sleepBefore);
    assert(relationships.find(11,22)->trust==0.25);
    return 0;
}
