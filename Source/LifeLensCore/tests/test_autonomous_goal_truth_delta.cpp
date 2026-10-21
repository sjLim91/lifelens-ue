#include <cassert>
#include <cmath>
#include <limits>

#include "lifelens/AutonomousGoalTruthDelta.h"

using namespace lifelens;
using Status=AutonomousGoalTruthDeltaStatus;

static AutonomousGoalCandidate singleGoal(
    AutonomousGoalMetric metric,double threshold,
    AutonomousGoalCompare comparison=AutonomousGoalCompare::AtMost)
{
    AutonomousGoalCandidate goal;
    goal.actor=11;
    goal.createdMinute=450;
    goal.expiresMinute=900;
    AutonomousGoalPredicate predicate;
    predicate.metric=metric;
    predicate.comparison=comparison;
    predicate.threshold=threshold;
    goal.desiredState.push_back(predicate);
    return goal;
}

int main()
{
    World before(9001);
    before.minute=450;
    before.characters.clear();
    Character actor;
    actor.id=11;
    actor.needs.hunger=0.70;
    actor.needs.thirst=0.68;
    actor.needs.sleep=0.30;
    actor.civilization.character=11;
    Character neighbor;
    neighbor.id=22;
    neighbor.civilization.character=22;
    before.characters={actor,neighbor};
    RelationshipBook beforeRelations;
    beforeRelations.getOrCreate(11,22).trust=0.50;

    const auto originalRng=before.rng;
    auto observed=before;
    observed.minute=480;
    auto observedRelations=beforeRelations;

    // Eating/drinking/sleeping are typed goals, not action-success flags.
    auto hunger=singleGoal(AutonomousGoalMetric::ActorNeed,0.30);
    hunger.desiredState[0].need=AutonomousNeedKind::Hunger;
    observed.characters[0].needs.hunger=0.28;
    const auto reached=compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,observedRelations,hunger);
    assert(reached.status==Status::ReachedSinceBaseline);
    assert(!reached.satisfiedBefore &&reached.satisfiedAfter);
    assert(reached.gapBefore>0 &&reached.gapAfter==0);
    assert(reached.predicates.size()==1);
    assert(std::abs(reached.predicates[0].before-0.70)<1e-12);

    observed.characters[0].needs.hunger=0.50;
    const auto partial=compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,observedRelations,hunger);
    assert(partial.status==Status::ProgressButUnmet);
    assert(!partial.satisfiedAfter &&partial.gapAfter<partial.gapBefore);

    observed.characters[0].needs.hunger=0.90;
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,observedRelations,hunger)
        .status==Status::Regressed);
    observed.characters[0].needs.hunger=0.70;
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,observedRelations,hunger)
        .status==Status::Stalled);

    auto sleep=singleGoal(AutonomousGoalMetric::ActorNeed,0.35);
    sleep.desiredState[0].need=AutonomousNeedKind::Sleep;
    observed.characters[0].needs.sleep=0.20;
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,observedRelations,sleep)
        .status==Status::AlreadySatisfied);
    observed.characters[0].needs.sleep=0.66;
    // If the goal WAS satisfied but is no longer satisfied, that's a
    // regression, not an "already achieved" excuse.
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,observedRelations,sleep)
        .status==Status::Regressed);

    // A dead actor cannot have completed a living resident's plan merely
    // because its last need/inventory values happen to cross a threshold.
    auto deceasedActor=before;
    deceasedActor.minute=480;
    deceasedActor.characters.front().alive=false;
    deceasedActor.characters.front().needs.hunger=0.05;
    const auto afterDeathWithoutExecution=compareAutonomousGoalCoreTruth(
        before,beforeRelations,deceasedActor,beforeRelations,hunger);
    assert(afterDeathWithoutExecution.status==Status::Unverifiable);
    assert(!afterDeathWithoutExecution.satisfiedAfter);
    assert(afterDeathWithoutExecution.predicates.empty());
    auto deceasedBaseline=before;
    deceasedBaseline.characters.front().alive=false;
    assert(compareAutonomousGoalCoreTruth(
        deceasedBaseline,beforeRelations,before,beforeRelations,hunger)
        .status==Status::Unverifiable);

    // Inventories are read, never awarded, by the evaluator.
    before.characters[0].civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Wood,2,0.5,1.0});
    observed=before;
    observed.minute=480;
    auto wood=singleGoal(
        AutonomousGoalMetric::ActorRawMaterialCount,5,
        AutonomousGoalCompare::AtLeast);
    wood.desiredState[0].material=MaterialKind::Wood;
    observed.characters[0].civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Wood,2,0.5,1.0});
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,observedRelations,wood)
        .status==Status::ProgressButUnmet);
    observed.characters[0].civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Wood,1,0.5,1.0});
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,observedRelations,wood)
        .status==Status::ReachedSinceBaseline);
    assert(before.characters[0].civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood)==2);

    // Social trust is strictly directional (11->22, not the reverse).
    auto trust=singleGoal(
        AutonomousGoalMetric::RelationshipTrust,0.75,
        AutonomousGoalCompare::AtLeast);
    trust.desiredState[0].targetResident=22;
    observedRelations.getOrCreate(22,11).trust=0.99;
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,observedRelations,trust)
        .status==Status::Stalled);
    observedRelations.getOrCreate(11,22).trust=0.65;
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,observedRelations,trust)
        .status==Status::ProgressButUnmet);
    observedRelations.getOrCreate(11,22).trust=0.77;
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,observedRelations,trust)
        .status==Status::ReachedSinceBaseline);

    // Conjunction: improvement in thirst must not mask a social regression.
    auto mixed=singleGoal(AutonomousGoalMetric::ActorNeed,0.30);
    mixed.desiredState[0].need=AutonomousNeedKind::Thirst;
    mixed.desiredState.push_back(trust.desiredState[0]);
    observed=before;
    observed.minute=480;
    observed.characters[0].needs.thirst=0.40;
    observedRelations=beforeRelations;
    observedRelations.getOrCreate(11,22).trust=0.20;
    const auto mixture=compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,observedRelations,mixed);
    assert(mixture.status==Status::MixedChanges);
    assert(!mixture.satisfiedAfter);
    assert(mixture.predicates.size()==2);

    // Life goals can truly regress: alive -> dead is NOT AlreadySatisfied.
    auto alive=singleGoal(
        AutonomousGoalMetric::KnownResidentAlive,1.0,
        AutonomousGoalCompare::AtLeast);
    alive.desiredState[0].targetResident=22;
    observed=before;
    observed.minute=480;
    observed.characters[1].alive=false;
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,beforeRelations,alive)
        .status==Status::Regressed);
    // Trust is unverifiable for a deceased counterpart: don't claim progress.
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,beforeRelations,trust)
        .status==Status::Unverifiable);

    // Deadline and identity are part of the task contract, not afterthoughts.
    observed=before;
    observed.minute=901;
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,beforeRelations,hunger)
        .status==Status::Expired);
    observed.minute=449;
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,beforeRelations,hunger)
        .status==Status::InvalidContext);
    observed=before;
    observed.seed=9002;
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,beforeRelations,hunger)
        .status==Status::InvalidContext);
    observed=before;
    observed.populationSeed++;
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,beforeRelations,hunger)
        .status==Status::InvalidContext);
    observed=before;
    hunger.desiredState[0].threshold=
        std::numeric_limits<double>::quiet_NaN();
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,observed,beforeRelations,hunger)
        .status==Status::InvalidContext);

    // Distinct region identity with the same world seed is not one
    // continuous local history; duplicated goal predicates are invalid.
    auto regionSwapped=before;
    regionSwapped.hasInitialStartRegionSelection=true;
    regionSwapped.initialStartRegionCoord.x=11;
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,regionSwapped,beforeRelations,trust)
        .status==Status::InvalidContext);
    auto duplicate=trust;
    duplicate.desiredState.push_back(trust.desiredState.front());
    assert(compareAutonomousGoalCoreTruth(
        before,beforeRelations,before,beforeRelations,duplicate)
        .status==Status::InvalidContext);

    // A nonexistent record is not evidence of no trust.
    RelationshipBook absentRelations;
    assert(compareAutonomousGoalCoreTruth(
        before,absentRelations,observed,beforeRelations,trust)
        .status==Status::Unverifiable);

    // Read-only: never touches the RNG, residents, needs, stock or relations.
    assert(before.rng==originalRng);
    assert(before.characters[0].needs.hunger==0.70);
    assert(before.characters[0].civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood)==2);
    assert(beforeRelations.find(11,22)->trust==0.50);
    return 0;
}
