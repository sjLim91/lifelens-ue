#include <cmath>
#include <iostream>

#include "lifelens/LifeCycle.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static bool near(double a,double b,double eps=1e-9)
{
    return std::fabs(a-b)<=eps;
}

int main()
{
    // Existing/founder characters without a birth timestamp keep their configured adult stage.
    Character founder;
    founder.id=1;
    founder.lifeStage=LifeStage::Adult;
    GrowthSnapshot founderGrowth=advanceCharacterGrowth(founder,90*LifeMinutesPerYear);
    CHECK(!founderGrowth.changed);
    CHECK(founder.lifeStage==LifeStage::Adult);

    Character child;
    child.id=100;
    child.hasBirthMinute=true;
    child.birthMinute=1000;
    child.lifeStage=LifeStage::Baby;
    child.baseMetabolism=1.10;
    child.baseSleepTendency=0.90;
    applyLifeStageProfile(child,LifeStage::Baby);

    GrowthSnapshot baby=observeGrowth(child,child.birthMinute+300);
    CHECK(baby.ageYears==0);
    CHECK(baby.stage==LifeStage::Baby);
    CHECK(!baby.profile.canWork);
    CHECK(!baby.profile.canFormRomance);
    CHECK(!baby.profile.canLiveIndependently);
    CHECK(baby.profile.autonomy<0.10);

    const int toddlerMinute=child.birthMinute+2*LifeMinutesPerYear;
    GrowthSnapshot toddler=advanceCharacterGrowth(child,toddlerMinute);
    CHECK(toddler.changed);
    CHECK(child.lifeStage==LifeStage::Toddler);
    CHECK(child.lifeHistory.size()==1);
    CHECK(child.lifeHistory[0].type==LifeEventType::LifeStageChanged);
    CHECK(child.lifeHistory[0].value==static_cast<int>(LifeStage::Toddler));
    CHECK(near(child.metabolism,child.baseMetabolism*lifeStageProfile(LifeStage::Toddler).metabolismMultiplier));
    CHECK(near(child.sleepTendency,child.baseSleepTendency*lifeStageProfile(LifeStage::Toddler).sleepTendencyMultiplier));

    // A large simulation jump records every crossed stage rather than silently skipping childhood.
    const int teenMinute=child.birthMinute+14*LifeMinutesPerYear;
    GrowthSnapshot teen=advanceCharacterGrowth(child,teenMinute);
    CHECK(teen.changed);
    CHECK(teen.ageYears==14);
    CHECK(child.lifeStage==LifeStage::Teen);
    CHECK(child.lifeHistory.size()==3);
    CHECK(child.lifeHistory[1].value==static_cast<int>(LifeStage::Child));
    CHECK(child.lifeHistory[2].value==static_cast<int>(LifeStage::Teen));
    CHECK(teen.profile.canAttendSchool);
    CHECK(teen.profile.canFormRomance);
    CHECK(!teen.profile.canMarry);
    CHECK(!teen.profile.canParent);

    const int youngAdultMinute=child.birthMinute+18*LifeMinutesPerYear;
    GrowthSnapshot youngAdult=advanceCharacterGrowth(child,youngAdultMinute);
    CHECK(youngAdult.changed);
    CHECK(child.lifeStage==LifeStage::YoungAdult);
    CHECK(youngAdult.profile.canWork);
    CHECK(youngAdult.profile.canLiveIndependently);
    CHECK(youngAdult.profile.canMarry);
    CHECK(youngAdult.profile.canParent);

    const int middleAgeMinute=child.birthMinute+45*LifeMinutesPerYear;
    GrowthSnapshot middle=advanceCharacterGrowth(child,middleAgeMinute);
    CHECK(middle.changed);
    CHECK(child.lifeStage==LifeStage::MiddleAge);
    CHECK(middle.profile.responsibility>0.90);

    const int elderlyMinute=child.birthMinute+65*LifeMinutesPerYear;
    GrowthSnapshot elderly=advanceCharacterGrowth(child,elderlyMinute);
    CHECK(elderly.changed);
    CHECK(child.lifeStage==LifeStage::Elderly);
    CHECK(elderly.profile.movementScale<middle.profile.movementScale);
    CHECK(!elderly.profile.canWork);
    CHECK(!elderly.profile.canParent);

    // Inspecting an earlier time never regresses the resident's actual stage.
    GrowthSnapshot oldSnapshot=advanceCharacterGrowth(child,teenMinute);
    CHECK(!oldSnapshot.changed);
    CHECK(child.lifeStage==LifeStage::Elderly);

    CHECK(lifeStageForAgeYears(0)==LifeStage::Baby);
    CHECK(lifeStageForAgeYears(2)==LifeStage::Toddler);
    CHECK(lifeStageForAgeYears(5)==LifeStage::Child);
    CHECK(lifeStageForAgeYears(13)==LifeStage::Teen);
    CHECK(lifeStageForAgeYears(18)==LifeStage::YoungAdult);
    CHECK(lifeStageForAgeYears(25)==LifeStage::Adult);
    CHECK(lifeStageForAgeYears(45)==LifeStage::MiddleAge);
    CHECK(lifeStageForAgeYears(65)==LifeStage::Elderly);

    return 0;
}
