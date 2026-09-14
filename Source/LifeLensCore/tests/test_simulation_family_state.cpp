#include <cassert>

#include "lifelens/Simulation.h"

using namespace lifelens;

int main()
{
    Simulation sim(42);
    sim.setupSocialDemo();

    const int minute = sim.world().minute;
    assert(sim.romances().startDating(1, 2, 1, minute));
    assert(sim.households().formSharedHousehold(1, 1, 2));
    assert(sim.pregnancies().start(1, 2, minute) != nullptr);

    const FamilyObservation first = sim.observeFamily(1);
    assert(first.subjectId == 1);
    assert(first.hasRomanceHistory);
    assert(first.hasActivePartner);
    assert(first.partnerId == 2);
    assert(first.partnerName == "SocialB");
    assert(first.partnerStage == RomanceStage::Dating);
    assert(first.householdId == 1);
    assert(first.cohabitingWithPartner);
    assert(first.isGestationalParent);
    assert(first.expectingChild);
    assert(first.pregnancyPartnerId == 2);
    assert(first.pregnancyPartnerName == "SocialB");

    const FamilyObservation second = sim.observeFamily(2);
    assert(second.subjectId == 2);
    assert(second.hasActivePartner);
    assert(second.partnerId == 1);
    assert(second.householdId == 1);
    assert(second.cohabitingWithPartner);
    assert(!second.isGestationalParent);
    assert(second.expectingChild);
    assert(second.pregnancyPartnerId == 1);

    const WorldOverviewObservation world = sim.observeWorldOverview();
    assert(world.totalResidents == 2);
    assert(world.livingResidents == 2);
    assert(world.households == 1);
    assert(world.activeCouples == 1);
    assert(world.datingCouples == 1);
    assert(world.activePregnancies == 1);

    // Restarting a demo must reset all authoritative family books instead of
    // leaking state from a previous world/session.
    sim.setupSocialDemo();
    assert(sim.romances().all().empty());
    assert(sim.households().all().empty());
    assert(sim.pregnancies().all().empty());
    assert(sim.observeWorldOverview().activeCouples == 0);
    assert(sim.observeWorldOverview().activePregnancies == 0);

    return 0;
}
