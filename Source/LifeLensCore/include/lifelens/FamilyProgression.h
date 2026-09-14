#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "Aging.h"
#include "Marriage.h"
#include "Pregnancy.h"

namespace lifelens {

constexpr int FamilyProgressionDayMinutes=LifeMinutesPerDay;
constexpr int FamilyProgressionDecisionMinuteOfDay=12*60;
constexpr int FamilyDatingToCohabitationMinutes=30*FamilyProgressionDayMinutes;
constexpr int FamilyDatingToEngagementMinutes=90*FamilyProgressionDayMinutes;
constexpr int FamilyCohabitationToEngagementMinutes=30*FamilyProgressionDayMinutes;
constexpr int FamilyEngagementToMarriageMinutes=60*FamilyProgressionDayMinutes;
constexpr int FamilyMarriageToPregnancyMinutes=30*FamilyProgressionDayMinutes;
constexpr int FamilyPregnancyAttemptIntervalDays=7;

inline double clampFamilyProgression(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

inline double traitSimilarity(double a,double b)
{
    return 1.0-std::abs(clampFamilyProgression(a)-clampFamilyProgression(b));
}

inline double familyPersonalityCompatibility(const Character& a,const Character& b)
{
    const double score=(
        traitSimilarity(a.personality.openness,b.personality.openness)+
        traitSimilarity(a.personality.agreeableness,b.personality.agreeableness)+
        traitSimilarity(a.personality.emotionalStability,b.personality.emotionalStability)+
        traitSimilarity(a.personality.sociability,b.personality.sociability)+
        traitSimilarity(a.personality.patience,b.personality.patience)+
        traitSimilarity(a.personality.conscientiousness,b.personality.conscientiousness))/6.0;
    return clampFamilyProgression(score);
}

inline double familyLifeGoalAlignment(const Character& a,const Character& b)
{
    return clampFamilyProgression(
        0.55*traitSimilarity(a.lifeCondition.lifeGoalFamilyFocus,b.lifeCondition.lifeGoalFamilyFocus)+
        0.25*traitSimilarity(a.personality.ambition,b.personality.ambition)+
        0.20*traitSimilarity(a.personality.orderliness,b.personality.orderliness));
}

inline double familyExternalStress(const Character& actor,const Relationship& towardPartner)
{
    return clampFamilyProgression(
        0.38*towardPartner.conflict+
        0.24*towardPartner.fear+
        0.18*towardPartner.grudge+
        0.12*actor.emotion.anxiety+
        0.08*actor.emotion.anger);
}

inline RomanceContext autonomousRomanceContext(
    const Character& actor,
    const Character& target,
    const Relationship& towardTarget,
    double pastRelationshipPenalty=0.0)
{
    RomanceContext context;
    context.personalityCompatibility=familyPersonalityCompatibility(actor,target);
    context.sharedExperience=clampFamilyProgression(towardTarget.familiarity);
    context.lifeGoalAlignment=familyLifeGoalAlignment(actor,target);
    context.pastRelationshipPenalty=clampFamilyProgression(pastRelationshipPenalty);
    context.available=actor.alive && target.alive &&
        lifeStageProfile(actor.lifeStage).canFormRomance &&
        lifeStageProfile(target.lifeStage).canFormRomance;
    return context;
}

inline CohabitationContext autonomousCohabitationContext(
    const Character& actor,
    const Character& partner,
    const Relationship& towardPartner)
{
    CohabitationContext context;
    const double bond=towardPartner.socialBond();
    context.housingReadiness=clampFamilyProgression(0.50+0.30*bond+0.20*actor.personality.adaptability);
    context.financialReadiness=clampFamilyProgression(0.45+0.30*actor.personality.conscientiousness+0.25*actor.personality.orderliness);
    context.lifeGoalAlignment=familyLifeGoalAlignment(actor,partner);
    context.scheduleCompatibility=clampFamilyProgression(0.50+0.25*familyPersonalityCompatibility(actor,partner)+0.25*actor.personality.patience);
    context.currentHousingPressure=0.20;
    context.externalStress=familyExternalStress(actor,towardPartner);
    context.available=actor.alive && partner.alive && lifeStageProfile(actor.lifeStage).canLiveIndependently;
    return context;
}

inline MarriageContext autonomousMarriageContext(
    const Character& actor,
    const Character& partner,
    const Relationship& towardPartner,
    double relationshipDuration)
{
    MarriageContext context;
    context.marriageIntent=clampFamilyProgression(
        0.40+0.40*actor.lifeCondition.lifeGoalFamilyFocus+0.20*actor.personality.conscientiousness);
    context.lifeGoalAlignment=familyLifeGoalAlignment(actor,partner);
    context.householdStability=clampFamilyProgression(
        0.45+0.30*towardPartner.comfort+0.25*towardPartner.trust);
    context.financialReadiness=clampFamilyProgression(
        0.45+0.30*actor.personality.conscientiousness+0.25*actor.personality.orderliness);
    context.relationshipDuration=clampFamilyProgression(relationshipDuration);
    context.externalStress=familyExternalStress(actor,towardPartner);
    context.available=actor.alive && partner.alive && lifeStageProfile(actor.lifeStage).canMarry;
    context.mergeHouseholdsOnMarriage=true;
    return context;
}

inline PregnancyContext autonomousPregnancyContext(
    const Character& gestationalParent,
    const Character& partner,
    const Relationship& gestationalToPartner,
    const Relationship& partnerToGestational)
{
    PregnancyContext context;
    const double mutualConflict=0.5*(gestationalToPartner.conflict+partnerToGestational.conflict);
    const double mutualTrust=0.5*(gestationalToPartner.trust+partnerToGestational.trust);
    context.gestationalIntent=clampFamilyProgression(
        0.45+0.45*gestationalParent.lifeCondition.lifeGoalFamilyFocus+0.10*gestationalParent.personality.conscientiousness);
    context.partnerIntent=clampFamilyProgression(
        0.45+0.45*partner.lifeCondition.lifeGoalFamilyFocus+0.10*partner.personality.conscientiousness);
    context.lifeSituation=clampFamilyProgression(
        0.45+0.30*gestationalParent.lifeCondition.physicalHealth+0.25*partner.lifeCondition.physicalHealth);
    context.householdCondition=clampFamilyProgression(0.55+0.35*mutualTrust-0.25*mutualConflict);
    context.financialReadiness=clampFamilyProgression(
        0.45+0.275*gestationalParent.personality.conscientiousness+0.275*partner.personality.conscientiousness);
    context.externalStress=clampFamilyProgression(
        0.45*mutualConflict+0.30*gestationalParent.emotion.anxiety+0.25*partner.emotion.anxiety);
    context.available=gestationalParent.alive && partner.alive;
    return context;
}

inline ReproductiveProfile autonomousReproductiveProfile(const Character& character,int currentMinute)
{
    ReproductiveProfile profile;
    profile.ageYears=character.hasBirthMinute
        ? ageYearsFromMinutes(character.birthMinute,currentMinute)
        : 25;
    profile.health=clampFamilyProgression(character.lifeCondition.physicalHealth);
    profile.fertility=clampFamilyProgression(
        0.45*character.genetics.healthPotential+
        0.55*character.lifeCondition.reproductivePotential);
    profile.canGestate=character.sex==Sex::Female;
    profile.canContributeGenetics=character.alive && profile.ageYears>=18;
    return profile;
}

inline void evolveRomanticChemistry(
    const Character& first,
    const Character& second,
    Relationship& firstToSecond,
    Relationship& secondToFirst)
{
    if(!first.alive || !second.alive) return;
    if(!lifeStageProfile(first.lifeStage).canFormRomance || !lifeStageProfile(second.lifeStage).canFormRomance) return;

    const double mutualFamiliarity=0.5*(firstToSecond.familiarity+secondToFirst.familiarity);
    const double mutualBond=0.5*(firstToSecond.socialBond()+secondToFirst.socialBond());
    const double negative=0.5*(
        firstToSecond.conflict+firstToSecond.fear+firstToSecond.grudge+
        secondToFirst.conflict+secondToFirst.fear+secondToFirst.grudge)/3.0;
    if(mutualFamiliarity<0.18 || mutualBond<0.22 || negative>0.45) return;

    const double compatibility=familyPersonalityCompatibility(first,second);
    const double attractionTarget=clampFamilyProgression(
        0.12+0.40*compatibility+0.34*mutualBond+0.14*mutualFamiliarity);
    const double interestTarget=clampFamilyProgression(
        0.05+0.32*compatibility+0.34*mutualBond+0.24*attractionTarget+0.05*mutualFamiliarity);

    const auto approach=[](double current,double target,double rate){
        return clampFamilyProgression(current+(target-current)*rate);
    };

    firstToSecond.attraction=approach(firstToSecond.attraction,attractionTarget,0.055);
    secondToFirst.attraction=approach(secondToFirst.attraction,attractionTarget,0.055);
    if(attractionTarget>=0.42){
        firstToSecond.romanticInterest=approach(firstToSecond.romanticInterest,interestTarget,0.050);
        secondToFirst.romanticInterest=approach(secondToFirst.romanticInterest,interestTarget,0.050);
        firstToSecond.sexualAttraction=approach(firstToSecond.sexualAttraction,0.82*attractionTarget,0.040);
        secondToFirst.sexualAttraction=approach(secondToFirst.sexualAttraction,0.82*attractionTarget,0.040);
    }
}

inline double deterministicFamilyRoll(
    std::uint64_t worldSeed,
    CharacterId first,
    CharacterId second,
    std::uint64_t epoch)
{
    std::uint64_t x=worldSeed ^ (first*0x9E3779B97F4A7C15ull) ^
        (second*0xBF58476D1CE4E5B9ull) ^ (epoch*0x94D049BB133111EBull);
    x+=0x9E3779B97F4A7C15ull;
    x=(x^(x>>30))*0xBF58476D1CE4E5B9ull;
    x=(x^(x>>27))*0x94D049BB133111EBull;
    x^=(x>>31);
    const std::uint64_t mantissa=x>>11;
    return static_cast<double>(mantissa)*(1.0/9007199254740992.0);
}

} // namespace lifelens
