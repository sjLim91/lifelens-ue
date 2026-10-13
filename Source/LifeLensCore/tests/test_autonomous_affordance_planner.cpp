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

    const auto replay=draftAutonomousAffordancePlan(
        world,relationships,request,goal);
    assert(replay.status==plan.status);
    assert(replay.contextFingerprint==plan.contextFingerprint);
    assert(replay.proposedSteps.size()==plan.proposedSteps.size());
    assert(replay.proposedSteps[0].physicalGoal==
           plan.proposedSteps[0].physicalGoal);

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
