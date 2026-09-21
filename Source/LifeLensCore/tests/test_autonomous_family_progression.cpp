#include <algorithm>
#include <cmath>
#include <iostream>

#include "lifelens/FamilyProgression.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character* characterById(Simulation& sim,CharacterId id)
{
    for(auto& character:sim.world().characters) if(character.id==id) return &character;
    return nullptr;
}

static void makePairReady(Simulation& sim,CharacterId firstId,CharacterId secondId)
{
    Character* first=characterById(sim,firstId);
    Character* second=characterById(sim,secondId);
    if(first==nullptr || second==nullptr) return;

    first->lifeCondition.lifeGoalFamilyFocus=0.92;
    second->lifeCondition.lifeGoalFamilyFocus=0.92;
    first->lifeCondition.physicalHealth=1.0;
    second->lifeCondition.physicalHealth=1.0;
    first->lifeCondition.reproductivePotential=1.0;
    second->lifeCondition.reproductivePotential=1.0;
    first->genetics.healthPotential=0.95;
    second->genetics.healthPotential=0.95;
    first->personality.conscientiousness=0.92;
    second->personality.conscientiousness=0.92;
    first->personality.patience=0.90;
    second->personality.patience=0.90;
    first->personality.orderliness=0.88;
    second->personality.orderliness=0.88;
    first->personality.riskTolerance=0.75;
    second->personality.riskTolerance=0.75;

    Relationship& firstToSecond=sim.relationships().getOrCreate(firstId,secondId);
    Relationship& secondToFirst=sim.relationships().getOrCreate(secondId,firstId);
    for(Relationship* relation:{&firstToSecond,&secondToFirst}){
        relation->affection=0.95;
        relation->trust=0.95;
        relation->respect=0.90;
        relation->comfort=0.95;
        relation->familiarity=0.95;
        relation->attraction=0.95;
        relation->romanticInterest=0.95;
        relation->sexualAttraction=0.90;
        relation->commitment=0.90;
        relation->conflict=0.0;
        relation->jealousy=0.0;
        relation->fear=0.0;
        relation->grudge=0.0;
    }
}

static void advanceToDailyDecision(Simulation& sim,int targetMinute)
{
    sim.world().minute=targetMinute-1;
    sim.step();
}

static void makePairBrokenDown(
    Simulation& sim,
    CharacterId firstId,
    CharacterId secondId)
{
    Relationship& firstToSecond=
        sim.relationships().getOrCreate(firstId,secondId);
    Relationship& secondToFirst=
        sim.relationships().getOrCreate(secondId,firstId);
    for(Relationship* relation:{&firstToSecond,&secondToFirst}){
        relation->affection=0.03;
        relation->trust=0.02;
        relation->respect=0.08;
        relation->comfort=0.03;
        relation->familiarity=0.90;
        relation->attraction=0.04;
        relation->romanticInterest=0.03;
        relation->sexualAttraction=0.02;
        relation->commitment=0.04;
        relation->conflict=0.96;
        relation->jealousy=0.88;
        relation->fear=0.72;
        relation->grudge=0.94;
    }
}

static bool establishDirectMarriage(
    Simulation& sim,
    CharacterId firstId,
    CharacterId secondId,
    HouseholdId householdId,
    double sharedMoney,
    int marriageMinute)
{
    makePairReady(sim,firstId,secondId);
    if(!sim.romances().startDating(
            firstId,secondId,firstId,marriageMinute-20)) return false;
    if(!sim.romances().engage(
            firstId,secondId,marriageMinute-10)) return false;
    if(!sim.romances().marry(
            firstId,secondId,marriageMinute)) return false;
    if(!sim.genealogy().linkSpouses(firstId,secondId)) return false;
    if(!sim.households().create(
            householdId,{firstId,secondId},0,sharedMoney)) return false;

    Character* first=characterById(sim,firstId);
    Character* second=characterById(sim,secondId);
    if(first==nullptr || second==nullptr) return false;
    recordLifeEvent(
        first->lifeHistory,LifeEventType::Married,marriageMinute,{secondId});
    recordLifeEvent(
        second->lifeHistory,LifeEventType::Married,marriageMinute,{firstId});
    return true;
}

static bool sameGenetics(const GeneticsProfile& a,const GeneticsProfile& b)
{
    return a.faceShape==b.faceShape &&
        a.eyePigment==b.eyePigment &&
        a.hairPigment==b.hairPigment &&
        a.skinTone==b.skinTone &&
        a.heightPotential==b.heightPotential &&
        a.buildPotential==b.buildPotential &&
        a.healthPotential==b.healthPotential &&
        a.learningPotential==b.learningPotential &&
        a.temperamentSensitivity==b.temperamentSensitivity;
}

struct ScenarioResult {
    std::string childName;
    Sex childSex=Sex::Male;
    GeneticsProfile childGenetics;
    int conceptionMinute=-1;
    int birthMinute=-1;
};

static bool runFamilyScenario(std::uint64_t seed,ScenarioResult& result)
{
    Simulation sim(seed);
    sim.setupNewGame();
    if(sim.world().characters.size()!=4) return false;

    const CharacterId maleId=sim.world().characters[0].id;
    const CharacterId femaleId=sim.world().characters[2].id;
    makePairReady(sim,maleId,femaleId);

    const int datingDecision=2*FamilyProgressionDayMinutes+FamilyProgressionDecisionMinuteOfDay;
    advanceToDailyDecision(sim,datingDecision);
    const RomancePair* pair=sim.romances().findActivePair(maleId,femaleId);
    if(pair==nullptr || pair->stage!=RomanceStage::Dating) return false;
    const int datingMinute=pair->startedMinute;

    advanceToDailyDecision(sim,datingMinute+FamilyDatingToCohabitationMinutes);
    const Household* maleHome=sim.households().householdOf(maleId);
    const Household* femaleHome=sim.households().householdOf(femaleId);
    if(maleHome==nullptr || femaleHome==nullptr || maleHome->id!=femaleHome->id) return false;

    advanceToDailyDecision(sim,datingMinute+FamilyDatingToEngagementMinutes);
    pair=sim.romances().findActivePair(maleId,femaleId);
    if(pair==nullptr || pair->stage!=RomanceStage::Engaged) return false;
    const int engagedMinute=pair->engagedMinute;

    advanceToDailyDecision(sim,engagedMinute+FamilyEngagementToMarriageMinutes);
    pair=sim.romances().findActivePair(maleId,femaleId);
    if(pair==nullptr || pair->stage!=RomanceStage::Married) return false;
    const int marriedMinute=pair->marriedMinute;

    PregnancyState* pregnancy=nullptr;
    for(int attempt=0;attempt<80 && pregnancy==nullptr;++attempt){
        const int attemptMinute=marriedMinute+
            (30+attempt*FamilyPregnancyAttemptIntervalDays)*FamilyProgressionDayMinutes;
        advanceToDailyDecision(sim,attemptMinute);
        pregnancy=sim.pregnancies().activeFor(femaleId);
    }
    if(pregnancy==nullptr) return false;
    result.conceptionMinute=pregnancy->conceptionMinute;
    const int dueMinute=pregnancy->dueMinute;

    sim.world().minute=dueMinute-1;
    sim.step();
    if(sim.pregnancies().activeFor(femaleId)!=nullptr) return false;
    if(sim.world().characters.size()!=5 || sim.births().all().size()!=1) return false;

    const BirthRecord& birth=sim.births().all().front();
    Character* child=characterById(sim,birth.childId);
    if(child==nullptr) return false;
    if(!sim.genealogy().isDirectParent(maleId,child->id) ||
       !sim.genealogy().isDirectParent(femaleId,child->id)) return false;
    const Household* childHome=sim.households().householdOf(child->id);
    if(childHome==nullptr || childHome->id!=maleHome->id) return false;
    const ResidentObservation childObservation=sim.observeResident(child->id);
    if(childObservation.id!=child->id || childObservation.name!=child->name) return false;

    result.childName=child->name;
    result.childSex=child->sex;
    result.childGenetics=child->genetics;
    result.birthMinute=child->birthMinute;
    return true;
}

int main()
{
    // Formal New Game still starts with no forced couple/family state.
    Simulation untouched(8811);
    untouched.setupNewGame();
    CHECK(untouched.romances().all().empty());
    CHECK(untouched.households().all().empty());
    advanceToDailyDecision(untouched,FamilyProgressionDecisionMinuteOfDay);
    CHECK(untouched.romances().all().empty());

    // Familiarity + social bond can create chemistry without directly forcing a couple.
    Simulation chemistry(9922);
    chemistry.setupNewGame();
    const CharacterId chemistryA=chemistry.world().characters[0].id;
    const CharacterId chemistryB=chemistry.world().characters[2].id;
    Relationship& chemistryAB=chemistry.relationships().getOrCreate(chemistryA,chemistryB);
    Relationship& chemistryBA=chemistry.relationships().getOrCreate(chemistryB,chemistryA);
    chemistryAB.affection=chemistryBA.affection=0.58;
    chemistryAB.trust=chemistryBA.trust=0.58;
    chemistryAB.comfort=chemistryBA.comfort=0.58;
    chemistryAB.familiarity=chemistryBA.familiarity=0.62;
    const double attractionBefore=chemistryAB.attraction;
    for(int day=1;day<=10;++day){
        advanceToDailyDecision(
            chemistry,day*FamilyProgressionDayMinutes+FamilyProgressionDecisionMinuteOfDay);
    }
    CHECK(chemistryAB.attraction>attractionBefore);

    ScenarioResult first;
    ScenarioResult repeated;
    CHECK(runFamilyScenario(20260914,first));
    CHECK(runFamilyScenario(20260914,repeated));
    CHECK(first.childName==repeated.childName);
    CHECK(first.childSex==repeated.childSex);
    CHECK(first.conceptionMinute==repeated.conceptionMinute);
    CHECK(first.birthMinute==repeated.birthMinute);
    CHECK(sameGenetics(first.childGenetics,repeated.childGenetics));

    // A healthy marriage remains stable after the breakdown grace period.
    Simulation stableMarriage(330011);
    stableMarriage.setupNewGame();
    const CharacterId stableA=stableMarriage.world().characters[0].id;
    const CharacterId stableB=stableMarriage.world().characters[2].id;
    const int stableMarriageMinute=FamilyProgressionDecisionMinuteOfDay;
    CHECK(establishDirectMarriage(
        stableMarriage,stableA,stableB,700,800.0,stableMarriageMinute));
    advanceToDailyDecision(
        stableMarriage,
        stableMarriageMinute+FamilyMarriageBreakdownGraceMinutes);
    const RomancePair* stablePair=
        stableMarriage.romances().findLatest(stableA,stableB);
    CHECK(stablePair!=nullptr);
    CHECK(stablePair->stage==RomanceStage::Married);

    // Severe sustained mutual breakdown now progresses through the already
    // modeled Separated/Divorced states instead of leaving every living
    // marriage permanently locked in Married.
    Simulation breakdown(440022);
    breakdown.setupNewGame();
    const CharacterId breakdownA=breakdown.world().characters[0].id;
    const CharacterId breakdownB=breakdown.world().characters[2].id;
    const int breakdownMarriageMinute=FamilyProgressionDecisionMinuteOfDay;
    CHECK(establishDirectMarriage(
        breakdown,breakdownA,breakdownB,800,1000.0,breakdownMarriageMinute));
    makePairBrokenDown(breakdown,breakdownA,breakdownB);

    advanceToDailyDecision(
        breakdown,
        breakdownMarriageMinute+FamilyMarriageBreakdownGraceMinutes);
    const RomancePair* breakdownPair=
        breakdown.romances().findLatest(breakdownA,breakdownB);
    CHECK(breakdownPair!=nullptr);
    CHECK(breakdownPair->stage==RomanceStage::Separated);
    Character* breakdownFirst=characterById(breakdown,breakdownA);
    Character* breakdownSecond=characterById(breakdown,breakdownB);
    CHECK(breakdownFirst!=nullptr);
    CHECK(breakdownSecond!=nullptr);
    const LifeHistoryEntry* separatedEvent=
        latestLifeEvent(
            breakdownFirst->lifeHistory,
            LifeEventType::Separated);
    CHECK(separatedEvent!=nullptr);
    CHECK(breakdown.genealogy().relationBetween(
        breakdownA,breakdownB)==KinshipType::Spouse);
    const Household* separatedHomeA=
        breakdown.households().householdOf(breakdownA);
    const Household* separatedHomeB=
        breakdown.households().householdOf(breakdownB);
    CHECK(separatedHomeA!=nullptr);
    CHECK(separatedHomeB!=nullptr);
    CHECK(separatedHomeA->id==separatedHomeB->id);

    advanceToDailyDecision(
        breakdown,
        separatedEvent->minute+FamilySeparationToDivorceMinutes);
    breakdownPair=breakdown.romances().findLatest(
        breakdownA,breakdownB);
    CHECK(breakdownPair!=nullptr);
    CHECK(breakdownPair->stage==RomanceStage::Divorced);
    CHECK(latestLifeEvent(
        breakdownFirst->lifeHistory,
        LifeEventType::Divorced)!=nullptr);
    CHECK(latestLifeEvent(
        breakdownSecond->lifeHistory,
        LifeEventType::Divorced)!=nullptr);
    CHECK(breakdown.genealogy().relationBetween(
        breakdownA,breakdownB)!=KinshipType::Spouse);

    const Household* divorcedHomeA=
        breakdown.households().householdOf(breakdownA);
    const Household* divorcedHomeB=
        breakdown.households().householdOf(breakdownB);
    CHECK(divorcedHomeA!=nullptr);
    CHECK(divorcedHomeB!=nullptr);
    CHECK(divorcedHomeA->id!=divorcedHomeB->id);
    CHECK(std::abs(
        (divorcedHomeA->sharedMoney+divorcedHomeB->sharedMoney)
        -1000.0)<1e-9);

    return 0;
}
