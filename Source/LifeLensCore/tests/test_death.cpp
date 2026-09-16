#include <iostream>
#include <vector>
#include "lifelens/Death.h"
using namespace lifelens;

static int fail(const char* m){ std::cerr << m << '\n'; return 1; }

int main(){
    Character a,b,c,d;
    a.id=1; a.lifeStage=LifeStage::Elderly; a.childrenIds={3};
    b.id=2; b.lifeStage=LifeStage::Elderly;
    c.id=3; c.lifeStage=LifeStage::Adult; c.parentIds={1};
    d.id=4; d.lifeStage=LifeStage::Adult;

    RelationshipBook rels;
    rels.getOrCreate(2,1);
    rels.getOrCreate(3,1);
    auto& rb=rels.getOrCreate(2,1);
    rb.affection=rb.trust=rb.respect=rb.comfort=rb.familiarity=0.95;
    auto& rc=rels.getOrCreate(3,1);
    rc.affection=rc.trust=rc.respect=rc.comfort=rc.familiarity=0.80;

    RomanceBook romance;
    if(!romance.startDating(1,2,1,10)) return fail("dating");
    if(!romance.engage(1,2,20)) return fail("engage");
    if(!romance.marry(1,2,30)) return fail("marry");

    std::vector<Character*> people={&a,&b,&c,&d};
    auto out=applyDeath(a,1000,DeathCause::AgeRelated,people,rels,romance);
    if(!out.died || a.alive || a.deathMinute!=1000) return fail("state");
    auto pair=romance.findLatest(1,2);
    if(pair==nullptr || pair->stage!=RomanceStage::Widowed) return fail("widow");
    if(b.emotion.grief<=0.0 || c.emotion.grief<=0.0 || d.emotion.grief!=0.0) return fail("grief");
    if(b.memory.entries.empty() || b.memory.entries.back().what!="death") return fail("memory");
    if(b.memory.entries.back().sourceCharacter!=b.id) return fail("memory source");
    if(c.lifeHistory.empty() || c.lifeHistory.back().type!=LifeEventType::Bereavement) return fail("history");
    if(out.bereavedResidents!=2) return fail("bereaved count");
    if(out.population.living!=3 || out.population.deceased!=1 || out.population.livingAdults!=3) return fail("population");
    if(out.population.needsAdultReplacement(2) || !out.population.needsAdultReplacement(4)) return fail("continuity");
    if(applyDeath(a,1100,DeathCause::Other,people,rels,romance).died) return fail("duplicate");

    // v1 does not fabricate unexplained premature natural deaths. Illness,
    // accident, starvation and pathogen mortality are later health-system work.
    Character young;
    young.id=10;
    young.hasBirthMinute=true;
    young.birthMinute=0;
    young.lifeStage=LifeStage::Adult;
    young.lifeCondition.physicalHealth=1.0;
    const int youngMinute=30*LifeMinutesPerYear;
    if(dailyMortalityProbability(young,youngMinute)!=0.0) return fail("young probability");
    if(shouldDieToday(young,12345,youngMinute)) return fail("young mortality");
    const double rollA=deterministicMortalityRoll(12345,young.id,youngMinute);
    const double rollB=deterministicMortalityRoll(12345,young.id,youngMinute);
    if(rollA!=rollB || rollA<0.0 || rollA>=1.0) return fail("deterministic roll");

    Character older=young;
    older.id=13;
    older.lifeStage=LifeStage::MiddleAge;
    const int olderMinute=60*LifeMinutesPerYear;
    if(dailyMortalityProbability(older,olderMinute)<=0.0) return fail("older probability");

    Character oldest=young;
    oldest.id=11;
    oldest.lifeStage=LifeStage::Elderly;
    oldest.lifeCondition.physicalHealth=0.1;
    const int oldestMinute=111*LifeMinutesPerYear;
    if(dailyMortalityProbability(oldest,oldestMinute)!=1.0) return fail("oldest guarantee");
    if(!shouldDieToday(oldest,555,oldestMinute)) return fail("oldest mortality");
    if(inferNaturalDeathCause(oldest,oldestMinute)!=DeathCause::AgeRelated) return fail("oldest cause");

    Character ill=young;
    ill.id=12;
    ill.lifeCondition.physicalHealth=0.10;
    if(inferNaturalDeathCause(ill,youngMinute)!=DeathCause::Illness) return fail("ill cause");
    return 0;
}
