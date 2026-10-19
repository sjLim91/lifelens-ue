#include <cassert>
#include <cmath>

#include "lifelens/Health.h"

using namespace lifelens;

static HealthState wounded()
{
    HealthState health;
    health.injurySeverity=0.62;
    health.careKnowledge01=0.24;
    health.immunity01=0.13;
    health.accidentEpisodes=1;
    health.lastAccidentMinute=480;
    return health;
}

static double progress(
    HealthState& state,DailyHealthInputs needs,int minute=1440)
{
    needs.hazardPotential01=0.0; // No random accident in controlled fixture.
    const auto outcome=advanceHealthOneDay(
        state,needs,4242001,17,minute);
    assert(!outcome.accidentOccurred);
    assert(outcome.fatalCause==HealthFatalCause::None);
    return state.injurySeverity;
}

int main()
{
    const HealthState initial=wounded();

    DailyHealthInputs satisfied;
    satisfied.hunger01=0.0;
    satisfied.thirst01=0.0;
    satisfied.sleep01=0.0;
    satisfied.geneticHealthPotential01=0.68;
    satisfied.baselinePhysicalHealth01=0.85;

    DailyHealthInputs limited=satisfied;
    limited.hunger01=0.85;
    limited.thirst01=0.85;
    limited.sleep01=0.85;

    DailyHealthInputs unrested=satisfied;
    unrested.sleep01=0.90;

    HealthState restored=initial,stressed=initial,tired=initial;
    const double recovered=progress(restored,satisfied);
    const double slow=progress(stressed,limited);
    const double tiredResult=progress(tired,unrested);
    assert(recovered<initial.injurySeverity);
    assert(slow<initial.injurySeverity); // Healing never stops artificially.
    assert(recovered<tiredResult &&tiredResult<slow);
    assert(restored.accidentEpisodes==initial.accidentEpisodes);
    assert(stressed.accidentEpisodes==initial.accidentEpisodes);
    assert(restored.lastAccidentMinute==480);

    // Satisfied physiology preserves the established pre-change healing
    // amount exactly: no global boost to unaffected/healthy residents.
    const double resilience=healthClamp01(
        0.30+0.38*satisfied.geneticHealthPotential01
        +0.22*satisfied.baselinePhysicalHealth01
        +0.10*initial.immunity01);
    const double previousDailyRecovery=0.018+0.025*resilience
        +0.018*initial.careKnowledge01;
    assert(std::abs((initial.injurySeverity-recovered)
        -previousDailyRecovery)<1e-12);

    // Replaying the same factual Needs, injury and Core day is deterministic.
    HealthState replay=initial;
    assert(progress(replay,satisfied)==recovered);
    assert(replay.accidentEpisodes==restored.accidentEpisodes);

    // The existing care-knowledge value continues to matter, but
    // no treatment/caregiver is fabricated by the recovery calculation.
    HealthState experienced=initial;
    experienced.careKnowledge01=0.90;
    assert(progress(experienced,satisfied)<recovered);

    // Across several real days of poor sleep/food/water, otherwise identical
    // people remain injured longer than rested/nourished counterparts.
    HealthState healthy=initial,deprived=initial;
    for(int day=1;day<=7;++day){
        progress(healthy,satisfied,day*1440);
        progress(deprived,limited,day*1440);
        assert(healthy.injurySeverity<=deprived.injurySeverity);
    }
    assert(healthy.injurySeverity<deprived.injurySeverity);
    return 0;
}
