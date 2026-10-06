#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "lifelens/FamilyProgression.h"
#include "lifelens/Health.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "system_balance_audit.h"

namespace {

using namespace lifelens;

constexpr int MinutesPerDay=24*60;

struct ResidentMetrics {
    CharacterId id=0;
    std::string name;
    std::uint64_t observedMinutes=0;
    std::array<double,5> needSum{};
    std::array<double,5> needMax{};
    std::array<std::uint64_t,5> saturatedMinutes{};
    std::array<std::uint64_t,5> saturatedStreak{};
    std::array<std::uint64_t,5> longestSaturatedStreak{};

    std::uint64_t currentIdleStreak=0;
    std::uint64_t longestIdleStreak=0;

    bool sleepInteracting=false;
    double sleepSessionStartNeed=0.0;
    std::uint64_t currentSleepSessionMinutes=0;
    std::uint64_t sleepSessions=0;
    std::uint64_t sleepThirtyMinuteSessions=0;
    std::uint64_t meaningfulSleepSessions=0;
    std::uint64_t longestSleepSessionMinutes=0;
    double accumulatedSleepRecovery=0.0;

    std::uint64_t physicalMinutes=0;
    std::uint64_t socialMinutes=0;
    std::uint64_t civilizationMinutes=0;
    std::uint64_t parentingMinutes=0;
    std::uint64_t teachingMinutes=0;
    std::uint64_t tradeMinutes=0;
    std::uint64_t idleMinutes=0;
    std::array<std::uint64_t,6> physicalGoalMinutes{};

    std::uint64_t migrationCandidateHours=0;
    double maxMigrationPressure=0.0;
    double maxIllness=0.0;
    double maxInjury=0.0;
    double maxEnvironmentalStress=0.0;
};

struct SleepAuditMetrics {
    std::uint64_t travelMinutes=0, precipitationMinutes=0, severeExposureMinutes=0;
    std::uint64_t saturatedSleepMinutes=0, saturatedTravelMinutes=0, saturatedCooldownMinutes=0;
    std::array<std::uint64_t,6> saturationByGoal{};
    double exposureSum=0, precipitationSum=0, wetnessSum=0, windSum=0;
    std::uint64_t observations=0;
};
struct SleepWorldAudit {
    std::uint64_t observations=0, precipitationMinutes=0, severeExposureMinutes=0;
    std::uint64_t sleepingPlaceMinutes=0, shelterMinutes=0;
    double shelterEffectivenessSum=0, shelterDurabilitySum=0;
    std::uint64_t shelterSamples=0;
    GridPos origin{};
};

struct EventMetrics {
    std::uint64_t routeFailures=0;
    std::uint64_t timeouts=0;
    std::uint64_t preemptions=0;
    std::uint64_t sleepInterruptions=0;
    std::uint64_t socialEvents=0;
    std::uint64_t civilizationEvents=0;
    std::uint64_t tradeDepartures=0;
    std::uint64_t tradeExchanges=0;
    std::uint64_t tradeReturns=0;

    std::uint64_t deathsIllness=0;
    std::uint64_t deathsAccident=0;
    std::uint64_t deathsExposure=0;
    std::uint64_t deathsDeprivation=0;
    std::uint64_t deathsOther=0;
    std::uint64_t becameIll=0;
    std::uint64_t recoveredIllness=0;

    std::array<std::uint64_t,10> parentingActions{};
    std::array<std::uint64_t,5> physicalStarts{};
    std::array<std::uint64_t,5> physicalCompletions{};
};

std::array<double,5> needsArray(const Needs& n)
{
    return {n.hunger,n.thirst,n.sleep,n.bladder,n.hygiene};
}

std::size_t goalIndex(Goal goal)
{
    switch(goal){
        case Goal::Eat: return 0;
        case Goal::Drink: return 1;
        case Goal::Sleep: return 2;
        case Goal::UseToilet: return 3;
        case Goal::Wash: return 4;
        case Goal::Idle:
        default: return 5;
    }
}

std::vector<int> parseCheckpoints(const std::string& raw,int maxDays)
{
    std::vector<int> result;
    std::stringstream stream(raw);
    std::string token;
    while(std::getline(stream,token,',')){
        const int day=std::atoi(token.c_str());
        if(day>0 && day<=maxDays) result.push_back(day);
    }
    if(result.empty()) result.push_back(maxDays);
    std::sort(result.begin(),result.end());
    result.erase(std::unique(result.begin(),result.end()),result.end());
    if(result.back()!=maxDays) result.push_back(maxDays);
    return result;
}

int naturalUnits(const World& world,MaterialKind material)
{
    int total=0;
    for(const auto& node:world.resourceNodes){
        if(node.material==material) total+=std::max(0,node.quantity);
    }
    return total;
}

int carriedUnits(const World& world,MaterialKind material)
{
    int total=0;
    for(const auto& resident:world.characters){
        total+=resident.civilization.inventory.count(
            ItemKind::RawMaterial,material);
    }
    return total;
}

int storedUnits(const World& world,MaterialKind material)
{
    int total=0;
    for(const auto& storage:world.storageSites){
        total+=storage.inventory.count(ItemKind::RawMaterial,material);
    }
    return total;
}

int facilityCount(const World& world,FacilityKind kind)
{
    int total=0;
    for(const auto& facility:world.facilities){
        if(facility.kind==kind && facility.active) ++total;
    }
    return total;
}

const Character* findResident(const World& world,CharacterId id)
{
    for(const auto& c:world.characters){
        if(c.id==id) return &c;
    }
    return nullptr;
}

ResidentMetrics& ensureResidentMetrics(
    std::map<CharacterId,ResidentMetrics>& metrics,
    const Character& resident)
{
    auto [it,inserted]=metrics.emplace(
        resident.id,ResidentMetrics{});
    if(inserted){
        it->second.id=resident.id;
        it->second.name=resident.name;
    }
    return it->second;
}

void closeSleepSession(
    ResidentMetrics& metrics,
    const Character* resident)
{
    if(!metrics.sleepInteracting) return;
    const double currentNeed=
        resident==nullptr
            ? metrics.sleepSessionStartNeed
            : resident->needs.sleep;
    const double recovery=std::max(
        0.0,
        metrics.sleepSessionStartNeed-currentNeed);
    ++metrics.sleepSessions;
    if(metrics.currentSleepSessionMinutes>=30){
        ++metrics.sleepThirtyMinuteSessions;
    }
    if(recovery>=0.10){
        ++metrics.meaningfulSleepSessions;
    }
    metrics.accumulatedSleepRecovery+=recovery;
    metrics.sleepInteracting=false;
    metrics.currentSleepSessionMinutes=0;
}

void observeResidentMinute(
    Simulation& sim,
    Character& resident,
    ResidentMetrics& metrics)
{
    ++metrics.observedMinutes;
    const auto values=needsArray(resident.needs);
    for(std::size_t i=0;i<values.size();++i){
        metrics.needSum[i]+=values[i];
        metrics.needMax[i]=std::max(metrics.needMax[i],values[i]);
        if(values[i]>=0.999){
            ++metrics.saturatedMinutes[i];
            ++metrics.saturatedStreak[i];
            metrics.longestSaturatedStreak[i]=std::max(
                metrics.longestSaturatedStreak[i],
                metrics.saturatedStreak[i]);
        }else{
            metrics.saturatedStreak[i]=0;
        }
    }

    metrics.maxIllness=std::max(
        metrics.maxIllness,resident.health.illnessSeverity);
    metrics.maxInjury=std::max(
        metrics.maxInjury,resident.health.injurySeverity);
    metrics.maxEnvironmentalStress=std::max(
        metrics.maxEnvironmentalStress,
        resident.health.environmentalStress01);

    const ResidentPresentationObservation presentation=
        sim.observeResidentPresentation(resident.id);
    const bool sleepingInteraction=
        presentation.active
        && presentation.kind==PresentationActionKind::Physical
        && presentation.physicalGoal==Goal::Sleep
        && presentation.phase==PresentationActionPhase::Interacting;

    if(sleepingInteraction){
        if(!metrics.sleepInteracting){
            metrics.sleepInteracting=true;
            metrics.sleepSessionStartNeed=resident.needs.sleep;
            metrics.currentSleepSessionMinutes=0;
        }
        ++metrics.currentSleepSessionMinutes;
        metrics.longestSleepSessionMinutes=std::max(
            metrics.longestSleepSessionMinutes,
            metrics.currentSleepSessionMinutes);
    }else{
        closeSleepSession(metrics,&resident);
    }

    if(!presentation.active
       || presentation.phase==PresentationActionPhase::Idle){
        ++metrics.idleMinutes;
        ++metrics.currentIdleStreak;
        metrics.longestIdleStreak=std::max(
            metrics.longestIdleStreak,
            metrics.currentIdleStreak);
        return;
    }

    metrics.currentIdleStreak=0;
    switch(presentation.kind){
        case PresentationActionKind::Physical:
            ++metrics.physicalMinutes;
            ++metrics.physicalGoalMinutes[
                goalIndex(presentation.physicalGoal)];
            break;
        case PresentationActionKind::Social:
            ++metrics.socialMinutes;
            break;
        case PresentationActionKind::Civilization:
            ++metrics.civilizationMinutes;
            break;
        case PresentationActionKind::Parenting:
            ++metrics.parentingMinutes;
            break;
        case PresentationActionKind::KnowledgeTeaching:
            ++metrics.teachingMinutes;
            break;
        case PresentationActionKind::Trade:
            ++metrics.tradeMinutes;
            break;
        case PresentationActionKind::None:
        default:
            ++metrics.idleMinutes;
            break;
    }
}

void observeSleepAuditMinute(Simulation& sim,const Character& resident,
    SleepAuditMetrics& audit,std::ofstream& trace,int traceStart,int traceEnd,CharacterId traceResident)
{
    const auto plan=sim.observeSleepRuntimeDiagnostic(resident.id);
    const auto presentation=sim.observeResidentPresentation(resident.id);
    const auto weather=deriveDynamicEnvironment(sim.world().genesisIdentity(),chunkCoordForGrid(plan.position),sim.world().minute);
    const auto exposure=evaluateSleepEnvironment(weather,deriveEnvironmentalConsequences(weather),false);
    ++audit.observations;
    audit.exposureSum+=exposure.exposure01; audit.precipitationSum+=weather.precipitationIntensity01;
    audit.wetnessSum+=weather.surfaceWetness01; audit.windSum+=weather.windIntensity01;
    if(weather.precipitationIntensity01>=0.05) ++audit.precipitationMinutes;
    if(exposure.exposedEmergencyOnly) ++audit.severeExposureMinutes;
    const bool sleepMoving=presentation.active && presentation.kind==PresentationActionKind::Physical
        && presentation.physicalGoal==Goal::Sleep && presentation.phase==PresentationActionPhase::Moving;
    if(sleepMoving) ++audit.travelMinutes;
    if(resident.needs.sleep>=0.999){
        ++audit.saturationByGoal[static_cast<std::size_t>(plan.goal)];
        if(presentation.sleepContext!=SleepContext::None) ++audit.saturatedSleepMinutes;
        if(sleepMoving) ++audit.saturatedTravelMinutes;
        if(sim.world().minute<plan.penaltyUntilMinute) ++audit.saturatedCooldownMinutes;
    }
    if(trace && sim.world().minute>=traceStart && sim.world().minute<=traceEnd
       && (traceResident==0 || traceResident==resident.id)){
        const auto* facility=bestOperationalSleepFacility(sim.world(),plan.position,0);
        const auto& stats=plan.counters;
        trace<<sim.world().minute<<','<<resident.id<<','<<resident.needs.sleep<<','<<resident.needs.bladder
            <<','<<resident.needs.hygiene<<','<<resident.needs.hunger<<','<<resident.needs.thirst
            <<','<<goalName(plan.goal)<<','<<(plan.hasAction?static_cast<int>(plan.action):-1)
            <<','<<plan.penaltyUntilMinute<<','<<sleepContextName(presentation.sleepContext)
            <<','<<stats.selectedFacility<<','<<(stats.selectedFacility?(stats.selectedKind==FacilityKind::Shelter?"Shelter":"SleepingPlace"):"Ground")
            <<','<<(stats.selectedFacility?manhattan(plan.position,stats.selectedPosition):0)
            <<','<<exposure.exposure01<<','<<sleepRecoveryPerMinuteAt(sim.world(),plan.position,facility)
            <<','<<plan.remainingTicks<<','<<stats.lastReplanMinute<<','<<stats.lastSessionEnd
            <<','<<stats.lastSessionEndMinute<<','<<stats.lastFailureMinute<<','<<goalName(stats.lastFailedGoal)
            <<','<<stats.lastFailureWasRoute<<','<<plan.position.x<<','<<plan.position.y
            <<','<<plan.target.x<<','<<plan.target.y<<','<<plan.hasTarget<<','<<plan.arrived
            <<','<<resident.health.illnessSeverity<<'\n';
    }
}

void observeSleepWorldMinute(Simulation& sim,SleepWorldAudit& audit)
{
    ++audit.observations;
    const auto weather=deriveDynamicEnvironment(sim.world().genesisIdentity(),chunkCoordForGrid(audit.origin),sim.world().minute);
    const auto exposure=evaluateSleepEnvironment(weather,deriveEnvironmentalConsequences(weather),false);
    if(weather.precipitationIntensity01>=0.05) ++audit.precipitationMinutes;
    if(exposure.exposedEmergencyOnly) ++audit.severeExposureMinutes;
    for(const auto& facility:sim.world().facilities){
        if(!facilityOperationalAndActive(facility)) continue;
        if(facility.kind==FacilityKind::SleepingPlace) ++audit.sleepingPlaceMinutes;
        if(facility.kind==FacilityKind::Shelter){
            ++audit.shelterMinutes; ++audit.shelterSamples;
            audit.shelterEffectivenessSum+=facilityEffectiveness01(facility);
            audit.shelterDurabilitySum+=facility.durability;
        }
    }
}

void emitSleepAudit(Simulation& sim,std::uint64_t seed,const std::map<CharacterId,SleepAuditMetrics>& audits,
    const std::map<CharacterId,ResidentMetrics>& metrics,const SleepWorldAudit& worldAudit)
{
    for(const auto& entry:audits){
        const auto& a=entry.second; const auto stats=sim.observeSleepRuntimeDiagnostic(entry.first).counters;
        const auto& m=metrics.at(entry.first);
        const auto total=stats.recoveryMinutes[0]+stats.recoveryMinutes[1]+stats.recoveryMinutes[2];
        std::cout<<"SLEEP_DIAGNOSTIC seed="<<seed<<" id="<<entry.first
            <<" totalSleepMinutes="<<total<<" protectedSleepMinutes="<<stats.recoveryMinutes[0]
            <<" exposedSleepMinutes="<<stats.recoveryMinutes[1]<<" exposedEmergencySleepMinutes="<<stats.recoveryMinutes[2]
            <<" sleepTravelMinutes="<<a.travelMinutes<<" sleepReplanCount="<<stats.replans
            <<" failedSleepPlanCount="<<stats.failedPlansByGoal[2]<<" failedToiletPlanCount="<<stats.failedPlansByGoal[3]
            <<" shelterRejectedByDistanceCount="<<stats.shelterRejectedDistance
            <<" shelterRejectedByCapacityCount="<<stats.shelterRejectedCapacity<<" shelterRejectedByRouteCount="<<stats.shelterRejectedRoute
            <<" sleepSessions="<<stats.sessionStarts<<" sleepSessionsEndedBeforeRestedTarget="<<(stats.sessionClosed-stats.sessionEnds[0])
            <<" incompleteClosedSessions="<<(stats.sessionClosed-stats.sessionEnds[0])
            <<" endedRested="<<stats.sessionEnds[0]<<" endedUrgent="<<stats.sessionEnds[1]<<" endedBudget="<<stats.sessionEnds[2]
            <<" endedReplan="<<stats.sessionEnds[3]<<" endedFailure="<<stats.sessionEnds[4]
            <<" backoffRestStarts="<<stats.backoffRestStarts
            <<" meanSleepNeedAtStart="<<(stats.sessionStarts?stats.startNeedSum/stats.sessionStarts:0)
            <<" meanSleepNeedAtEnd="<<(stats.sessionClosed?stats.endNeedSum/stats.sessionClosed:0)
            <<" totalSleepSaturationMinutes="<<m.saturatedMinutes[2]<<" maximumContinuousSleepSaturationMinutes="<<m.longestSaturatedStreak[2]
            <<" saturatedWhileSleeping="<<a.saturatedSleepMinutes<<" saturatedWhileTravelling="<<a.saturatedTravelMinutes
            <<" saturatedDuringCooldown="<<a.saturatedCooldownMinutes
            <<" saturatedGoalSleep="<<a.saturationByGoal[2]<<" saturatedGoalToilet="<<a.saturationByGoal[3]<<" saturatedGoalIdle="<<a.saturationByGoal[5]
            <<" precipitationMinutes="<<a.precipitationMinutes<<" severeExposureMinutes="<<a.severeExposureMinutes
            <<" meanExposure="<<(a.observations?a.exposureSum/a.observations:0)
            <<" meanPrecipitation="<<(a.observations?a.precipitationSum/a.observations:0)
            <<" meanWetness="<<(a.observations?a.wetnessSum/a.observations:0)
            <<" meanWind="<<(a.observations?a.windSum/a.observations:0)<<"\n";
    }
    int beds=0,roofs=0;
    for(const auto& facility:sim.world().facilities) if(facilityOperationalAndActive(facility)){
        beds+=facility.kind==FacilityKind::SleepingPlace; roofs+=facility.kind==FacilityKind::Shelter;
    }
    std::cout<<"SLEEP_WORLD seed="<<seed<<" operationalSleepingPlaceCount="<<beds<<" operationalShelterCount="<<roofs
        <<" meanOperationalSleepingPlaces="<<(worldAudit.observations?double(worldAudit.sleepingPlaceMinutes)/worldAudit.observations:0)
        <<" meanOperationalShelters="<<(worldAudit.observations?double(worldAudit.shelterMinutes)/worldAudit.observations:0)
        <<" meanShelterDurability="<<(worldAudit.shelterSamples?worldAudit.shelterDurabilitySum/worldAudit.shelterSamples:0)
        <<" meanShelterEffectiveness="<<(worldAudit.shelterSamples?worldAudit.shelterEffectivenessSum/worldAudit.shelterSamples:0)
        <<" precipitationMinutes="<<worldAudit.precipitationMinutes<<" severeExposureMinutes="<<worldAudit.severeExposureMinutes<<"\n";
}

void observeMigrationHour(
    Simulation& sim,
    std::map<CharacterId,ResidentMetrics>& metrics)
{
    for(const Character& resident:sim.world().characters){
        if(!resident.alive) continue;
        ResidentMetrics& item=ensureResidentMetrics(metrics,resident);
        const MigrationPressureObservation migration=
            sim.observeResidentMigrationPressure(resident.id);
        item.maxMigrationPressure=std::max(
            item.maxMigrationPressure,migration.pressure01);
        if(migration.candidate) ++item.migrationCandidateHours;
    }
}

struct PersistentStateGrowthMetrics {
    std::size_t memoryEntries=0;
    std::size_t beliefEntries=0;
    std::size_t lifeHistoryEntries=0;
    std::size_t socialFacts=0;
    std::size_t knowledgeReceipts=0;
    std::size_t relationships=0;
    std::size_t romances=0;
    std::size_t resourceNodes=0;
    std::size_t resourcePatches=0;
    std::size_t environmentalResidues=0;
};

PersistentStateGrowthMetrics persistentStateGrowthMetrics(
    const Simulation& sim)
{
    PersistentStateGrowthMetrics result;
    const World& world=sim.world();
    for(const Character& resident:world.characters){
        result.memoryEntries+=resident.memory.entries.size();
        result.beliefEntries+=resident.beliefs.beliefs.size();
        result.lifeHistoryEntries+=resident.lifeHistory.size();
    }
    result.socialFacts=sim.socialKnowledge().facts().size();
    result.knowledgeReceipts=sim.socialKnowledge().receipts().size();
    result.relationships=sim.relationships().size();
    result.romances=sim.romances().all().size();
    result.resourceNodes=world.resourceNodes.size();
    for(const GeneratedNaturalChunk& chunk:world.generatedNaturalChunks){
        result.resourcePatches+=chunk.resourcePatches.size();
    }
    result.environmentalResidues=world.environmentalResidues.all().size();
    return result;
}

void emitFamilyDiagnostics(
    const Simulation& sim,
    std::uint64_t seed,
    int day)
{
    const World& world=sim.world();
    for(const RomancePair& pair:sim.romances().all()){
        if(pair.stage!=RomanceStage::Married) continue;
        const Character* first=findResident(world,pair.first);
        const Character* second=findResident(world,pair.second);
        if(first==nullptr || second==nullptr) continue;

        const Character* gestationalParent=nullptr;
        const Character* partner=nullptr;
        if(first->sex==Sex::Female && second->sex==Sex::Male){
            gestationalParent=first;
            partner=second;
        }else if(second->sex==Sex::Female && first->sex==Sex::Male){
            gestationalParent=second;
            partner=first;
        }

        std::cout
            <<"FAMILY_DIAG"
            <<" seed="<<seed
            <<" day="<<day
            <<" first="<<pair.first
            <<" second="<<pair.second
            <<" marriedMinute="<<pair.marriedMinute;

        if(gestationalParent==nullptr || partner==nullptr){
            std::cout<<" conceptionPair=0\n";
            continue;
        }

        const Relationship* gestationalToPartner=
            sim.relationships().find(
                gestationalParent->id,partner->id);
        const Relationship* partnerToGestational=
            sim.relationships().find(
                partner->id,gestationalParent->id);
        if(gestationalToPartner==nullptr
           || partnerToGestational==nullptr){
            std::cout<<" conceptionPair=1 relationshipData=0\n";
            continue;
        }

        const ReproductiveProfile gestationalProfile=
            autonomousReproductiveProfile(
                *gestationalParent,world.minute);
        const ReproductiveProfile partnerProfile=
            autonomousReproductiveProfile(
                *partner,world.minute);
        const PregnancyContext context=autonomousPregnancyContext(
            *gestationalParent,*partner,
            *gestationalToPartner,*partnerToGestational);
        const PregnancyEvaluation evaluation=
            evaluatePregnancyAttempt(
                *gestationalParent,*partner,
                gestationalProfile,partnerProfile,
                *gestationalToPartner,*partnerToGestational,
                context);
        const bool activePregnancy=
            sim.pregnancies().activeFor(
                gestationalParent->id)!=nullptr;

        std::cout
            <<" conceptionPair=1"
            <<" gestational="<<gestationalParent->id
            <<" partner="<<partner->id
            <<" gestationalAge="<<gestationalProfile.ageYears
            <<" partnerAge="<<partnerProfile.ageYears
            <<" eligible="<<(evaluation.biologicallyEligible ? 1 : 0)
            <<" ready="<<(evaluation.ready ? 1 : 0)
            <<" readiness="<<std::fixed<<std::setprecision(4)
            <<evaluation.attemptReadiness
            <<" conceptionProbability="
            <<evaluation.conceptionProbability
            <<" activePregnancy="<<(activePregnancy ? 1 : 0)
            <<" gestationalHealth="<<gestationalProfile.health
            <<" gestationalFertility="<<gestationalProfile.fertility
            <<" partnerFertility="<<partnerProfile.fertility
            <<" externalStress="<<context.externalStress
            <<"\n";
    }
}

std::size_t encodedSnapshotBytes(Simulation& sim)
{
    std::vector<std::uint8_t> bytes;
    std::string error;
    if(!encodeSimulationSnapshot(
            sim.captureSnapshot(),bytes,&error)){
        return 0;
    }
    return bytes.size();
}

void emitCheckpoint(
    Simulation& sim,
    std::uint64_t seed,
    int day,
    std::size_t initialPopulation,
    const std::map<CharacterId,ResidentMetrics>& metrics,
    const EventMetrics& events,
    std::chrono::steady_clock::time_point start)
{
    const World& world=sim.world();
    const WorldOverviewObservation overview=
        sim.observeWorldOverview();
    const SettlementNetworkObservation settlements=
        sim.observeSettlementNetwork();
    const SettlementTradeNetworkObservation trade=
        sim.observeSettlementTradeNetwork();

    int migrationCandidates=0;
    double maxMigrationPressure=0.0;
    int healthExposed=0;
    int healthIll=0;
    int healthInjured=0;
    int healthCritical=0;
    int maleLiving=0;
    int femaleLiving=0;

    for(const Character& resident:world.characters){
        if(!resident.alive) continue;
        if(resident.sex==Sex::Male) ++maleLiving;
        else ++femaleLiving;

        const MigrationPressureObservation migration=
            sim.observeResidentMigrationPressure(resident.id);
        if(migration.candidate) ++migrationCandidates;
        maxMigrationPressure=std::max(
            maxMigrationPressure,migration.pressure01);

        switch(healthStage(resident.health)){
            case HealthStage::Exposed: ++healthExposed; break;
            case HealthStage::Ill:
            case HealthStage::Recovering: ++healthIll; break;
            case HealthStage::Injured: ++healthInjured; break;
            case HealthStage::Critical: ++healthCritical; break;
            case HealthStage::Well: break;
        }
    }

    const auto elapsedMs=
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now()-start).count();
    const std::size_t snapshotBytes=encodedSnapshotBytes(sim);
    const std::size_t bornAfterStart=
        overview.totalResidents>initialPopulation
            ? overview.totalResidents-initialPopulation
            : 0;

    std::cout
        <<"CHECKPOINT"
        <<" seed="<<seed
        <<" day="<<day
        <<" minute="<<world.minute
        <<" totalResidents="<<overview.totalResidents
        <<" living="<<overview.livingResidents
        <<" deceased="<<overview.deceasedResidents
        <<" bornAfterStart="<<bornAfterStart
        <<" maleLiving="<<maleLiving
        <<" femaleLiving="<<femaleLiving
        <<" babies="<<overview.lifeStages.baby
        <<" toddlers="<<overview.lifeStages.toddler
        <<" children="<<overview.lifeStages.child
        <<" teens="<<overview.lifeStages.teen
        <<" households="<<overview.households
        <<" activeCouples="<<overview.activeCouples
        <<" dating="<<overview.datingCouples
        <<" engaged="<<overview.engagedCouples
        <<" married="<<overview.marriedCouples
        <<" pregnancies="<<overview.activePregnancies
        <<" majorLifeEvents="<<overview.majorLifeEvents
        <<" snapshotBytes="<<snapshotBytes
        <<" elapsedMs="<<elapsedMs
        <<"\n";

    const PersistentStateGrowthMetrics growth=
        persistentStateGrowthMetrics(sim);
    std::cout
        <<"STATE_GROWTH"
        <<" seed="<<seed
        <<" day="<<day
        <<" memoryEntries="<<growth.memoryEntries
        <<" beliefEntries="<<growth.beliefEntries
        <<" lifeHistoryEntries="<<growth.lifeHistoryEntries
        <<" socialFacts="<<growth.socialFacts
        <<" knowledgeReceipts="<<growth.knowledgeReceipts
        <<" relationships="<<growth.relationships
        <<" romances="<<growth.romances
        <<" resourceNodes="<<growth.resourceNodes
        <<" resourcePatches="<<growth.resourcePatches
        <<" environmentalResidues="<<growth.environmentalResidues
        <<" persistedLogTail="<<std::min(
            sim.logs().size(),MaxPersistedSnapshotLogs)
        <<"\n";
    emitFamilyDiagnostics(sim,seed,day);

    std::cout
        <<"SETTLEMENT"
        <<" seed="<<seed
        <<" day="<<day
        <<" settlements="<<settlements.settlementCount
        <<" activeSettlements="<<settlements.activeSettlementCount
        <<" residentsAssigned="<<settlements.residentAssignedCount
        <<" migrationCandidates="<<migrationCandidates
        <<" maxMigrationPressure="<<std::fixed<<std::setprecision(4)
        <<maxMigrationPressure
        <<" tradeRoutes="<<trade.routeCount
        <<" activeTradeRoutes="<<trade.activeRouteCount
        <<" interSettlementPartnerships="
        <<trade.interSettlementPartnershipCount
        <<" tradeEvidence="<<trade.exchangeEvidenceCount
        <<"\n";

    std::cout
        <<"WORLD"
        <<" seed="<<seed
        <<" day="<<day
        <<" naturalWater="<<naturalUnits(world,MaterialKind::Water)
        <<" naturalFood="<<naturalUnits(world,MaterialKind::PlantFood)
        <<" carriedWater="<<carriedUnits(world,MaterialKind::Water)
        <<" carriedFood="<<carriedUnits(world,MaterialKind::PlantFood)
        <<" storedWater="<<storedUnits(world,MaterialKind::Water)
        <<" storedFood="<<storedUnits(world,MaterialKind::PlantFood)
        <<" chunks="<<world.generatedNaturalChunks.size()
        <<" facilities="<<world.facilities.size()
        <<" storages="<<world.storageSites.size()
        <<" primitiveStorageFacilities="
        <<facilityCount(world,FacilityKind::PrimitiveStorage)
        <<" firePits="<<facilityCount(world,FacilityKind::FirePit)
        <<" workSurfaces="<<facilityCount(world,FacilityKind::WorkSurface)
        <<" sleepingPlaces="<<facilityCount(world,FacilityKind::SleepingPlace)
        <<" shelters="<<facilityCount(world,FacilityKind::Shelter)
        <<" furnaces="<<facilityCount(world,FacilityKind::Furnace)
        <<" plots="<<facilityCount(world,FacilityKind::CultivatedPlot)
        <<"\n";

    std::cout
        <<"HEALTH"
        <<" seed="<<seed
        <<" day="<<day
        <<" exposed="<<healthExposed
        <<" illOrRecovering="<<healthIll
        <<" injured="<<healthInjured
        <<" critical="<<healthCritical
        <<"\n";

    std::cout
        <<"EVENTS"
        <<" seed="<<seed
        <<" day="<<day
        <<" routeFailures="<<events.routeFailures
        <<" timeouts="<<events.timeouts
        <<" preemptions="<<events.preemptions
        <<" sleepInterruptions="<<events.sleepInterruptions
        <<" socialEvents="<<events.socialEvents
        <<" civilizationEvents="<<events.civilizationEvents
        <<" tradeDepartures="<<events.tradeDepartures
        <<" tradeExchanges="<<events.tradeExchanges
        <<" tradeReturns="<<events.tradeReturns
        <<" deathsIllness="<<events.deathsIllness
        <<" deathsAccident="<<events.deathsAccident
        <<" deathsExposure="<<events.deathsExposure
        <<" deathsDeprivation="<<events.deathsDeprivation
        <<" deathsOther="<<events.deathsOther
        <<" becameIll="<<events.becameIll
        <<" recoveredIllness="<<events.recoveredIllness
        <<" parentingFeed="<<events.parentingActions[0]
        <<" parentingPutToSleep="<<events.parentingActions[1]
        <<" parentingBathe="<<events.parentingActions[2]
        <<" parentingToiletAssist="<<events.parentingActions[3]
        <<" parentingHold="<<events.parentingActions[4]
        <<" parentingPlay="<<events.parentingActions[5]
        <<" parentingEducate="<<events.parentingActions[6]
        <<" parentingDiscipline="<<events.parentingActions[7]
        <<" parentingComfort="<<events.parentingActions[8]
        <<" parentingHealthCare="<<events.parentingActions[9]
        <<" startsEat="<<events.physicalStarts[0]
        <<" startsDrink="<<events.physicalStarts[1]
        <<" startsSleep="<<events.physicalStarts[2]
        <<" startsToilet="<<events.physicalStarts[3]
        <<" startsWash="<<events.physicalStarts[4]
        <<" doneEat="<<events.physicalCompletions[0]
        <<" doneDrink="<<events.physicalCompletions[1]
        <<" doneSleep="<<events.physicalCompletions[2]
        <<" doneToilet="<<events.physicalCompletions[3]
        <<" doneWash="<<events.physicalCompletions[4]
        <<"\n";

    const std::array<const char*,5> needNames={
        "hunger","thirst","sleep","bladder","hygiene"
    };
    for(const auto& pair:metrics){
        const ResidentMetrics& item=pair.second;
        const Character* resident=findResident(world,item.id);
        if(resident==nullptr) continue;
        const double denom=
            static_cast<double>(std::max<std::uint64_t>(
                1,item.observedMinutes));
        std::cout
            <<"RESIDENT"
            <<" seed="<<seed
            <<" day="<<day
            <<" id="<<item.id
            <<" alive="<<(resident->alive?1:0)
            <<" stage="<<lifeStageName(resident->lifeStage)
            <<" sex="<<sexName(resident->sex)
            <<" observedMin="<<item.observedMinutes;
        for(std::size_t i=0;i<needNames.size();++i){
            std::cout
                <<" "<<needNames[i]<<"Avg="
                <<std::fixed<<std::setprecision(4)
                <<(item.needSum[i]/denom)
                <<" "<<needNames[i]<<"Max="<<item.needMax[i]
                <<" "<<needNames[i]<<"SatMin="<<item.saturatedMinutes[i]
                <<" "<<needNames[i]<<"LongestSat="
                <<item.longestSaturatedStreak[i];
        }
        std::cout
            <<" sleepSessions="<<item.sleepSessions
            <<" sleep30Sessions="<<item.sleepThirtyMinuteSessions
            <<" meaningfulSleepSessions="<<item.meaningfulSleepSessions
            <<" longestSleepSession="<<item.longestSleepSessionMinutes
            <<" sleepRecoverySum="<<item.accumulatedSleepRecovery
            <<" physicalMin="<<item.physicalMinutes
            <<" socialMin="<<item.socialMinutes
            <<" civilizationMin="<<item.civilizationMinutes
            <<" parentingMin="<<item.parentingMinutes
            <<" teachingMin="<<item.teachingMinutes
            <<" tradeMin="<<item.tradeMinutes
            <<" idleMin="<<item.idleMinutes
            <<" longestIdle="<<item.longestIdleStreak
            <<" migrationCandidateHours="<<item.migrationCandidateHours
            <<" maxMigrationPressure="<<item.maxMigrationPressure
            <<" maxIllness="<<item.maxIllness
            <<" maxInjury="<<item.maxInjury
            <<" maxEnvStress="<<item.maxEnvironmentalStress
            <<"\n";
    }
}

} // namespace

int main(int argc,char** argv)
{
    int days=1000;
    std::uint64_t seed=874213954;
    std::string checkpointArg="1,7,30,100,365,1000";
    std::string auditDirectory;
    bool auditProbeOnly=false;
    bool sleepDiagnostics=false;
    std::string sleepTracePath,loadSnapshot,snapshotDirectory;
    int traceStart=0,traceEnd=std::numeric_limits<int>::max();
    CharacterId traceResident=0;

    for(int i=1;i<argc;++i){
        const std::string arg=argv[i];
        if(arg=="--audit-probe-only"){
            auditProbeOnly=true;
        }else if(arg=="--audit-directory" && i+1<argc){
            auditDirectory=argv[++i];
        }else if(arg=="--days" && i+1<argc){
            days=std::max(1,std::atoi(argv[++i]));
        }else if(arg=="--seed" && i+1<argc){
            seed=std::strtoull(argv[++i],nullptr,10);
        }else if(arg=="--checkpoints" && i+1<argc){
            checkpointArg=argv[++i];
        }else if(arg=="--sleep-diagnostics"){
            sleepDiagnostics=true;
        }else if(arg=="--sleep-trace" && i+1<argc){
            sleepTracePath=argv[++i]; sleepDiagnostics=true;
        }else if(arg=="--sleep-trace-start" && i+1<argc){traceStart=std::atoi(argv[++i]);
        }else if(arg=="--sleep-trace-end" && i+1<argc){traceEnd=std::atoi(argv[++i]);
        }else if(arg=="--sleep-trace-resident" && i+1<argc){traceResident=std::strtoull(argv[++i],nullptr,10);
        }else if(arg=="--load-snapshot" && i+1<argc){loadSnapshot=argv[++i];
        }else if(arg=="--snapshot-directory" && i+1<argc){snapshotDirectory=argv[++i];
        }
    }

    const std::vector<int> checkpoints=
        parseCheckpoints(checkpointArg,days);
    Simulation sim(seed);
    sim.setupNewGame();
    if(!loadSnapshot.empty()){
        std::ifstream input(loadSnapshot,std::ios::binary);
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        SimulationStateSnapshot snapshot; std::string error;
        if(!decodeSimulationSnapshot(bytes,snapshot,&error) || !sim.restoreSnapshot(snapshot,&error)){
            std::cerr<<"snapshot load failed: "<<error<<"\n"; return 1;
        }
    }
    sim.enableSleepDiagnostics(sleepDiagnostics);
    std::ofstream sleepTrace;
    if(!sleepTracePath.empty()){
        sleepTrace.open(sleepTracePath);
        if(!sleepTrace){std::cerr<<"cannot write sleep trace\n";return 1;}
        sleepTrace<<"minute,id,sleep,bladder,hygiene,hunger,thirst,goal,action,penaltyUntilMinute,sleepContext,facilityId,facilityType,distance,exposure,recoveryPerMinute,remainingTicks,lastReplanMinute,lastEndReason,lastEndMinute,lastFailureMinute,lastFailedGoal,routeFailure,x,y,targetX,targetY,hasTarget,arrived,illness\n";
    }
    if(!snapshotDirectory.empty()) std::filesystem::create_directories(snapshotDirectory);
    const std::size_t initialPopulation=
        sim.world().characters.size();

    lifelens::audit::SystemBalanceAudit systemAudit(sim,auditDirectory);
    if(auditProbeOnly){
        if(loadSnapshot.empty()){std::cerr<<"--audit-probe-only requires --load-snapshot\n";return 1;}
        systemAudit.probePlanning();
        return systemAudit.finish() ? 0 : 2;
    }
    EventMetrics events;
    sim.onEvent([&](const std::string& line){
        systemAudit.event(line);
        if(line.find("route failed")!=std::string::npos){
            ++events.routeFailures;
        }
        if(line.find("timed out")!=std::string::npos){
            ++events.timeouts;
        }
        if(line.find("preempted current activity")!=std::string::npos){
            ++events.preemptions;
        }
        if(line.find("woke from Sleep")!=std::string::npos){
            ++events.sleepInterruptions;
        }
        if(line.find(" -> Civilization ")!=std::string::npos){
            ++events.civilizationEvents;
        }
        if(line.find(" -> Approach ")!=std::string::npos
           || line.find(" -> Avoid ")!=std::string::npos
           || line.find(" -> Repair ")!=std::string::npos
           || line.find(" -> Comfort ")!=std::string::npos){
            ++events.socialEvents;
        }
        if(line.find("departed settlement")!=std::string::npos
           && line.find("to trade with")!=std::string::npos){
            ++events.tradeDepartures;
        }
        if(line.find("traveled from settlement")!=std::string::npos
           && line.find("and exchanged")!=std::string::npos){
            ++events.tradeExchanges;
        }
        if(line.find("returned from inter-settlement trade")!=std::string::npos){
            ++events.tradeReturns;
        }

        if(line.find("died from illness")!=std::string::npos){
            ++events.deathsIllness;
        }else if(line.find("died from an accident")!=std::string::npos){
            ++events.deathsAccident;
        }else if(line.find("died from environmental exposure")!=std::string::npos){
            ++events.deathsExposure;
        }else if(line.find("died from severe deprivation")!=std::string::npos){
            ++events.deathsDeprivation;
        }else if(line.find(" died")!=std::string::npos){
            ++events.deathsOther;
        }
        if(line.find("became ill after accumulated pathogen exposure")!=std::string::npos){
            ++events.becameIll;
        }
        if(line.find("recovered from illness and gained resilience")!=std::string::npos){
            ++events.recoveredIllness;
        }

        const std::array<const char*,10> parentingNames={
            "Feed","PutToSleep","Bathe","ToiletAssist","Hold",
            "Play","Educate","Discipline","Comfort","HealthCare"
        };
        if(line.find(" cared for ")!=std::string::npos){
            for(std::size_t i=0;i<parentingNames.size();++i){
                const std::string token=
                    std::string(" -> ")+parentingNames[i];
                if(line.find(token)!=std::string::npos){
                    ++events.parentingActions[i];
                    break;
                }
            }
        }

        const std::array<const char*,5> physicalNames={
            "Eat","Drink","Sleep","UseToilet","Wash"
        };
        for(std::size_t i=0;i<physicalNames.size();++i){
            const std::string startToken=
                std::string(" -> ")+physicalNames[i]+" (need ";
            const std::string completeToken=
                std::string(" completed ")+physicalNames[i];
            if(line.find(startToken)!=std::string::npos){
                ++events.physicalStarts[i];
            }
            if(line.find(completeToken)!=std::string::npos){
                ++events.physicalCompletions[i];
            }
        }
    });

    std::map<CharacterId,ResidentMetrics> metrics;
    for(const Character& resident:sim.world().characters){
        ensureResidentMetrics(metrics,resident);
    }

    std::map<CharacterId,SleepAuditMetrics> sleepAudits;
    SleepWorldAudit sleepWorld;
    if(!sim.world().characters.empty()) sim.runtimePosition(sim.world().characters.front().id,sleepWorld.origin);
    std::size_t checkpointIndex=0;
    const int totalMinutes=days*MinutesPerDay;
    const auto start=std::chrono::steady_clock::now();

    for(int minute=1;minute<=totalMinutes;++minute){
        systemAudit.beforeStep();
        sim.step();
        systemAudit.minute();
        if(minute%MinutesPerDay==0) systemAudit.sample(minute/MinutesPerDay,false);

        for(Character& resident:sim.world().characters){
            ResidentMetrics& item=
                ensureResidentMetrics(metrics,resident);
            if(!resident.alive){
                closeSleepSession(item,&resident);
                continue;
            }
            observeResidentMinute(sim,resident,item);
            if(sleepDiagnostics) observeSleepAuditMinute(sim,resident,sleepAudits[resident.id],sleepTrace,traceStart,traceEnd,traceResident);
        }

        if(sleepDiagnostics) observeSleepWorldMinute(sim,sleepWorld);
        if(minute%60==0){
            observeMigrationHour(sim,metrics);
        }

        if(checkpointIndex<checkpoints.size()
           && minute>=checkpoints[checkpointIndex]*MinutesPerDay){
            systemAudit.sample(checkpoints[checkpointIndex],true);
            emitCheckpoint(
                sim,
                seed,
                checkpoints[checkpointIndex],
                initialPopulation,
                metrics,
                events,
                start);
            if(!snapshotDirectory.empty()){
                std::vector<std::uint8_t> bytes; std::string error;
                if(!encodeSimulationSnapshot(sim.captureSnapshot(),bytes,&error)){std::cerr<<error<<"\n";return 1;}
                std::ofstream output(snapshotDirectory+"/day-"+std::to_string(checkpoints[checkpointIndex])+".llsave",std::ios::binary);
                output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
                if(!output){std::cerr<<"snapshot write failed\n";return 1;}
            }
            std::cout.flush();
            ++checkpointIndex;
        }
    }

    for(auto& pair:metrics){
        closeSleepSession(
            pair.second,
            findResident(sim.world(),pair.first));
    }

    if(sleepDiagnostics) emitSleepAudit(sim,seed,sleepAudits,metrics,sleepWorld);
    std::cout
        <<"AUDIT_COMPLETE"
        <<" seed="<<seed
        <<" days="<<days
        <<" checkpoints="<<checkpoints.size()
        <<"\n";
    return systemAudit.finish() ? 0 : 2;
}
