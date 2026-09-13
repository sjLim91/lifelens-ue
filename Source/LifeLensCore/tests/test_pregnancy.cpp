#include <iostream>

#include "lifelens/Pregnancy.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeCharacter(CharacterId id,const char* name)
{
    Character c;
    c.id=id;
    c.name=name;
    c.personality.conscientiousness=0.80;
    c.personality.patience=0.78;
    return c;
}

static Relationship stableBond(CharacterId from,CharacterId to)
{
    Relationship r;
    r.from=from;
    r.to=to;
    r.affection=0.92;
    r.trust=0.91;
    r.commitment=0.90;
    r.comfort=0.88;
    r.familiarity=0.90;
    r.conflict=0.02;
    r.fear=0.01;
    return r;
}

static PregnancyContext readyContext()
{
    PregnancyContext c;
    c.gestationalIntent=0.95;
    c.partnerIntent=0.93;
    c.lifeSituation=0.88;
    c.householdCondition=0.90;
    c.financialReadiness=0.82;
    c.externalStress=0.04;
    return c;
}

int main()
{
    Character gestational=makeCharacter(1,"A");
    Character partner=makeCharacter(2,"B");
    Relationship aToB=stableBond(1,2);
    Relationship bToA=stableBond(2,1);

    ReproductiveProfile gestationalProfile;
    gestationalProfile.ageYears=29;
    gestationalProfile.health=0.92;
    gestationalProfile.fertility=0.90;
    gestationalProfile.canGestate=true;
    gestationalProfile.canContributeGenetics=true;

    ReproductiveProfile partnerProfile;
    partnerProfile.ageYears=31;
    partnerProfile.health=0.90;
    partnerProfile.fertility=0.88;
    partnerProfile.canGestate=false;
    partnerProfile.canContributeGenetics=true;

    const PregnancyEvaluation evaluation=evaluatePregnancyAttempt(
        gestational,partner,gestationalProfile,partnerProfile,
        aToB,bToA,readyContext());
    CHECK(evaluation.biologicallyEligible);
    CHECK(evaluation.ready);
    CHECK(evaluation.attemptReadiness>=0.62);
    CHECK(evaluation.conceptionProbability>0.0);
    CHECK(evaluation.conceptionProbability<1.0);

    PregnancyBook pregnancies;
    const int conceptionMinute=5000;
    PregnancyAttemptOutcome conceived=applyPregnancyAttempt(
        gestational,partner,gestationalProfile,partnerProfile,
        aToB,bToA,readyContext(),pregnancies,conceptionMinute,0.0);
    CHECK(conceived.result==PregnancyAttemptResult::Conceived);
    const PregnancyState* state=pregnancies.activeFor(gestational.id);
    CHECK(state!=nullptr);
    CHECK(state->geneticPartner==partner.id);
    CHECK(state->conceptionMinute==conceptionMinute);
    CHECK(state->dueMinute==conceptionMinute+PregnancyGestationMinutes);
    CHECK(state->stage==PregnancyStage::FirstTrimester);

    PregnancyAttemptOutcome duplicate=applyPregnancyAttempt(
        gestational,partner,gestationalProfile,partnerProfile,
        aToB,bToA,readyContext(),pregnancies,conceptionMinute+10,0.0);
    CHECK(duplicate.result==PregnancyAttemptResult::ExistingPregnancy);

    PregnancyState* mutableState=pregnancies.activeFor(gestational.id);
    CHECK(mutableState!=nullptr);
    const double hungerBefore=gestational.needs.hunger;
    const double sleepBefore=gestational.needs.sleep;
    const double bladderBefore=gestational.needs.bladder;
    const double nutritionBefore=mutableState->nutrition;

    const int secondTrimesterMinute=conceptionMinute+(15*7*PregnancyMinutePerDay);
    advancePregnancy(*mutableState,gestational,secondTrimesterMinute);
    CHECK(mutableState->stage==PregnancyStage::SecondTrimester);
    CHECK(gestational.needs.hunger>hungerBefore);
    CHECK(gestational.needs.sleep>sleepBefore);
    CHECK(gestational.needs.bladder>bladderBefore);
    CHECK(mutableState->nutrition<nutritionBefore);
    CHECK(mutableState->fatigue>0.15);

    const int thirdTrimesterMinute=conceptionMinute+(30*7*PregnancyMinutePerDay);
    advancePregnancy(*mutableState,gestational,thirdTrimesterMinute);
    CHECK(mutableState->stage==PregnancyStage::ThirdTrimester);

    advancePregnancy(*mutableState,gestational,mutableState->dueMinute);
    CHECK(mutableState->stage==PregnancyStage::Due);
    CHECK(pregnancies.complete(gestational.id,mutableState->dueMinute));
    CHECK(pregnancies.activeFor(gestational.id)==nullptr);

    // Intent/life situation governs whether an attempt happens, separately from biology.
    Character c=makeCharacter(3,"C");
    Character d=makeCharacter(4,"D");
    Relationship cToD=stableBond(c.id,d.id);
    Relationship dToC=stableBond(d.id,c.id);
    PregnancyContext lowIntent=readyContext();
    lowIntent.gestationalIntent=0.02;
    lowIntent.partnerIntent=0.02;
    lowIntent.lifeSituation=0.05;
    lowIntent.householdCondition=0.10;
    lowIntent.financialReadiness=0.10;
    lowIntent.externalStress=0.90;
    PregnancyBook lowIntentBook;
    PregnancyAttemptOutcome notReady=applyPregnancyAttempt(
        c,d,gestationalProfile,partnerProfile,cToD,dToC,
        lowIntent,lowIntentBook,100,0.0);
    CHECK(notReady.result==PregnancyAttemptResult::NotReady);
    CHECK(lowIntentBook.activeFor(c.id)==nullptr);

    // Biological eligibility is distinct from intent and relationship quality.
    ReproductiveProfile ineligible=gestationalProfile;
    ineligible.fertility=0.0;
    PregnancyBook ineligibleBook;
    PregnancyAttemptOutcome noEligibility=applyPregnancyAttempt(
        c,d,ineligible,partnerProfile,cToD,dToC,
        readyContext(),ineligibleBook,100,0.0);
    CHECK(noEligibility.result==PregnancyAttemptResult::NotEligible);

    // A ready attempt may still not conceive; deterministicRoll lets Simulation seed this later.
    PregnancyBook noConceptionBook;
    PregnancyAttemptOutcome noConception=applyPregnancyAttempt(
        c,d,gestationalProfile,partnerProfile,cToD,dToC,
        readyContext(),noConceptionBook,100,1.0);
    CHECK(noConception.result==PregnancyAttemptResult::NoConception);
    CHECK(noConceptionBook.activeFor(c.id)==nullptr);

    CHECK(fertilityAgeFactor(17)==0.0);
    CHECK(fertilityAgeFactor(29)==1.0);
    CHECK(fertilityAgeFactor(42)<fertilityAgeFactor(35));
    CHECK(fertilityAgeFactor(50)==0.0);

    return 0;
}
