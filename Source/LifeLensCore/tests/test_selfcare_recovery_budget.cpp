#include <cassert>
#include <iostream>
#include <string>

#include "lifelens/Simulation.h"
#include "lifelens/Planner.h"
#include "lifelens/SettlementProgression.h"

using namespace lifelens;

namespace {

bool containsLog(const Simulation& sim,const std::string& token)
{
    for(const auto& line:sim.logs()){
        if(line.find(token)!=std::string::npos) return true;
    }
    return false;
}

}

int main()
{
    const NeedsRuleset rates=DefaultSimulationRuleset.needs;
    const double dayMinutes=24.0*60.0;

    // Primitive fallback actions must be capable of servicing one day of body
    // pressure in a human-scale number of sessions. Travel still costs time,
    // but the interaction itself may not require dozens of repetitions.
    const NeedsDelta eat=emergencyUseEffectPerTick(Goal::Eat);
    const NeedsDelta drink=emergencyUseEffectPerTick(Goal::Drink);
    const NeedsDelta toilet=emergencyUseEffectPerTick(Goal::UseToilet);
    const NeedsDelta wash=emergencyUseEffectPerTick(Goal::Wash);

    assert((-eat.hunger)*4.0 >= rates.hungerPerMinute*dayMinutes);
    assert((-drink.thirst)*5.0 >= rates.thirstPerMinute*dayMinutes);
    assert((-toilet.bladder)
        *static_cast<double>(emergencyUseDurationTicks(Goal::UseToilet))
        *4.0 >= rates.bladderPerMinute*dayMinutes);

    const double toiletHygieneBurden=
        toilet.hygiene
        *static_cast<double>(emergencyUseDurationTicks(Goal::UseToilet))
        *4.0
        +0.025*4.0;
    assert((-wash.hygiene)
        *static_cast<double>(emergencyUseDurationTicks(Goal::Wash))
        *2.0
        >= rates.hygienePerMinute*dayMinutes+toiletHygieneBurden);

    ConstructedFacility shelter;
    shelter.id=1;
    shelter.kind=FacilityKind::Shelter;
    shelter.state=FacilityState::Operational;
    shelter.active=true;
    shelter.durability=1.0;

    ConstructedFacility bed=shelter;
    bed.id=2;
    bed.kind=FacilityKind::SleepingPlace;

    const double shelterRecovery=settlementSleepRecoveryPerTick(shelter);
    const double bedRecovery=settlementSleepRecoveryPerTick(bed);
    assert(shelterRecovery>rates.sleepPerMinute);
    assert(bedRecovery>shelterRecovery);

    // Production NEW GAME uses sleepRecoveryPerMinuteAt(), not merely the
    // Planner fallback delta. With only fatigue changing, an exhausted founder
    // must be able to finish a real sleep session within the configured 10-hour
    // cap rather than living forever at fatigue saturation.
    SimulationRuleset sleepRules=DefaultSimulationRuleset;
    sleepRules.needs.hungerPerMinute=0.0;
    sleepRules.needs.thirstPerMinute=0.0;
    sleepRules.needs.bladderPerMinute=0.0;
    sleepRules.needs.hygienePerMinute=0.0;

    Simulation sim(4242001,0,CurrentWorldGenerationVersion,sleepRules);
    sim.setupNewGame();
    sim.world().characters.resize(1);
    Character& resident=sim.world().characters.front();
    resident.needs={0.05,0.05,1.0,0.05,0.05};
    const std::string completion=
        resident.name+" completed Sleep via emergency fallback";

    bool completed=false;
    for(int minute=0;minute<=MaximumSleepSessionMinutes+30;++minute){
        sim.step();
        if(containsLog(sim,completion)){
            completed=true;
            break;
        }
    }

    assert(completed);
    assert(resident.needs.sleep<=RestedSleepNeedTarget+0.01);

    std::cout<<"integrated self-care recovery budget PASS\n";
    return 0;
}
