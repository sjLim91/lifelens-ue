#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "lifelens/Health.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

namespace {

DailyHealthInputs healthyInputs()
{
    DailyHealthInputs input;
    input.geneticHealthPotential01=0.65;
    input.baselinePhysicalHealth01=0.90;
    input.hunger01=0.15;
    input.thirst01=0.15;
    input.sleep01=0.18;
    input.hygiene01=0.15;
    return input;
}

void testCleanResidentStaysHealthy()
{
    HealthState state;
    DailyHealthInputs input=healthyInputs();
    for(int day=1;day<=14;++day){
        const auto outcome=advanceHealthOneDay(
            state,input,17,101,day*24*60);
        assert(outcome.fatalCause==HealthFatalCause::None);
    }
    assert(state.pathogenLoad<0.20);
    assert(state.illnessSeverity<0.18);
    assert(healthStage(state)==HealthStage::Well);
}

void testContaminatedWaterAndSoilCanCauseIllnessAndRecovery()
{
    HealthState state;
    DailyHealthInputs dirty=healthyInputs();
    dirty.localContamination01=0.90;
    dirty.hygiene01=0.55;

    for(int day=1;day<=6;++day){
        recordContaminatedWaterExposure(
            state,0.80,day*24*60-10);
        advanceHealthOneDay(
            state,dirty,19,202,day*24*60);
    }

    assert(state.infectionEpisodes>=1);
    assert(state.illnessSeverity>=0.18);
    assert(state.lastExposureMinute>0);
    const double immunityBefore=state.immunity01;

    DailyHealthInputs clean=healthyInputs();
    bool recovered=false;
    for(int day=7;day<=80;++day){
        const auto outcome=advanceHealthOneDay(
            state,clean,19,202,day*24*60);
        recovered=recovered || outcome.recovered;
        if(recovered && state.illnessSeverity<0.08) break;
    }

    assert(recovered);
    assert(state.recoveryEpisodes>=1);
    assert(state.immunity01>immunityBefore);
    assert(state.careKnowledge01>0.0);
}

void testSanitationKnowledgeReducesContaminationPressure()
{
    HealthState withoutSanitation;
    HealthState withSanitation;
    DailyHealthInputs raw=healthyInputs();
    raw.localContamination01=0.82;
    raw.hygiene01=0.35;

    for(int day=1;day<=10;++day){
        DailyHealthInputs a=raw;
        DailyHealthInputs b=raw;
        b.sanitationKnowledge=true;
        advanceHealthOneDay(
            withoutSanitation,a,23,301,day*24*60);
        advanceHealthOneDay(
            withSanitation,b,23,302,day*24*60);
    }

    assert(withSanitation.pathogenLoad<withoutSanitation.pathogenLoad);
    assert(withSanitation.careKnowledge01>withoutSanitation.careKnowledge01);
}

void testEnvironmentalStressAndHazardProducePhysicalConsequences()
{
    HealthState exposure;
    DailyHealthInputs cold=healthyInputs();
    cold.coldStress01=1.0;
    cold.wetStress01=0.75;
    cold.thirst01=0.40;
    for(int day=1;day<=5;++day){
        advanceHealthOneDay(
            exposure,cold,29,401,day*24*60);
    }
    assert(exposure.environmentalStress01>0.45);
    assert(healthFunctionalCapacity01(exposure)<1.0);

    DailyHealthInputs hazardous=healthyInputs();
    hazardous.hazardPotential01=1.0;
    hazardous.baselinePhysicalHealth01=0.20;
    hazardous.sleep01=1.0;

    const int minute=12*24*60;
    const int dayIndex=minute/(24*60);
    std::uint64_t chosenId=0;
    for(std::uint64_t id=1;id<100000;++id){
        if(deterministicHealthRoll(
               31,id,dayIndex,0x4845414c54484143ULL)<0.009){
            chosenId=id;
            break;
        }
    }
    assert(chosenId!=0);

    HealthState injured;
    const auto outcome=advanceHealthOneDay(
        injured,hazardous,31,chosenId,minute);
    assert(outcome.accidentOccurred);
    assert(injured.accidentEpisodes==1);
    assert(injured.injurySeverity>0.0);
}


void testSevereDeprivationCanBeFatalAndDependencyAmplifiesRisk()
{
    DailyHealthInputs autonomous=healthyInputs();
    autonomous.hunger01=1.0;
    autonomous.thirst01=1.0;
    autonomous.directCareDependency01=0.0;

    DailyHealthInputs dependent=autonomous;
    dependent.directCareDependency01=1.0;

    const double autonomousChance=deprivationFatalChance(autonomous);
    const double dependentChance=deprivationFatalChance(dependent);
    assert(autonomousChance>0.0);
    assert(dependentChance>autonomousChance);

    DailyHealthInputs ordinary=healthyInputs();
    ordinary.hunger01=0.90;
    ordinary.thirst01=0.90;
    ordinary.directCareDependency01=1.0;
    assert(deprivationFatalChance(ordinary)==0.0);

    const int minute=20*24*60;
    const int dayIndex=minute/(24*60);
    std::uint64_t chosenId=0;
    for(std::uint64_t id=1;id<100000;++id){
        if(deterministicHealthRoll(
               37,id,dayIndex,0x4845414c54484450ULL)<dependentChance){
            chosenId=id;
            break;
        }
    }
    assert(chosenId!=0);

    HealthState state;
    const auto outcome=advanceHealthOneDay(
        state,dependent,37,chosenId,minute);
    assert(outcome.fatalCause==HealthFatalCause::Deprivation);
}

void testHealthSnapshotRoundTrip()
{
    Simulation simulation(41,73);
    simulation.setupNewGame();
    assert(!simulation.world().characters.empty());

    Character& resident=simulation.world().characters.front();
    resident.health.pathogenLoad=0.37;
    resident.health.illnessSeverity=0.22;
    resident.health.immunity01=0.46;
    resident.health.injurySeverity=0.11;
    resident.health.environmentalStress01=0.18;
    resident.health.careKnowledge01=0.29;
    resident.health.pendingWaterContaminationDose=0.14;
    resident.health.infectionEpisodes=2;
    resident.health.recoveryEpisodes=1;
    resident.health.accidentEpisodes=1;
    resident.health.lastExposureMinute=321;
    resident.health.lastIllnessMinute=333;
    resident.health.lastRecoveryMinute=444;
    resident.health.lastAccidentMinute=555;

    const SimulationStateSnapshot source=simulation.captureSnapshot();
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(source,bytes,&error));
    assert(error.empty());

    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(error.empty());
    assert(!decoded.world.characters.empty());

    const HealthState& health=decoded.world.characters.front().health;
    assert(std::abs(health.pathogenLoad-0.37)<1e-12);
    assert(std::abs(health.illnessSeverity-0.22)<1e-12);
    assert(std::abs(health.immunity01-0.46)<1e-12);
    assert(std::abs(health.careKnowledge01-0.29)<1e-12);
    assert(std::abs(health.pendingWaterContaminationDose-0.14)<1e-12);
    assert(health.infectionEpisodes==2);
    assert(health.recoveryEpisodes==1);
    assert(health.accidentEpisodes==1);
    assert(health.lastAccidentMinute==555);
}

} // namespace

int main()
{
    testCleanResidentStaysHealthy();
    testContaminatedWaterAndSoilCanCauseIllnessAndRecovery();
    testSanitationKnowledgeReducesContaminationPressure();
    testEnvironmentalStressAndHazardProducePhysicalConsequences();
    testSevereDeprivationCanBeFatalAndDependencyAmplifiesRisk();
    testHealthSnapshotRoundTrip();
    return 0;
}
