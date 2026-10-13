#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

#include "lifelens/AutonomousExperimentPlanner.h"

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
