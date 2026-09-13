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

    const auto before = sim.observeAllResidents();
    assert(before.size() == 2);
    assert(before[0].id == 1);
    assert(before[0].name == "SocialA");
    assert(before[0].activityKind == ObservedActivityKind::Idle);
    assert(before[0].activityLabel == "Idle");
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
        if (resident.activityKind == ObservedActivityKind::Social) {
            sawSocial = true;
            assert(resident.activityLabel == "Approach" ||
                   resident.activityLabel == "Comfort" ||
                   resident.activityLabel == "Repair" ||
                   resident.activityLabel == "Avoid");
            assert(resident.activityTargetId != 0);
            assert(!resident.activityTargetName.empty());
        }
    }
    assert(sawSocial);

    const ResidentObservation a = sim.observeResident(1);
    assert(a.id == 1);
    assert(near(a.needs.hunger, sim.world().characters[0].needs.hunger));
    assert(near(a.emotionValence, sim.world().characters[0].emotion.valence));
    assert(a.relationships.size() == 1);

    // Social action uses a five-tick duration and must return to non-social state.
    sim.runMinutes(4);
    const auto afterDuration = sim.observeAllResidents();
    for (const auto& resident : afterDuration) {
        assert(resident.activityKind != ObservedActivityKind::Social);
    }

    const ResidentObservation missing = sim.observeResident(999999);
    assert(missing.id == 0);
    assert(missing.name.empty());

    return 0;
}
