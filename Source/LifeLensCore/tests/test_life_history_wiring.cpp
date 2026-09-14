#include <iostream>
#include <random>
#include <vector>
#include "lifelens/LifeHistoryWiring.h"
using namespace lifelens;

static int fail(const char* m){ std::cerr << m << '\n'; return 1; }

int main(){
    Character a,b;
    a.id=1; a.name="A"; a.lifeStage=LifeStage::Adult;
    b.id=2; b.name="B"; b.lifeStage=LifeStage::Adult;

    Relationship ab; ab.from=1; ab.to=2;
    Relationship ba; ba.from=2; ba.to=1;
    RomanceContext rc;
    RomanceBook romances;

    auto dating=applyDatingProposalTracked(a,b,ab,ba,rc,rc,romances,100,0.0,0.0);
    if(dating.result!=DatingProposalResult::Accepted) return fail("dating");
    if(!hasLifeEvent(a.lifeHistory,LifeEventType::DatingStarted) || !hasLifeEvent(b.lifeHistory,LifeEventType::DatingStarted)) return fail("dating history");

    MarriageContext mc; mc.mergeHouseholdsOnMarriage=false;
    auto engagement=applyEngagementProposalTracked(a,b,ab,ba,mc,mc,romances,200,0.0,0.0);
    if(engagement.result!=EngagementProposalResult::Accepted) return fail("engagement");
    if(!hasLifeEvent(a.lifeHistory,LifeEventType::Engaged)) return fail("engagement history");

    HouseholdBook households;
    auto marriage=applyMarriageDecisionTracked(a,b,ab,ba,mc,mc,romances,households,300,0,0,0.0,0.0,0.0);
    if(marriage.result!=MarriageDecisionResult::Married) return fail("marriage");
    if(!hasLifeEvent(a.lifeHistory,LifeEventType::Married) || !hasLifeEvent(b.lifeHistory,LifeEventType::Married)) return fail("marriage history");

    ReproductiveProfile ga; ga.canGestate=true; ga.health=1.0; ga.fertility=1.0; ga.ageYears=25;
    ReproductiveProfile gb; gb.canContributeGenetics=true; gb.health=1.0; gb.fertility=1.0; gb.ageYears=25;
    PregnancyContext pc; pc.gestationalIntent=1.0; pc.partnerIntent=1.0; pc.lifeSituation=1.0; pc.householdCondition=1.0; pc.financialReadiness=1.0;
    PregnancyBook pregnancies;
    auto conception=applyPregnancyAttemptTracked(a,b,ga,gb,ab,ba,pc,pregnancies,400,0.0,0.0);
    if(conception.result!=PregnancyAttemptResult::Conceived) return fail("pregnancy");
    if(!hasLifeEvent(a.lifeHistory,LifeEventType::PregnancyStarted)) return fail("pregnancy history");

    const PregnancyState* p=pregnancies.activeFor(a.id);
    if(p==nullptr) return fail("pregnancy state");
    BirthBook births;
    std::mt19937_64 rng(42);
    auto birth=performBirthTracked(a,b,3,"Child",pregnancies,households,births,rng,p->dueMinute);
    if(birth.result!=BirthResult::Success) return fail("birth");
    if(!hasLifeEvent(a.lifeHistory,LifeEventType::ChildBorn) || !hasLifeEvent(b.lifeHistory,LifeEventType::ChildBorn)) return fail("parent birth history");
    if(!hasLifeEvent(birth.child.lifeHistory,LifeEventType::Birth)) return fail("child birth history");

    RelationshipBook relationshipBook;
    auto& bToA=relationshipBook.getOrCreate(b.id,a.id);
    bToA.affection=bToA.trust=bToA.respect=bToA.comfort=bToA.familiarity=1.0;
    std::vector<Character*> residents={&a,&b};
    auto death=applyDeathTracked(a,1000,DeathCause::AgeRelated,residents,relationshipBook,romances);
    if(!death.died) return fail("death");
    if(!hasLifeEvent(a.lifeHistory,LifeEventType::Death)) return fail("death history");
    if(!hasLifeEvent(b.lifeHistory,LifeEventType::Bereavement)) return fail("bereavement history");
    if(!hasLifeEvent(b.lifeHistory,LifeEventType::PartnerWidowed)) return fail("widow history");

    if(countLifeEvents(a.lifeHistory,LifeEventType::ChildBorn)!=1) return fail("child born count");
    return 0;
}
