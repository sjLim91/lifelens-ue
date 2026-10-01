#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/PhysiologyBalance.h"
#include "lifelens/Planner.h"
#include "lifelens/PrimitiveSanitation.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationRuleset.h"

using namespace lifelens;

namespace {

bool containsLog(const std::vector<std::string>& logs,const std::string& token)
{
    for(const auto& line:logs){
        if(line.find(token)!=std::string::npos) return true;
    }
    return false;
}

}

int main()
{
    constexpr double MinutesPerDay=24.0*60.0;
    const NeedsRuleset& needs=DefaultSimulationRuleset.needs;
    const PhysiologyBalance& balance=DefaultPhysiologyBalance;

    const double mealsPerDay=
        needs.hungerPerMinute*MinutesPerDay
        /balance.primitiveEatHungerRelief;
    const double drinksPerDay=
        needs.thirstPerMinute*MinutesPerDay
        /balance.primitiveDrinkThirstRelief;
    const double toiletRelief=
        balance.primitiveToiletBladderReliefPerMinute
        *static_cast<double>(balance.primitiveToiletMinutes);
    const double toiletsPerDay=
        needs.bladderPerMinute*MinutesPerDay/toiletRelief;
    const double hygieneBurdenPerOutdoorToilet=
        balance.primitiveToiletHygieneBurdenPerMinute
            *static_cast<double>(balance.primitiveToiletMinutes)
        +balance.outdoorToiletCompletionHygieneBurden;
    const double primitiveWashRelief=
        balance.primitiveWashHygieneReliefPerMinute
        *static_cast<double>(balance.primitiveWashMinutes);
    const double washesPerDay=
        (
            needs.hygienePerMinute*MinutesPerDay
            +toiletsPerDay*hygieneBurdenPerOutdoorToilet
        )/primitiveWashRelief;
    const double waterUsesPerDay=drinksPerDay+washesPerDay;

    // Primitive founders should have enough day left for relationships,
    // learning and civilization instead of spending all waking time on Needs.
    assert(mealsPerDay>=3.0 && mealsPerDay<=4.5);
    assert(drinksPerDay>=3.5 && drinksPerDay<=5.0);
    assert(toiletsPerDay>=5.0 && toiletsPerDay<=7.0);
    assert(washesPerDay>=1.0 && washesPerDay<=2.5);
    assert(waterUsesPerDay<=7.0);

    // An average adult can maintain neutral-weather fatigue with roughly one
    // long primitive night's sleep. High sleep tendency remains a real reason
    // to value bedding rather than making ground sleep universally sufficient.
    const double outdoorHoursPerDay=
        needs.sleepPerMinute*MinutesPerDay
        /balance.outdoorSleepRecoveryPerMinute/60.0;
    const double highTendencySleepingPlaceHours=
        needs.sleepPerMinute*1.20*MinutesPerDay
        /balance.sleepingPlaceRecoveryBasePerMinute/60.0;
    assert(outdoorHoursPerDay>=8.0 && outdoorHoursPerDay<=9.5);
    assert(highTendencySleepingPlaceHours<=10.0);

    // Severe fatigue is no longer mathematically stuck near the urgent band
    // after the maximum primitive sleep session.
    const double severeFatigueAfterMaxOutdoorSleep=
        1.0
        +MaximumSleepSessionMinutes*needs.sleepPerMinute
        -MaximumSleepSessionMinutes*balance.outdoorSleepRecoveryPerMinute;
    assert(severeFatigueAfterMaxOutdoorSleep>RestedSleepNeedTarget);
    assert(severeFatigueAfterMaxOutdoorSleep<=0.25);

    assert(balance.shelterSleepRecoveryBasePerMinute
        >=balance.outdoorSleepRecoveryPerMinute);
    assert(balance.sleepingPlaceRecoveryBasePerMinute
        >balance.shelterSleepRecoveryBasePerMinute);
    assert(balance.smartObjectSleepRecoveryPerMinute
        >=balance.sleepingPlaceRecoveryBasePerMinute);

    const double smartWashRelief=
        -facilityUseEffectPerTick(Goal::Wash).hygiene
        *static_cast<double>(facilityUseDurationTicks(Goal::Wash));
    assert(smartWashRelief>primitiveWashRelief);

    // Headless execution must honor an actual DugPit instead of moving to the
    // site and then applying the dirtier emergency-outdoor result anyway.
    SimulationRuleset isolatedRules=DefaultSimulationRuleset;
    isolatedRules.needs.hungerPerMinute=0.0;
    isolatedRules.needs.thirstPerMinute=0.0;
    isolatedRules.needs.sleepPerMinute=0.0;
    isolatedRules.needs.bladderPerMinute=0.0;
    isolatedRules.needs.hygienePerMinute=0.0;

    Simulation simulation(
        981001,0,CurrentWorldGenerationVersion,isolatedRules);
    simulation.setupNewGame();
    simulation.world().characters.resize(1);

    Character& actor=simulation.world().characters.front();
    const CharacterId actorId=actor.id;
    actor.needs={0.01,0.01,0.01,0.95,0.20};

    GridPos actorPosition{};
    assert(simulation.runtimePosition(actorId,actorPosition));

    PrimitiveSanitationSite pit;
    pit.id=99001;
    pit.kind=PrimitiveSanitationSiteKind::DugPit;
    pit.pos=actorPosition;
    pit.establishedBy=actorId;
    pit.establishedMinute=simulation.world().minute;
    pit.active=true;
    pit.improvementWork=DugSanitationPitWorkRequired;
    pit.improvedBy=actorId;
    pit.improvedMinute=simulation.world().minute;
    assert(validPrimitiveSanitationSite(pit));
    simulation.world().primitiveSanitationSites.push_back(pit);

    const double hygieneBefore=actor.needs.hygiene;
    for(int minute=0;minute<20
        && !containsLog(
            simulation.logs(),
            actor.name+" completed UseToilet via sanitation site");
        ++minute){
        simulation.step();
    }

    assert(containsLog(
        simulation.logs(),
        actor.name+" completed UseToilet via sanitation site"));
    const PrimitiveSanitationSite* used=findPrimitiveSanitationSite(
        simulation.world().primitiveSanitationSites,pit.id);
    assert(used!=nullptr);
    assert(used->useCount==1);
    assert(actor.needs.bladder<0.80);

    const double expectedPitBurden=
        primitiveSanitationUseEffectPerTick(
            PrimitiveSanitationSiteKind::DugPit).hygiene
            *primitiveSanitationUseDurationTicks(
                PrimitiveSanitationSiteKind::DugPit)
        +primitiveSanitationHygieneBurden(
            PrimitiveSanitationSiteKind::DugPit);
    assert(expectedPitBurden<hygieneBurdenPerOutdoorToilet);
    assert(actor.needs.hygiene<hygieneBefore+0.03);

    std::cout
        <<"daily physiology budget PASS"
        <<" meals/day="<<mealsPerDay
        <<" drinks/day="<<drinksPerDay
        <<" toilets/day="<<toiletsPerDay
        <<" washes/day="<<washesPerDay
        <<" outdoor-sleep-hours/day="<<outdoorHoursPerDay
        <<"\n";
    return 0;
}
