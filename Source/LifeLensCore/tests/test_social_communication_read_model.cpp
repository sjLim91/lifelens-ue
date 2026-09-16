#include <iostream>
#include <string>

#include "lifelens/Simulation.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if (!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while (false)

int main()
{
    Simulation sim(90901);
    sim.setupSocialDemo();
    sim.setExternalPhysicalExecution(true);

    // Force a clean positive-approach situation so the first completed social
    // action deterministically generates a real SocialEvent.
    for (auto& character : sim.world().characters) {
        character.needs = {0.05, 0.05, 0.05, 0.05, 0.05};
        character.emotion = EmotionState{};
    }
    for (const auto& from : sim.world().characters) {
        for (const auto& to : sim.world().characters) {
            if (from.id == to.id) continue;
            Relationship& relationship = sim.relationships().getOrCreate(from.id, to.id);
            relationship.affection = 0.80;
            relationship.trust = 0.80;
            relationship.comfort = 0.80;
            relationship.familiarity = 0.90;
            relationship.conflict = 0.0;
            relationship.fear = 0.0;
            relationship.grudge = 0.0;
        }
    }

    CHECK(sim.observeRecentSocialEvents().empty());

    PendingContextActionObservation pending;
    for (int minute = 0; minute < 8 && !pending.active; ++minute) {
        sim.step();
        for (const auto& resident : sim.world().characters) {
            const PendingContextActionObservation candidate =
                sim.observePendingContextAction(resident.id);
            if (candidate.active &&
                candidate.kind == ContextActionKind::Social &&
                candidate.socialIntent != SocialIntent::Avoid) {
                pending = candidate;
                break;
            }
        }
    }

    CHECK(pending.active);
    CHECK(pending.socialIntent == SocialIntent::Approach);
    CHECK(sim.observeRecentSocialEvents().empty());

    GridPos targetPos{};
    CHECK(sim.runtimePosition(pending.targetResident, targetPos));
    CHECK(sim.completeExternalContextAction(pending.actor, pending.token, targetPos));

    const auto recent = sim.observeRecentSocialEvents();
    CHECK(recent.size() == 1);
    CHECK(recent.front().sequence == 1);
    CHECK(recent.front().actor == pending.actor);
    CHECK(recent.front().target == pending.targetResident);
    CHECK(recent.front().type == SocialEventType::PositiveInteraction);
    CHECK(recent.front().presentationLevel == SocialPresentationLevel::Everyday);
    CHECK(recent.front().intensity > 0.0);
    CHECK(recent.front().importance > 0.0);
    CHECK(recent.front().minute == sim.world().minute);
    CHECK(recent.front().successful);

    CHECK(sim.observeRecentSocialEvents(0).empty());
    CHECK(sim.observeRecentSocialEvents(1).size() == 1);

    // Recent presentation provenance is runtime-only. A restored simulation
    // must not replay speech bubbles / event-feed entries from before the save.
    const SimulationStateSnapshot snapshot = sim.captureSnapshot();
    Simulation restored(90901);
    std::string error;
    CHECK(restored.restoreSnapshot(snapshot, &error));
    CHECK(error.empty());
    CHECK(restored.observeRecentSocialEvents().empty());

    CHECK(socialPresentationLevel(SocialEventType::Comfort) ==
        SocialPresentationLevel::Meaningful);
    CHECK(socialPresentationLevel(SocialEventType::Conflict) ==
        SocialPresentationLevel::Important);
    CHECK(socialPresentationLevel(SocialEventType::Commitment) ==
        SocialPresentationLevel::Important);

    std::cout << "social communication read model passed\n";
    return 0;
}
