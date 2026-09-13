#include <iostream>

#include "lifelens/Aging.h"
#include "lifelens/Pregnancy.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeCharacter(CharacterId id,int ageYears,int childCount=0)
{
    Character c;
    c.id=id;
    c.name="Resident";
    c.hasBirthMinute=true;
    c.birthMinute=0;
    c.lifeStage=lifeStageForAgeYears(ageYears);
    c.genetics.healthPotential=0.70;
    for(int i=0;i<childCount;++i) c.childrenIds.push_back(1000+i);
    return c;
}

static Relationship strongBond(CharacterId from,CharacterId to)
{
    Relationship r;
    r.from=from;
    r.to=to;
    r.affection=0.90;
    r.trust=0.90;
    r.commitment=0.90;
    r.comfort=0.85;
    return r;
}

static PregnancyContext readyContext()
{
    PregnancyContext c;
    c.gestationalIntent=0.95;
    c.partnerIntent=0.95;
    c.lifeSituation=0.90;
    c.householdCondition=0.90;
    c.financialReadiness=0.85;
    c.externalStress=0.02;
    return c;
}

int main()
{
    const LifeCondition young=lifeConditionForAge(25,0.70,0);
    const LifeCondition middle=lifeConditionForAge(55,0.70,0);
    const LifeCondition elderly=lifeConditionForAge(80,0.70,0);

    CHECK(young.energyCapacity>middle.energyCapacity);
    CHECK(middle.energyCapacity>elderly.energyCapacity);
    CHECK(young.movementCapacity>elderly.movementCapacity);
    CHECK(young.reproductivePotential>middle.reproductivePotential);
    CHECK(elderly.reproductivePotential==0.0);
    CHECK(elderly.appearanceAgeFactor>middle.appearanceAgeFactor);
    CHECK(elderly.familyRoleSalience>young.familyRoleSalience);

    const LifeCondition noChildren=lifeConditionForAge(55,0.70,0);
    const LifeCondition withChildren=lifeConditionForAge(55,0.70,3);
    CHECK(withChildren.lifeGoalFamilyFocus>noChildren.lifeGoalFamilyFocus);
    CHECK(withChildren.familyRoleSalience>noChildren.familyRoleSalience);

    Character resident=makeCharacter(1,25,2);
    resident.lifeCondition.physicalHealth=0.62; // prior illness/injury should not be healed by aging.
    const int age55Minute=55*LifeMinutesPerYear;
    AgingSnapshot aged=advanceAging(resident,age55Minute);
    CHECK(aged.ageYears==55);
    CHECK(resident.lifeStage==LifeStage::MiddleAge);
    CHECK(resident.lifeCondition.physicalHealth<=0.62);
    CHECK(resident.lifeCondition.energyCapacity<1.0);
    CHECK(resident.lifeCondition.reproductivePotential<1.0);
    CHECK(resident.lifeCondition.appearanceAgeFactor>0.0);
    CHECK(resident.lifeCondition.familyRoleSalience>=withChildren.familyRoleSalience-0.05);

    const double healthAt55=resident.lifeCondition.physicalHealth;
    const int age80Minute=80*LifeMinutesPerYear;
    advanceAging(resident,age80Minute);
    CHECK(resident.lifeStage==LifeStage::Elderly);
    CHECK(resident.lifeCondition.physicalHealth<=healthAt55);
    CHECK(resident.lifeCondition.reproductivePotential==0.0);
    CHECK(resident.lifeCondition.workCapacity<0.30);

    // Aging condition must materially feed into pregnancy probability, not just UI.
    Character gestational;
    gestational.id=10;
    Character partner;
    partner.id=11;
    gestational.lifeCondition=lifeConditionForAge(28,0.75,0);
    partner.lifeCondition=lifeConditionForAge(30,0.75,0);

    ReproductiveProfile gp;
    gp.ageYears=28;
    gp.health=0.95;
    gp.fertility=0.95;
    gp.canGestate=true;
    gp.canContributeGenetics=true;
    ReproductiveProfile pp;
    pp.ageYears=30;
    pp.health=0.95;
    pp.fertility=0.95;
    pp.canContributeGenetics=true;

    Relationship gToP=strongBond(gestational.id,partner.id);
    Relationship pToG=strongBond(partner.id,gestational.id);
    const PregnancyEvaluation youngEval=evaluatePregnancyAttempt(
        gestational,partner,gp,pp,gToP,pToG,readyContext());
    CHECK(youngEval.biologicallyEligible);
    CHECK(youngEval.conceptionProbability>0.0);

    // Same nominal profile but diminished lifetime reproductive condition lowers probability.
    Character reduced=gestational;
    reduced.lifeCondition.reproductivePotential=0.20;
    reduced.lifeCondition.physicalHealth=0.60;
    const PregnancyEvaluation reducedEval=evaluatePregnancyAttempt(
        reduced,partner,gp,pp,gToP,pToG,readyContext());
    CHECK(reducedEval.biologicallyEligible);
    CHECK(reducedEval.conceptionProbability<youngEval.conceptionProbability);

    Character noPotential=gestational;
    noPotential.lifeCondition.reproductivePotential=0.0;
    const PregnancyEvaluation ineligible=evaluatePregnancyAttempt(
        noPotential,partner,gp,pp,gToP,pToG,readyContext());
    CHECK(!ineligible.biologicallyEligible);
    CHECK(ineligible.conceptionProbability==0.0);

    return 0;
}
