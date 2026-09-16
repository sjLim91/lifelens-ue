#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "Character.h"
#include "Relationship.h"
#include "Romance.h"

namespace lifelens {

constexpr int PregnancyMinutePerDay=24*60;
constexpr int PregnancyGestationDays=280;
constexpr int PregnancyGestationMinutes=PregnancyGestationDays*PregnancyMinutePerDay;

inline double clampPregnancy(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

enum class PregnancyStage {
    FirstTrimester,
    SecondTrimester,
    ThirdTrimester,
    Due,
    Completed
};

struct ReproductiveProfile {
    int ageYears=25;
    double health=1.0;
    double fertility=1.0;
    bool canGestate=false;
    bool canContributeGenetics=true;
};

struct PregnancyContext {
    double gestationalIntent=0.5;
    double partnerIntent=0.5;
    double lifeSituation=0.5;
    double householdCondition=0.5;
    double financialReadiness=0.5;
    double externalStress=0.0;
    bool available=true;
};

struct PregnancyEvaluation {
    double attemptReadiness=0.0;
    double conceptionProbability=0.0;
    bool ready=false;
    bool biologicallyEligible=false;
};

inline double fertilityAgeFactor(int ageYears)
{
    if(ageYears<18 || ageYears>=50) return 0.0;
    if(ageYears<=34) return 1.0;
    if(ageYears<=39) return 0.78;
    if(ageYears<=44) return 0.42;
    return 0.15;
}

inline PregnancyEvaluation evaluatePregnancyAttempt(
    const Character& gestationalParent,
    const Character& partner,
    const ReproductiveProfile& gestationalProfile,
    const ReproductiveProfile& partnerProfile,
    const Relationship& gestationalToPartner,
    const Relationship& partnerToGestational,
    const PregnancyContext& context,
    double readinessThreshold=0.62)
{
    PregnancyEvaluation result;
    if(gestationalParent.id==0 || partner.id==0 || gestationalParent.id==partner.id || !context.available) return result;

    const double ageFactor=fertilityAgeFactor(gestationalProfile.ageYears);
    const double gestationalLifeFertility=clampPregnancy(
        gestationalParent.lifeCondition.reproductivePotential);
    const double partnerLifeFertility=clampPregnancy(
        partner.lifeCondition.reproductivePotential);
    const bool eligible=gestationalProfile.canGestate &&
        partnerProfile.canContributeGenetics &&
        ageFactor>0.0 && gestationalProfile.health>0.05 &&
        gestationalProfile.fertility>0.0 && partnerProfile.fertility>0.0 &&
        gestationalLifeFertility>0.01 && partnerLifeFertility>0.01;
    result.biologicallyEligible=eligible;
    if(!eligible) return result;

    const double mutualTrust=(gestationalToPartner.trust+partnerToGestational.trust)*0.5;
    const double mutualCommitment=(gestationalToPartner.commitment+partnerToGestational.commitment)*0.5;
    const double mutualAffection=(gestationalToPartner.affection+partnerToGestational.affection)*0.5;
    const double relationshipStability=clampPregnancy(
        0.35*mutualTrust+0.35*mutualCommitment+0.30*mutualAffection-
        0.20*((gestationalToPartner.conflict+partnerToGestational.conflict)*0.5)-
        0.12*((gestationalToPartner.fear+partnerToGestational.fear)*0.5));

    result.attemptReadiness=clampPregnancy(
        0.24*clampPregnancy(context.gestationalIntent)+
        0.20*clampPregnancy(context.partnerIntent)+
        0.18*relationshipStability+
        0.12*clampPregnancy(context.lifeSituation)+
        0.10*clampPregnancy(context.householdCondition)+
        0.07*clampPregnancy(context.financialReadiness)+
        0.04*clampPregnancy(gestationalParent.personality.conscientiousness)+
        0.03*clampPregnancy(partner.personality.conscientiousness)+
        0.02*clampPregnancy(gestationalParent.personality.patience)-
        0.14*clampPregnancy(context.externalStress));

    const double biologicalHealth=clampPregnancy(
        0.40*clampPregnancy(gestationalProfile.health)+
        0.15*clampPregnancy(partnerProfile.health)+
        0.20*clampPregnancy(gestationalParent.lifeCondition.physicalHealth)+
        0.10*clampPregnancy(partner.lifeCondition.physicalHealth)+
        0.15*relationshipStability);
    const double lifeFertilityFactor=std::sqrt(
        gestationalLifeFertility*partnerLifeFertility);
    result.conceptionProbability=clampPregnancy(
        0.32*ageFactor*
        clampPregnancy(gestationalProfile.fertility)*
        clampPregnancy(partnerProfile.fertility)*
        lifeFertilityFactor*
        biologicalHealth);
    result.ready=result.attemptReadiness>=clampPregnancy(readinessThreshold);
    return result;
}

struct PregnancyState {
    CharacterId gestationalParent=0;
    CharacterId geneticPartner=0;
    int conceptionMinute=0;
    int dueMinute=PregnancyGestationMinutes;
    int lastUpdateMinute=0;
    PregnancyStage stage=PregnancyStage::FirstTrimester;
    double health=1.0;
    double fatigue=0.15;
    double stress=0.10;
    double nutrition=0.85;

    bool active() const { return stage!=PregnancyStage::Completed; }
};

inline PregnancyStage pregnancyStageFor(int conceptionMinute,int dueMinute,int currentMinute)
{
    if(currentMinute>=dueMinute) return PregnancyStage::Due;
    const int elapsed=std::max(0,currentMinute-conceptionMinute);
    const int day=elapsed/PregnancyMinutePerDay;
    if(day<14*7) return PregnancyStage::FirstTrimester;
    if(day<28*7) return PregnancyStage::SecondTrimester;
    return PregnancyStage::ThirdTrimester;
}

class PregnancyBook {
public:
    const PregnancyState* activeFor(CharacterId characterId) const
    {
        for(auto it=items_.rbegin();it!=items_.rend();++it){
            if(it->active() && it->gestationalParent==characterId) return &(*it);
        }
        return nullptr;
    }

    PregnancyState* activeFor(CharacterId characterId)
    {
        for(auto it=items_.rbegin();it!=items_.rend();++it){
            if(it->active() && it->gestationalParent==characterId) return &(*it);
        }
        return nullptr;
    }

    PregnancyState* start(CharacterId gestationalParent,CharacterId partner,int conceptionMinute)
    {
        if(gestationalParent==0 || partner==0 || gestationalParent==partner || activeFor(gestationalParent)!=nullptr) return nullptr;
        PregnancyState state;
        state.gestationalParent=gestationalParent;
        state.geneticPartner=partner;
        state.conceptionMinute=std::max(0,conceptionMinute);
        state.dueMinute=state.conceptionMinute+PregnancyGestationMinutes;
        state.lastUpdateMinute=state.conceptionMinute;
        items_.push_back(state);
        return &items_.back();
    }

    bool complete(CharacterId gestationalParent,int currentMinute)
    {
        PregnancyState* state=activeFor(gestationalParent);
        if(state==nullptr || currentMinute<state->dueMinute) return false;
        state->stage=PregnancyStage::Completed;
        state->lastUpdateMinute=std::max(state->lastUpdateMinute,currentMinute);
        return true;
    }

    bool terminate(CharacterId gestationalParent,int currentMinute)
    {
        PregnancyState* state=activeFor(gestationalParent);
        if(state==nullptr) return false;
        state->stage=PregnancyStage::Completed;
        state->lastUpdateMinute=std::max(state->lastUpdateMinute,std::max(0,currentMinute));
        return true;
    }

    const std::vector<PregnancyState>& all() const { return items_; }

private:
    std::vector<PregnancyState> items_;
};

enum class PregnancyAttemptResult {
    Invalid,
    ExistingPregnancy,
    NotEligible,
    NotReady,
    NoConception,
    Conceived
};

struct PregnancyAttemptOutcome {
    PregnancyAttemptResult result=PregnancyAttemptResult::Invalid;
    double readiness=0.0;
    double conceptionProbability=0.0;
};

inline PregnancyAttemptOutcome applyPregnancyAttempt(
    Character& gestationalParent,
    Character& partner,
    const ReproductiveProfile& gestationalProfile,
    const ReproductiveProfile& partnerProfile,
    const Relationship& gestationalToPartner,
    const Relationship& partnerToGestational,
    const PregnancyContext& context,
    PregnancyBook& pregnancies,
    int currentMinute,
    double deterministicRoll,
    double readinessThreshold=0.62)
{
    PregnancyAttemptOutcome outcome;
    if(gestationalParent.id==0 || partner.id==0 || gestationalParent.id==partner.id) return outcome;
    if(pregnancies.activeFor(gestationalParent.id)!=nullptr){
        outcome.result=PregnancyAttemptResult::ExistingPregnancy;
        return outcome;
    }

    const PregnancyEvaluation evaluation=evaluatePregnancyAttempt(
        gestationalParent,partner,gestationalProfile,partnerProfile,
        gestationalToPartner,partnerToGestational,context,readinessThreshold);
    outcome.readiness=evaluation.attemptReadiness;
    outcome.conceptionProbability=evaluation.conceptionProbability;
    if(!evaluation.biologicallyEligible){
        outcome.result=PregnancyAttemptResult::NotEligible;
        return outcome;
    }
    if(!evaluation.ready){
        outcome.result=PregnancyAttemptResult::NotReady;
        return outcome;
    }

    const double roll=clampPregnancy(deterministicRoll);
    if(roll>=evaluation.conceptionProbability){
        outcome.result=PregnancyAttemptResult::NoConception;
        return outcome;
    }

    PregnancyState* state=pregnancies.start(gestationalParent.id,partner.id,currentMinute);
    if(state==nullptr){
        outcome.result=PregnancyAttemptResult::Invalid;
        return outcome;
    }
    applyEmotionEvent(gestationalParent.emotion,EmotionEventType::PositiveSocial,0.35);
    applyEmotionEvent(partner.emotion,EmotionEventType::PositiveSocial,0.35);
    outcome.result=PregnancyAttemptResult::Conceived;
    return outcome;
}

inline void advancePregnancy(PregnancyState& state,Character& gestationalParent,int currentMinute)
{
    if(!state.active()) return;
    const int effectiveMinute=std::max(state.lastUpdateMinute,currentMinute);
    const int deltaMinutes=std::max(0,effectiveMinute-state.lastUpdateMinute);
    state.stage=pregnancyStageFor(state.conceptionMinute,state.dueMinute,effectiveMinute);

    double stageFactor=1.0;
    if(state.stage==PregnancyStage::SecondTrimester) stageFactor=1.10;
    else if(state.stage==PregnancyStage::ThirdTrimester) stageFactor=1.35;
    else if(state.stage==PregnancyStage::Due) stageFactor=1.45;

    gestationalParent.needs.apply({
        0.00016*stageFactor*deltaMinutes,
        0.00012*stageFactor*deltaMinutes,
        0.00013*stageFactor*deltaMinutes,
        0.00015*stageFactor*deltaMinutes,
        0.00003*deltaMinutes
    });

    state.fatigue=clampPregnancy(state.fatigue+0.00005*stageFactor*deltaMinutes);
    state.nutrition=clampPregnancy(state.nutrition-0.000025*stageFactor*deltaMinutes);
    if(state.nutrition<0.30) state.health=clampPregnancy(state.health-0.00003*deltaMinutes);
    if(state.stress>0.70) state.health=clampPregnancy(state.health-0.00002*deltaMinutes);
    state.lastUpdateMinute=effectiveMinute;
}

} // namespace lifelens
