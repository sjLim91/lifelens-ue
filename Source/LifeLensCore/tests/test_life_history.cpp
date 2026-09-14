#include <iostream>
#include <random>
#include <string>

#include "lifelens/Birth.h"
#include "lifelens/Marriage.h"
#include "lifelens/Pregnancy.h"
#include "lifelens/Romance.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeAdult(CharacterId id,const char* name)
{
    Character c;
    c.id=id;
    c.name=name;
    c.lifeStage=LifeStage::Adult;
    c.personality.riskTolerance=0.9;
    c.personality.conscientiousness=0.9;
    c.personality.patience=0.9;
    c.lifeCondition.physicalHealth=1.0;
    c.lifeCondition.reproductivePotential=1.0;
    return c;
}

static Relationship strongBond(CharacterId from,CharacterId to)
{
    Relationship r;
    r.from=from;
    r.to=to;
    r.affection=0.95;
    r.trust=0.95;
    r.respect=0.90;
    r.comfort=0.92;
    r.familiarity=0.94;
    r.attraction=0.94;
    r.romanticInterest=0.94;
    r.sexualAttraction=0.90;
    r.commitment=0.92;
    return r;
}

static RomanceContext romanceContext()
{
    RomanceContext c;
    c.personalityCompatibility=0.95;
    c.sharedExperience=0.90;
    c.lifeGoalAlignment=0.95;
    return c;
}

static MarriageContext marriageContext()
{
    MarriageContext c;
    c.marriageIntent=0.95;
    c.lifeGoalAlignment=0.95;
    c.householdStability=0.90;
    c.financialReadiness=0.90;
    c.relationshipDuration=0.90;
    c.mergeHouseholdsOnMarriage=true;
    return c;
}

int main()
{
    Character a=makeAdult(1,"A");
    Character b=makeAdult(2,"B");
    Relationship aToB=strongBond(a.id,b.id);
    Relationship bToA=strongBond(b.id,a.id);
    RomanceBook romances;

    DatingProposalOutcome dating=applyDatingProposal(
        a,b,aToB,bToA,romanceContext(),romanceContext(),romances,100,0.40,0.40);
    CHECK(dating.result==DatingProposalResult::Accepted);
    CHECK(hasLifeEvent(a.lifeHistory,LifeEventType::DatingStarted));
    CHECK(hasLifeEvent(b.lifeHistory,LifeEventType::DatingStarted));
    CHECK(latestLifeEvent(a.lifeHistory,LifeEventType::DatingStarted)->relatedCharacters[0]==b.id);

    EngagementProposalOutcome engagement=applyEngagementProposal(
        a,b,aToB,bToA,marriageContext(),marriageContext(),romances,200,0.40,0.40);
    CHECK(engagement.result==EngagementProposalResult::Accepted);
    CHECK(hasLifeEvent(a.lifeHistory,LifeEventType::Engaged));
    CHECK(hasLifeEvent(b.lifeHistory,LifeEventType::Engaged));

    HouseholdBook households;
    CHECK(households.create(10,{a.id},101,400.0));
    CHECK(households.create(20,{b.id},202,500.0));
    MarriageDecisionOutcome marriage=applyMarriageDecision(
        a,b,aToB,bToA,marriageContext(),marriageContext(),romances,households,
        300,30,303,1200.0,0.40,0.40);
    CHECK(marriage.result==MarriageDecisionResult::Married);
    CHECK(hasLifeEvent(a.lifeHistory,LifeEventType::Married));
    CHECK(hasLifeEvent(b.lifeHistory,LifeEventType::Married));
    CHECK(hasLifeEvent(a.lifeHistory,LifeEventType::CohabitationStarted));
    CHECK(hasLifeEvent(b.lifeHistory,LifeEventType::CohabitationStarted));

    ReproductiveProfile gestational;
    gestational.ageYears=29;
    gestational.health=1.0;
    gestational.fertility=1.0;
    gestational.canGestate=true;
    ReproductiveProfile partner;
    partner.ageYears=31;
    partner.health=1.0;
    partner.fertility=1.0;
    partner.canContributeGenetics=true;
    PregnancyContext pregnancyContext;
    pregnancyContext.gestationalIntent=1.0;
    pregnancyContext.partnerIntent=1.0;
    pregnancyContext.lifeSituation=1.0;
    pregnancyContext.householdCondition=1.0;
    pregnancyContext.financialReadiness=1.0;

    PregnancyBook pregnancies;
    PregnancyAttemptOutcome conceived=applyPregnancyAttempt(
        a,b,gestational,partner,aToB,bToA,pregnancyContext,pregnancies,400,0.0,0.30);
    CHECK(conceived.result==PregnancyAttemptResult::Conceived);
    CHECK(hasLifeEvent(a.lifeHistory,LifeEventType::PregnancyStarted));
    CHECK(hasLifeEvent(b.lifeHistory,LifeEventType::PregnancyStarted));

    PregnancyState* pregnancy=pregnancies.activeFor(a.id);
    CHECK(pregnancy!=nullptr);
    const int dueMinute=pregnancy->dueMinute;
    BirthBook births;
    std::mt19937_64 rng(42);
    BirthOutcome birth=performBirth(
        a,b,3,"Child",pregnancies,households,births,rng,dueMinute,0.08);
    CHECK(birth.result==BirthResult::Success);
    CHECK(hasLifeEvent(birth.child.lifeHistory,LifeEventType::Birth));
    CHECK(hasLifeEvent(a.lifeHistory,LifeEventType::ChildBorn));
    CHECK(hasLifeEvent(b.lifeHistory,LifeEventType::ChildBorn));
    CHECK(latestLifeEvent(a.lifeHistory,LifeEventType::ChildBorn)->relatedCharacters[0]==3);

    CHECK(countLifeEvents(a.lifeHistory,LifeEventType::DatingStarted)==1);
    CHECK(std::string(lifeEventName(LifeEventType::PregnancyStarted))=="PregnancyStarted");

    std::vector<LifeHistoryEntry> clamped;
    recordLifeEvent(clamped,LifeEventType::HouseholdChanged,-50,{},7);
    CHECK(clamped[0].minute==0);
    CHECK(clamped[0].value==7);
    return 0;
}
