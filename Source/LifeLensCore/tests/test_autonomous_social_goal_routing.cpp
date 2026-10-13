#include <cassert>
#include <cmath>

#include "lifelens/AutonomousSocialGoalRouting.h"

using namespace lifelens;

int main()
{
    World world(99012);
    world.minute=450;
    world.characters.clear();

    Character actor;
    actor.id=11;
    actor.personality.sociability=1.0;
    actor.personality.empathy=1.0;
    actor.personality.openness=1.0;
    actor.personality.introversion=0.0;
    actor.needs.hunger=0.2;
    actor.needs.thirst=0.2;
    actor.needs.sleep=0.2;
    actor.needs.bladder=0.2;
    actor.needs.hygiene=0.2;

    Character acquaintance;
    acquaintance.id=22;
    Character originalTarget;
    originalTarget.id=33;
    world.characters={actor,acquaintance,originalTarget};

    RelationshipBook relationships;
    auto& newFriend=relationships.getOrCreate(11,22);
    newFriend.trust=0.10;
    newFriend.affection=0.75;
    newFriend.comfort=0.65;
    newFriend.respect=0.45;
    auto& oldFriend=relationships.getOrCreate(11,33);
    oldFriend.trust=0.75;

    const auto originalRng=world.rng;
    const auto originalMinute=world.minute;
    const auto oldTrust=newFriend.trust;

    // Fallback is a real pre-selected Core social opportunity. The generated
    // personal aspiration may re-route to another physically meaningful
    // resident, but cannot create an extra activity slot.
    const double personalScore=scoreApproachIntent(
        world,world.characters[0],22,relationships);
    assert(personalScore>=0.18 && personalScore<=0.95);
    const SocialUtilityDecision baseline={
        SocialIntent::Approach,33,personalScore+0.03
    };
    const auto chosen=preferAutonomousSocialGoalTarget(
        world,relationships,world.characters[0],baseline);
    assert(chosen.has_value());
    assert(chosen->intent==SocialIntent::Approach
        || chosen->intent==SocialIntent::Repair);
    assert(chosen->target==22);
    assert(std::abs(chosen->utility-personalScore)<1e-12);

    const auto replay=preferAutonomousSocialGoalTarget(
        world,relationships,world.characters[0],baseline);
    assert(replay.has_value() && replay->target==chosen->target);
    assert(replay->intent==chosen->intent);
    assert(replay->utility==chosen->utility);

    // B5b existing social slot: a risk-averse resident may try Approach but
    // never Repair when its hypothetical budget only admits one effort unit
    // and a risk proxy of 0.08. This is method selection, not a new slot.
    newFriend.conflict=0.25;
    const auto beforeCompetingTrust=newFriend.trust;
    const auto beforeCompetingRng=world.rng;
    // Mutate the Core-owned resident, not a caller-invented copy: B4a
    // correctly rejects cognitive requests inconsistent with live World.
    const double priorRiskTolerance=world.characters[0].personality.riskTolerance;
    const double priorPatience=world.characters[0].personality.patience;
    world.characters[0].personality.riskTolerance=0.0;
    world.characters[0].personality.patience=0.0;
    const auto cautiousRoute=preferAutonomousSocialGoalTargetWithGoal(
        world,relationships,world.characters[0],baseline);
    if(cautiousRoute){
        assert(cautiousRoute->decision.intent==SocialIntent::Approach);
        assert(cautiousRoute->decision.target==22);
        assert(cautiousRoute->contextFingerprint!=0);
        assert(cautiousRoute->goal.actor==11);
    }
    world.characters[0].personality.riskTolerance=1.0;
    world.characters[0].personality.patience=1.0;
    const auto permissiveRoute=preferAutonomousSocialGoalTargetWithGoal(
        world,relationships,world.characters[0],baseline);
    if(permissiveRoute){
        assert(permissiveRoute->decision.intent==SocialIntent::Approach
            ||permissiveRoute->decision.intent==SocialIntent::Repair);
        assert(permissiveRoute->decision.target==22);
    }
    assert(world.rng==beforeCompetingRng);
    assert(relationships.find(11,22)->trust==beforeCompetingTrust);
    newFriend.conflict=0.0;
    world.characters[0].personality.riskTolerance=priorRiskTolerance;
    world.characters[0].personality.patience=priorPatience;

    // A much stronger original social preference cannot be overridden
    // by cognition; only close feasible alternatives are affected.
    const SocialUtilityDecision strong={
        SocialIntent::Approach,33,std::min(1.0,personalScore+0.30)
    };
    assert(!preferAutonomousSocialGoalTarget(
        world,relationships,world.characters[0],strong));

    // The normal unified utility gate must already have selected Social.
    // A non-social/invalid opportunity yields no permission to act.
    assert(!preferAutonomousSocialGoalTarget(
        world,relationships,world.characters[0],
        {SocialIntent::None,0,0.0}));
    assert(!preferAutonomousSocialGoalTarget(
        world,relationships,world.characters[0],
        {SocialIntent::Avoid,33,personalScore+0.03}));
    assert(!preferAutonomousSocialGoalTarget(
        world,relationships,world.characters[0],
        {SocialIntent::Comfort,33,personalScore+0.03}));

    world.characters[0].needs.hunger=0.99;
    assert(!preferAutonomousSocialGoalTarget(
        world,relationships,world.characters[0],baseline));
    world.characters[0].needs.hunger=0.2;
    world.characters[0].needs.sleep=0.99;
    assert(!preferAutonomousSocialGoalTarget(
        world,relationships,world.characters[0],baseline));
    world.characters[0].needs.sleep=0.2;

    // Only existing Core planning cadence can change the partner.
    world.minute=451;
    assert(!preferAutonomousSocialGoalTarget(
        world,relationships,world.characters[0],baseline));
    world.minute=450;

    // Ghost/dead resident is never converted into a social action.
    world.characters[1].alive=false;
    assert(!preferAutonomousSocialGoalTarget(
        world,relationships,world.characters[0],baseline));
    world.characters[1].alive=true;

    // Missing actual relationship provenance cannot invent a new target.
    RelationshipBook noRelationship;
    assert(!preferAutonomousSocialGoalTarget(
        world,noRelationship,world.characters[0],baseline));

    // All of these are decisions only: no simulation clock, resource, Need,
    // RNG or relationship changed by repeated goal evaluation.
    assert(world.rng==originalRng);
    assert(world.minute==originalMinute);
    assert(world.characters[0].needs.hunger==0.2);
    assert(world.characters[0].needs.sleep==0.2);
    assert(relationships.find(11,22)->trust==oldTrust);
    return 0;
}
