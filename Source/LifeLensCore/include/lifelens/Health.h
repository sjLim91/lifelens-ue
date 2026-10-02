#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "Needs.h"

namespace lifelens {

enum class HealthStage : std::uint8_t {
    Well = 0,
    Exposed,
    Ill,
    Recovering,
    Injured,
    Critical
};

enum class HealthFatalCause : std::uint8_t {
    None = 0,
    Illness,
    Accident,
    EnvironmentalExposure,
    // Appended so earlier ordinals stay stable. Severe unmet physiological
    // needs are a distinct causal path from pathogens, accidents and climate.
    Deprivation
};

struct HealthState {
    // Generic pathogen burden is an authority-level biological state. It is
    // driven by actual environmental/water exposure rather than a scripted
    // random "disease event".
    double pathogenLoad=0.0;
    double illnessSeverity=0.0;
    double immunity01=0.10;
    double injurySeverity=0.0;
    double environmentalStress01=0.0;
    double careKnowledge01=0.0;

    // Water exposure is accumulated at the actual drinking interaction and
    // consumed by the next daily health progression.
    double pendingWaterContaminationDose=0.0;

    int infectionEpisodes=0;
    int recoveryEpisodes=0;
    int accidentEpisodes=0;
    int lastExposureMinute=-1;
    int lastIllnessMinute=-1;
    int lastRecoveryMinute=-1;
    int lastAccidentMinute=-1;

    void normalize()
    {
        pathogenLoad=std::clamp(pathogenLoad,0.0,1.0);
        illnessSeverity=std::clamp(illnessSeverity,0.0,1.0);
        immunity01=std::clamp(immunity01,0.0,1.0);
        injurySeverity=std::clamp(injurySeverity,0.0,1.0);
        environmentalStress01=std::clamp(environmentalStress01,0.0,1.0);
        careKnowledge01=std::clamp(careKnowledge01,0.0,1.0);
        pendingWaterContaminationDose=std::clamp(
            pendingWaterContaminationDose,0.0,1.0);
        infectionEpisodes=std::max(0,infectionEpisodes);
        recoveryEpisodes=std::max(0,recoveryEpisodes);
        accidentEpisodes=std::max(0,accidentEpisodes);
    }
};

struct DailyHealthInputs {
    double localContamination01=0.0;
    double heatStress01=0.0;
    double coldStress01=0.0;
    double wetStress01=0.0;
    double hazardPotential01=0.0;

    double hunger01=0.0;
    double thirst01=0.0;
    double sleep01=0.0;
    double hygiene01=0.0;

    double geneticHealthPotential01=0.5;
    double baselinePhysicalHealth01=1.0;
    // 1.0 for residents who cannot independently satisfy basic Needs
    // (currently Baby/Toddler), lower/zero for autonomous residents.
    // This is an input to daily health evaluation, not persisted health state.
    double directCareDependency01=0.0;
    bool sanitationKnowledge=false;
};

struct DailyHealthOutcome {
    HealthStage before=HealthStage::Well;
    HealthStage after=HealthStage::Well;
    bool becameIll=false;
    bool recovered=false;
    bool accidentOccurred=false;
    HealthFatalCause fatalCause=HealthFatalCause::None;
};

inline const char* healthStageName(HealthStage stage)
{
    switch(stage){
        case HealthStage::Well: return "Well";
        case HealthStage::Exposed: return "Exposed";
        case HealthStage::Ill: return "Ill";
        case HealthStage::Recovering: return "Recovering";
        case HealthStage::Injured: return "Injured";
        case HealthStage::Critical: return "Critical";
    }
    return "Well";
}

inline double healthClamp01(double value)
{
    return std::clamp(value,0.0,1.0);
}

inline HealthStage healthStage(const HealthState& state)
{
    if(state.illnessSeverity>=0.78
       || state.injurySeverity>=0.78
       || state.environmentalStress01>=0.90){
        return HealthStage::Critical;
    }
    if(state.injurySeverity>=0.18
       && state.injurySeverity>=state.illnessSeverity){
        return HealthStage::Injured;
    }
    if(state.illnessSeverity>=0.18) return HealthStage::Ill;
    if(state.illnessSeverity>=0.06
       && state.pathogenLoad<0.28){
        return HealthStage::Recovering;
    }
    if(state.pathogenLoad>=0.20) return HealthStage::Exposed;
    return HealthStage::Well;
}

inline double healthFunctionalCapacity01(const HealthState& state)
{
    return healthClamp01(
        1.0
        -0.55*state.illnessSeverity
        -0.48*state.injurySeverity
        -0.24*state.environmentalStress01);
}

inline NeedsDelta healthNeedsPressurePerMinute(const HealthState& state)
{
    NeedsDelta result;
    const double illness=healthClamp01(state.illnessSeverity);
    const double injury=healthClamp01(state.injurySeverity);
    const double exposure=healthClamp01(state.environmentalStress01);
    result.thirst=0.00010*illness+0.00005*exposure;
    result.hunger=0.00005*illness;
    result.sleep=0.00022*illness+0.00016*injury+0.00008*exposure;
    result.hygiene=0.00005*illness;
    return result;
}

inline void recordContaminatedWaterExposure(
    HealthState& state,
    double contamination01,
    int currentMinute)
{
    const double contamination=healthClamp01(contamination01);
    if(contamination<=0.01) return;
    state.pendingWaterContaminationDose=healthClamp01(
        state.pendingWaterContaminationDose
        +0.42*contamination);
    state.lastExposureMinute=std::max(state.lastExposureMinute,currentMinute);
}

inline std::uint64_t healthMix64(std::uint64_t value)
{
    value+=0x9E3779B97F4A7C15ULL;
    value=(value^(value>>30))*0xBF58476D1CE4E5B9ULL;
    value=(value^(value>>27))*0x94D049BB133111EBULL;
    return value^(value>>31);
}

inline double deprivationFatalChance(const DailyHealthInputs& input)
{
    // Needs are sampled once per day. Only the extreme end of Hunger/Thirst
    // contributes so a transient ordinary urgent Need is not lethal.
    // Thirst is intentionally the dominant acute pressure. Direct-care
    // dependents are more vulnerable because they cannot self-resolve it.
    const double severeThirst=healthClamp01(
        (healthClamp01(input.thirst01)-0.97)/0.03);
    const double severeHunger=healthClamp01(
        (healthClamp01(input.hunger01)-0.985)/0.015);
    const double dependency=healthClamp01(input.directCareDependency01);

    const double autonomousChance=
        0.035*severeThirst
        +0.010*severeHunger;
    return healthClamp01(
        autonomousChance*(1.0+1.5*dependency));
}

inline double deterministicHealthRoll(
    std::uint64_t worldSeed,
    std::uint64_t characterId,
    int dayIndex,
    std::uint64_t domain)
{
    std::uint64_t state=healthMix64(worldSeed^domain);
    state=healthMix64(state^healthMix64(characterId));
    state=healthMix64(
        state^healthMix64(static_cast<std::uint64_t>(std::max(0,dayIndex))));
    constexpr double Denominator=9007199254740992.0; // 2^53
    return static_cast<double>(state>>11)/Denominator;
}

inline DailyHealthOutcome advanceHealthOneDay(
    HealthState& state,
    const DailyHealthInputs& rawInputs,
    std::uint64_t worldSeed,
    std::uint64_t characterId,
    int currentMinute)
{
    HealthState beforeState=state;
    state.normalize();

    DailyHealthInputs input=rawInputs;
    input.localContamination01=healthClamp01(input.localContamination01);
    input.heatStress01=healthClamp01(input.heatStress01);
    input.coldStress01=healthClamp01(input.coldStress01);
    input.wetStress01=healthClamp01(input.wetStress01);
    input.hazardPotential01=healthClamp01(input.hazardPotential01);
    input.hunger01=healthClamp01(input.hunger01);
    input.thirst01=healthClamp01(input.thirst01);
    input.sleep01=healthClamp01(input.sleep01);
    input.hygiene01=healthClamp01(input.hygiene01);
    input.geneticHealthPotential01=healthClamp01(input.geneticHealthPotential01);
    input.baselinePhysicalHealth01=healthClamp01(input.baselinePhysicalHealth01);
    input.directCareDependency01=healthClamp01(input.directCareDependency01);

    DailyHealthOutcome outcome;
    outcome.before=healthStage(beforeState);

    const double sanitationMitigation=input.sanitationKnowledge ? 0.76 : 1.0;
    const double contaminationDose=
        input.localContamination01
        *(0.14+0.10*input.hygiene01)
        *sanitationMitigation;
    const double deprivation=
        0.32*input.hunger01
        +0.42*input.thirst01
        +0.26*input.sleep01;
    const double innateResilience=
        healthClamp01(
            0.30
            +0.38*input.geneticHealthPotential01
            +0.22*input.baselinePhysicalHealth01
            +0.10*state.immunity01);
    const double pathogenClearance=
        0.035
        +0.070*state.immunity01
        +0.035*innateResilience
        +0.025*state.careKnowledge01;

    const double exposureDose=
        contaminationDose+state.pendingWaterContaminationDose;
    if(exposureDose>0.015){
        state.lastExposureMinute=std::max(state.lastExposureMinute,currentMinute);
    }
    state.pathogenLoad=healthClamp01(
        state.pathogenLoad+exposureDose-pathogenClearance);
    state.pendingWaterContaminationDose=0.0;

    const bool wasIll=beforeState.illnessSeverity>=0.18;
    if(state.pathogenLoad>=0.28){
        const double illnessGain=
            0.045
            +0.12*(state.pathogenLoad-0.28)
            +0.045*deprivation
            +0.030*input.hygiene01;
        state.illnessSeverity=healthClamp01(
            state.illnessSeverity+illnessGain);
    }else{
        const double recovery=
            0.045
            +0.065*innateResilience
            +0.050*state.immunity01
            +0.035*state.careKnowledge01
            -0.030*deprivation;
        state.illnessSeverity=healthClamp01(
            state.illnessSeverity-std::max(0.015,recovery));
    }

    const bool isIll=state.illnessSeverity>=0.18;
    if(!wasIll && isIll){
        ++state.infectionEpisodes;
        state.lastIllnessMinute=currentMinute;
        outcome.becameIll=true;
    }

    // Recovery is an episode-level transition, not a one-day threshold jump.
    // Severity commonly leaves the Ill band several days before reaching the
    // recovered band; keep the episode open until it actually resolves.
    const bool unresolvedIllnessEpisode=
        state.infectionEpisodes>state.recoveryEpisodes;
    if(unresolvedIllnessEpisode && state.illnessSeverity<0.08){
        ++state.recoveryEpisodes;
        state.lastRecoveryMinute=currentMinute;
        outcome.recovered=true;
        // Surviving an episode raises pathogen resilience and practical care
        // knowledge without inventing a technology unlock.
        state.immunity01=healthClamp01(
            state.immunity01+0.08+0.10*beforeState.illnessSeverity);
        state.careKnowledge01=healthClamp01(
            state.careKnowledge01+0.035+0.035*beforeState.illnessSeverity);
    }

    if(state.illnessSeverity>0.10){
        state.immunity01=healthClamp01(
            state.immunity01+0.012*state.illnessSeverity);
    }else{
        state.immunity01=healthClamp01(state.immunity01-0.0012);
    }
    if(input.sanitationKnowledge){
        state.careKnowledge01=healthClamp01(
            state.careKnowledge01+0.0015);
    }

    const double thermalStress=std::max(
        input.heatStress01,input.coldStress01);
    state.environmentalStress01=healthClamp01(
        0.60*state.environmentalStress01
        +0.28*thermalStress
        +0.10*input.wetStress01
        +0.12*input.thirst01);

    const int dayIndex=std::max(0,currentMinute/(24*60));
    const double vulnerability=
        healthClamp01(
            0.35
            +0.35*(1.0-input.baselinePhysicalHealth01)
            +0.20*state.illnessSeverity
            +0.10*input.sleep01);
    const double accidentChance=
        std::max(
            0.0,
            (input.hazardPotential01-0.38)
            *0.020
            *vulnerability);
    if(accidentChance>0.0
       && deterministicHealthRoll(
           worldSeed,characterId,dayIndex,0x4845414c54484143ULL)
            <accidentChance){
        const double injury=
            0.12
            +0.30*input.hazardPotential01
            +0.10*thermalStress;
        state.injurySeverity=healthClamp01(
            state.injurySeverity+injury);
        ++state.accidentEpisodes;
        state.lastAccidentMinute=currentMinute;
        outcome.accidentOccurred=true;
    }else{
        const double injuryRecovery=
            0.018
            +0.025*innateResilience
            +0.018*state.careKnowledge01;
        state.injurySeverity=healthClamp01(
            state.injurySeverity-injuryRecovery);
    }

    const double illnessFatalChance=
        state.illnessSeverity>0.82
            ? (state.illnessSeverity-0.82)
                *0.12
                *(1.35-0.75*innateResilience)
            : 0.0;
    const double accidentFatalChance=
        state.injurySeverity>0.86
            ? (state.injurySeverity-0.86)
                *0.18
                *(1.25-0.55*input.baselinePhysicalHealth01)
            : 0.0;
    const double exposureFatalChance=
        state.environmentalStress01>0.92
            ? (state.environmentalStress01-0.92)
                *0.20
                *(1.20-0.50*input.baselinePhysicalHealth01)
            : 0.0;
    const double deprivationChance=deprivationFatalChance(input);

    // Acute deprivation is evaluated before slower disease/accident paths.
    // This closes the previous immortal-dependent hole without fabricating a
    // caregiver or provision: survival still depends on actual Need relief.
    if(
        deprivationChance>0.0
        && deterministicHealthRoll(
            worldSeed,characterId,dayIndex,0x4845414c54484450ULL)
            <deprivationChance){
        outcome.fatalCause=HealthFatalCause::Deprivation;
    }else if(
        illnessFatalChance>0.0
        && deterministicHealthRoll(
            worldSeed,characterId,dayIndex,0x4845414c5448494cULL)
            <illnessFatalChance){
        outcome.fatalCause=HealthFatalCause::Illness;
    }else if(
        accidentFatalChance>0.0
        && deterministicHealthRoll(
            worldSeed,characterId,dayIndex,0x4845414c54484641ULL)
            <accidentFatalChance){
        outcome.fatalCause=HealthFatalCause::Accident;
    }else if(
        exposureFatalChance>0.0
        && deterministicHealthRoll(
            worldSeed,characterId,dayIndex,0x4845414c54484558ULL)
            <exposureFatalChance){
        outcome.fatalCause=HealthFatalCause::EnvironmentalExposure;
    }

    state.normalize();
    outcome.after=healthStage(state);
    return outcome;
}

} // namespace lifelens
