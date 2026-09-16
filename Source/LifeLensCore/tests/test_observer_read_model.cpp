#include <cassert>
#include <cmath>
#include <string>

#include "lifelens/Simulation.h"

using namespace lifelens;

static bool near(double a, double b, double eps = 1e-9)
{
    return std::fabs(a - b) <= eps;
}

int main()
{
    Simulation sim(9001);
    sim.setupSocialDemo();
    sim.setExternalPhysicalExecution(true);

    const auto before = sim.observeAllResidents();
    assert(before.size() == 2);
    assert(before[0].id == 1);
    assert(before[0].name == "SocialA");
    assert(before[0].activityKind == ObservedActivityKind::Idle);
    assert(before[0].activityLabel == "Idle");
    assert(before[0].physicalGoal == Goal::Idle);
    assert(before[0].socialIntent == SocialIntent::None);
    assert(before[0].relationships.size() == 1);
    assert(before[0].relationships[0].targetId == 2);
    assert(before[0].relationships[0].targetName == "SocialB");
    assert(near(before[0].relationships[0].affection, 0.84));
    assert(near(before[0].relationships[0].trust, 0.82));
    assert(near(before[0].relationships[0].familiarity, 0.88));
    assert(before[0].relationships[0].socialBond > 0.0);

    sim.step();

    const auto active = sim.observeAllResidents();
    assert(active.size() == 2);

    bool sawSocial = false;
    for (const auto& resident : active) {
        if (resident.activityKind != ObservedActivityKind::Social) continue;

        sawSocial = true;
        assert(resident.socialIntent == SocialIntent::Approach ||
               resident.socialIntent == SocialIntent::Comfort ||
               resident.socialIntent == SocialIntent::Repair ||
               resident.socialIntent == SocialIntent::Avoid);
        assert(resident.physicalGoal == Goal::Idle);
        assert(resident.activityTargetId != 0);
        assert(!resident.activityTargetName.empty());

        const PendingContextActionObservation pending =
            sim.observePendingContextAction(resident.id);
        assert(pending.active);
        assert(pending.kind == ContextActionKind::Social);
        assert(pending.token != 0);
        assert(pending.targetResident == resident.activityTargetId);

        GridPos targetPos{};
        assert(sim.runtimePosition(pending.targetResident, targetPos));
        GridPos resolved = targetPos;
        if (pending.socialIntent == SocialIntent::Avoid) {
            resolved.x += 1;
        }
        assert(sim.completeExternalContextAction(
            pending.actor, pending.token, resolved));
    }
    assert(sawSocial);

    const ResidentObservation a = sim.observeResident(1);
    assert(a.id == 1);
    assert(near(a.needs.hunger, sim.world().characters[0].needs.hunger));
    assert(near(a.emotionValence, sim.world().characters[0].emotion.valence));
    assert(a.relationships.size() == 1);

    // The read model must expose typed physical/social authority rather than
    // forcing downstream runtimes to parse display labels.
    World directWorld(7);
    Character first;
    first.id = 10;
    first.name = "First";
    Character second;
    second.id = 11;
    second.name = "Second";
    directWorld.characters = {first, second};
    RelationshipBook directRelationships;

    const ResidentObservation physical = buildResidentObservation(
        directWorld, directRelationships, directWorld.characters[0],
        true, Goal::Eat, false, SocialIntent::None, 0);
    assert(physical.activityKind == ObservedActivityKind::Physical);
    assert(physical.physicalGoal == Goal::Eat);
    assert(physical.socialIntent == SocialIntent::None);

    const ResidentObservation social = buildResidentObservation(
        directWorld, directRelationships, directWorld.characters[0],
        false, Goal::Idle, true, SocialIntent::Comfort, 11);
    assert(social.activityKind == ObservedActivityKind::Social);
    assert(social.physicalGoal == Goal::Idle);
    assert(social.socialIntent == SocialIntent::Comfort);
    assert(social.activityTargetId == 11);
    assert(social.activityTargetName == "Second");

    // Once the pending interaction is acknowledged, the authoritative read
    // model must no longer report a social action for that resident.
    const auto afterCompletion = sim.observeAllResidents();
    for (const auto& resident : afterCompletion) {
        assert(resident.activityKind != ObservedActivityKind::Social);
    }

    const ResidentObservation missing = sim.observeResident(999999);
    assert(missing.id == 0);
    assert(missing.name.empty());

    return 0;
}
