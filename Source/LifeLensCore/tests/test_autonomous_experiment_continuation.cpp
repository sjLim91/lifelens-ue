#include <cassert>
#include "lifelens/AutonomousExperimentContinuation.h"

using namespace lifelens;

static CognitiveRequest observe(const World& world,
    const RelationshipBook& relations,CharacterId id)
{
    const Character* actor=cognitiveCharacter(world,id);
    assert(actor);
    return buildCognitiveRequest(*actor,relations,world.minute,
        CognitiveTriggerKind::Reflection,cognitiveStrategicIntentCatalog(),
        DefaultCognitiveMemoryContextLimit,
        DefaultCognitiveBeliefContextLimit,
        DefaultCognitiveRelationshipContextLimit);
}

static AutonomousGoalCandidate trustGoal(
    const CognitiveRequest& request,CharacterId partner)
{
    AutonomousGoalCandidate goal;
    goal.actor=request.actor;
    goal.createdMinute=request.minute;
    goal.expiresMinute=request.minute+1440;
    goal.priority=0.7;
    goal.evidence={{AutonomousGoalEvidenceKind::Related,0}};
    AutonomousGoalPredicate predicate;
    predicate.metric=AutonomousGoalMetric::RelationshipTrust;
    predicate.targetResident=partner;
    predicate.comparison=AutonomousGoalCompare::AtLeast;
    predicate.threshold=0.85;
    goal.desiredState={predicate};
    return goal;
}

int main()
{
    World world(20319);
    world.minute=480;
    world.characters.clear();
    Character actor;
    actor.id=11;
    actor.needs={0.12,0.12,0.12,0.12,0.12};
    actor.personality.sociability=0.9;
    Character partner;
    partner.id=22;
    world.characters={actor,partner};
    RelationshipBook relations;
    auto& rel=relations.getOrCreate(11,22);
    rel.trust=0.40;
    rel.affection=0.75;
    rel.comfort=0.70;
    rel.conflict=0.25;
    const auto initialRng=world.rng;
    const auto issuedRequest=observe(world,relations,11);
    const auto goal=trustGoal(issuedRequest,22);
    const auto issued=composeAutonomousExperimentOptions(
        world,relations,issuedRequest,goal);
    assert(issued.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(issued.alternatives.size()==2);
    const auto selected=issued.alternatives.front();

    auto review=reviewAutonomousExperimentContinuation(
        world,relations,issuedRequest,goal,issued,selected);
    assert(review.status==AutonomousExperimentContinuationStatus
        ::PreviousAlternativeStillPreferred);
    assert(review.currentBest
        &&identicalAutonomousExperimentSteps(
            review.currentBest->steps,selected.steps));

    // A changed Core minute requires a newly observed resident context,
    // but is NOT license to reuse old risk/utility or to execute a step.
    auto advanced=world;
    advanced.minute+=15;
    const auto fresh=observe(advanced,relations,11);
    review=reviewAutonomousExperimentContinuation(
        advanced,relations,fresh,goal,issued,selected);
    assert(review.status==AutonomousExperimentContinuationStatus
        ::PreviousAlternativeStillPreferred);
    assert(review.currentBest.has_value());

    auto altered=selected;
    ++altered.effortUnits;
    review=reviewAutonomousExperimentContinuation(
        world,relations,issuedRequest,goal,issued,altered);
    assert(review.status==AutonomousExperimentContinuationStatus
        ::InvalidPreviousProposal);
    assert(!review.currentBest);

    // A prior Repair alternative is no longer allowed under a tighter Core
    // budget: deterministically select the still-valid Approach method.
    auto repair=issued.alternatives.front();
    for(const auto& alt:issued.alternatives)
        if(alt.steps.front().socialIntent==SocialIntent::Repair) repair=alt;
    assert(repair.steps.front().socialIntent==SocialIntent::Repair);
    AutonomousExperimentBudget constrained;
    constrained.maxEffortUnits=1;
    review=reviewAutonomousExperimentContinuation(
        world,relations,issuedRequest,goal,issued,repair,constrained);
    assert(review.status==AutonomousExperimentContinuationStatus
        ::ReplanToFreshAlternative);
    assert(review.currentBest &&review.currentBest->steps.size()==1);
    assert(review.currentBest->steps.front().socialIntent
        ==SocialIntent::Approach);

    // Genuine body needs override optional social experiment planning.
    auto exhausted=advanced;
    exhausted.characters.front().needs.sleep=0.60;
    const auto exhaustedRequest=observe(exhausted,relations,11);
    review=reviewAutonomousExperimentContinuation(
        exhausted,relations,exhaustedRequest,goal,issued,selected);
    assert(review.status==AutonomousExperimentContinuationStatus
        ::NoSafeAlternative);
    assert(!review.currentBest);

    // Core goal truth, not an accepted action, terminates a completed goal.
    auto attained=advanced;
    auto attainedRelations=relations;
    attainedRelations.getOrCreate(11,22).trust=0.91;
    const auto attainedRequest=observe(attained,attainedRelations,11);
    review=reviewAutonomousExperimentContinuation(
        attained,attainedRelations,attainedRequest,goal,issued,selected);
    assert(review.status==AutonomousExperimentContinuationStatus
        ::GoalAlreadySatisfied);
    assert(!review.currentBest);

    auto forgedRequest=fresh;
    forgedRequest.actor=33;
    review=reviewAutonomousExperimentContinuation(
        advanced,relations,forgedRequest,goal,issued,selected);
    assert(review.status==AutonomousExperimentContinuationStatus
        ::InvalidCurrentContext);
    auto expired=world;
    expired.minute=goal.expiresMinute+1;
    review=reviewAutonomousExperimentContinuation(
        expired,relations,observe(expired,relations,11),
        goal,issued,selected);
    assert(review.status==AutonomousExperimentContinuationStatus
        ::DeadlineExpired);
    assert(!review.currentBest);

    assert(world.minute==480 &&world.rng==initialRng);
    assert(world.characters.front().memory.entries.empty());
    assert(relations.find(11,22)->trust==0.40);
}
