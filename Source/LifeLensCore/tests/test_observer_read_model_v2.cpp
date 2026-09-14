#include <iostream>

#include "lifelens/ObserverReadModelV2.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeResident(CharacterId id,const char* name,LifeStage stage)
{
    Character c;
    c.id=id;
    c.name=name;
    c.lifeStage=stage;
    return c;
}

int main()
{
    World world(42);
    world.minute=12345;
    world.characters.push_back(makeResident(1,"A",LifeStage::Adult));
    world.characters.push_back(makeResident(2,"B",LifeStage::Adult));
    world.characters.push_back(makeResident(3,"Child",LifeStage::Child));
    world.characters.push_back(makeResident(4,"Sibling",LifeStage::Baby));

    Character& a=world.characters[0];
    Character& b=world.characters[1];
    Character& child=world.characters[2];
    Character& sibling=world.characters[3];

    a.childrenIds={3,4};
    b.childrenIds={3,4};
    child.parentIds={1,2};
    sibling.parentIds={1,2};

    a.emotion.joy=0.20;
    a.emotion.sadness=0.30;
    a.emotion.anger=0.10;
    a.emotion.fear=0.05;
    a.emotion.embarrassment=0.07;
    a.emotion.pride=0.40;
    a.emotion.jealousy=0.11;
    a.emotion.affection=0.80;
    a.emotion.anxiety=0.15;
    a.emotion.relief=0.25;
    a.emotion.grief=0.35;
    a.emotion.refreshSummary();

    EmotionObservation emotion=makeEmotionObservation(a);
    CHECK(emotion.joy==0.20);
    CHECK(emotion.grief==0.35);
    CHECK(emotion.affection==0.80);
    CHECK(emotion.intensity==0.80);
    CHECK(emotion.valence==a.emotion.valence);

    GenealogyBook genealogy;
    CHECK(genealogy.registerBirth(child.id,a.id,b.id));
    CHECK(genealogy.registerBirth(sibling.id,a.id,b.id));

    RomanceBook romances;
    CHECK(romances.startDating(a.id,b.id,a.id,100));
    CHECK(romances.engage(a.id,b.id,200));
    CHECK(romances.marry(a.id,b.id,300));

    HouseholdBook households;
    CHECK(households.create(10,{a.id,b.id,child.id,sibling.id},99,1200.0));

    PregnancyBook pregnancies;
    CHECK(pregnancies.start(a.id,b.id,400)!=nullptr);

    FamilyObservation aFamily=buildFamilyObservation(
        world,genealogy,romances,households,pregnancies,a);
    CHECK(aFamily.hasRomanceHistory);
    CHECK(aFamily.hasActivePartner);
    CHECK(aFamily.partnerId==b.id);
    CHECK(aFamily.partnerName=="B");
    CHECK(aFamily.partnerStage==RomanceStage::Married);
    CHECK(aFamily.cohabitingWithPartner);
    CHECK(aFamily.isGestationalParent);
    CHECK(aFamily.expectingChild);
    CHECK(aFamily.pregnancyPartnerId==b.id);
    CHECK(aFamily.children.size()==2);

    FamilyObservation bFamily=buildFamilyObservation(
        world,genealogy,romances,households,pregnancies,b);
    CHECK(!bFamily.isGestationalParent);
    CHECK(bFamily.expectingChild);
    CHECK(bFamily.pregnancyPartnerId==a.id);

    FamilyObservation childFamily=buildFamilyObservation(
        world,genealogy,romances,households,pregnancies,child);
    CHECK(childFamily.parents.size()==2);
    CHECK(childFamily.siblings.size()==1);
    CHECK(childFamily.siblings[0].id==sibling.id);
    CHECK(childFamily.siblings[0].kinship==KinshipType::Sibling);

    recordLifeEvent(a.lifeHistory,LifeEventType::Married,300,{b.id});
    recordLifeEvent(a.lifeHistory,LifeEventType::PregnancyStarted,400,{b.id});

    WorldOverviewObservation overview=buildWorldOverviewObservation(
        world,households,romances,pregnancies);
    CHECK(overview.minute==12345);
    CHECK(overview.totalResidents==4);
    CHECK(overview.livingResidents==4);
    CHECK(overview.deceasedResidents==0);
    CHECK(overview.lifeStages.adult==2);
    CHECK(overview.lifeStages.child==1);
    CHECK(overview.lifeStages.baby==1);
    CHECK(overview.households==1);
    CHECK(overview.activeCouples==1);
    CHECK(overview.marriedCouples==1);
    CHECK(overview.activePregnancies==1);
    CHECK(overview.majorLifeEvents==2);
    CHECK(std::string(romanceStageName(aFamily.partnerStage))=="Married");

    return 0;
}
