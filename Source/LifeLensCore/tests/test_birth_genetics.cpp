#include <cmath>
#include <iostream>
#include <random>

#include "lifelens/Birth.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeParent(CharacterId id,const char* name,double geneticBase)
{
    Character c;
    c.id=id;
    c.name=name;
    c.genetics.faceShape=clampGenetic(geneticBase+0.02);
    c.genetics.eyePigment=clampGenetic(geneticBase+0.04);
    c.genetics.hairPigment=clampGenetic(geneticBase+0.06);
    c.genetics.skinTone=clampGenetic(geneticBase+0.08);
    c.genetics.heightPotential=clampGenetic(geneticBase+0.10);
    c.genetics.buildPotential=clampGenetic(geneticBase+0.12);
    c.genetics.healthPotential=clampGenetic(geneticBase+0.14);
    c.genetics.learningPotential=clampGenetic(geneticBase+0.16);
    c.genetics.temperamentSensitivity=clampGenetic(geneticBase+0.18);
    return c;
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

static bool bounded(const GeneticsProfile& g)
{
    const double values[]={
        g.faceShape,g.eyePigment,g.hairPigment,g.skinTone,g.heightPotential,
        g.buildPotential,g.healthPotential,g.learningPotential,g.temperamentSensitivity};
    for(double value:values) if(value<0.0 || value>1.0) return false;
    return true;
}

int main()
{
    Character parentA=makeParent(1,"ParentA",0.20);
    Character parentB=makeParent(2,"ParentB",0.68);

    // Genetics are deterministic for the same parents + seed, but include variation.
    std::mt19937_64 expectedRng(99123);
    const GeneticsProfile expected=inheritGenetics(parentA.genetics,parentB.genetics,expectedRng,0.08);
    std::mt19937_64 repeatRng(99123);
    const GeneticsProfile repeated=inheritGenetics(parentA.genetics,parentB.genetics,repeatRng,0.08);
    CHECK(sameGenetics(expected,repeated));
    CHECK(bounded(expected));

    PregnancyBook pregnancies;
    PregnancyState* pregnancy=pregnancies.start(parentA.id,parentB.id,5000);
    CHECK(pregnancy!=nullptr);
    const int dueMinute=pregnancy->dueMinute;

    HouseholdBook households;
    CHECK(households.create(77,{parentA.id,parentB.id},9001,1500.0));

    BirthBook births;
    std::mt19937_64 birthRng(99123);

    // Birth cannot happen before the pregnancy is due.
    BirthOutcome tooEarly=performBirth(
        parentA,parentB,100,"Baby",pregnancies,households,births,birthRng,dueMinute-1,0.08);
    CHECK(tooEarly.result==BirthResult::NotDue);
    CHECK(pregnancies.activeFor(parentA.id)!=nullptr);
    CHECK(households.householdOf(100)==nullptr);

    // Re-seed so the successful birth uses the expected deterministic genetics.
    birthRng.seed(99123);
    BirthOutcome born=performBirth(
        parentA,parentB,100,"Baby",pregnancies,households,births,birthRng,dueMinute,0.08);
    CHECK(born.result==BirthResult::Success);
    CHECK(born.child.id==100);
    CHECK(born.child.name=="Baby");
    CHECK(born.child.birthMinute==dueMinute);
    CHECK(born.child.parentIds.size()==2);
    CHECK(born.child.parentIds[0]==parentA.id);
    CHECK(born.child.parentIds[1]==parentB.id);
    CHECK(sameGenetics(born.child.genetics,expected));
    CHECK(bounded(born.child.genetics));

    CHECK(born.child.lifeHistory.size()==1);
    CHECK(born.child.lifeHistory[0].type==LifeEventType::Birth);
    CHECK(born.child.lifeHistory[0].minute==dueMinute);
    CHECK(born.child.lifeHistory[0].relatedCharacters.size()==2);

    CHECK(parentA.childrenIds.size()==1 && parentA.childrenIds[0]==100);
    CHECK(parentB.childrenIds.size()==1 && parentB.childrenIds[0]==100);
    CHECK(pregnancies.activeFor(parentA.id)==nullptr);

    const BirthRecord* record=births.find(100);
    CHECK(record!=nullptr);
    CHECK(record->parentA==parentA.id);
    CHECK(record->parentB==parentB.id);
    CHECK(record->householdId==77);
    const Household* familyHome=households.householdOf(100);
    CHECK(familyHome!=nullptr && familyHome->id==77);
    CHECK(familyHome->contains(parentA.id));
    CHECK(familyHome->contains(parentB.id));
    CHECK(familyHome->contains(100));

    // A new pregnancy cannot reuse an already registered child GUID.
    PregnancyState* secondPregnancy=pregnancies.start(parentA.id,parentB.id,dueMinute+100);
    CHECK(secondPregnancy!=nullptr);
    const int secondDue=secondPregnancy->dueMinute;
    std::mt19937_64 duplicateRng(5);
    BirthOutcome duplicate=performBirth(
        parentA,parentB,100,"OtherBaby",pregnancies,households,births,duplicateRng,secondDue,0.08);
    CHECK(duplicate.result==BirthResult::DuplicateChild);
    CHECK(pregnancies.activeFor(parentA.id)!=nullptr);

    // Pregnancy partner identity is enforced.
    Character wrongPartner=makeParent(3,"Wrong",0.45);
    std::mt19937_64 wrongRng(5);
    BirthOutcome wrong=performBirth(
        parentA,wrongPartner,101,"WrongBaby",pregnancies,households,births,wrongRng,secondDue,0.08);
    CHECK(wrong.result==BirthResult::WrongPartner);

    return 0;
}
