#include <iostream>
#include <sstream>
#include <string>

#include "lifelens/Simulation.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return false; \
    } \
} while(false)

static bool sameMemory(const MemoryRecord& a,const MemoryRecord& b)
{
    return a.who==b.who && a.sourceCharacter==b.sourceCharacter && a.what==b.what && a.where==b.where &&
        a.minute==b.minute && a.emotionValence==b.emotionValence && a.emotionIntensity==b.emotionIntensity &&
        a.importance==b.importance && a.confidence==b.confidence && a.witnessed==b.witnessed &&
        a.source==b.source && a.decayPerDay==b.decayPerDay && a.tags==b.tags;
}

static bool sameBelief(const BeliefRecord& a,const BeliefRecord& b)
{
    return a.subject==b.subject && a.proposition==b.proposition && a.stance==b.stance &&
        a.confidence==b.confidence && a.supportWeight==b.supportWeight &&
        a.contradictionWeight==b.contradictionWeight && a.supportCount==b.supportCount &&
        a.contradictionCount==b.contradictionCount && a.lastUpdatedMinute==b.lastUpdatedMinute;
}

static bool sameCharacter(const Character& a,const Character& b)
{
    if(a.id!=b.id || a.name!=b.name || a.sex!=b.sex) return false;
    if(a.needs.hunger!=b.needs.hunger || a.needs.thirst!=b.needs.thirst || a.needs.sleep!=b.needs.sleep ||
       a.needs.bladder!=b.needs.bladder || a.needs.hygiene!=b.needs.hygiene) return false;
#define P(field) if(a.personality.field!=b.personality.field) return false
    P(introversion); P(conscientiousness); P(openness); P(agreeableness); P(emotionalStability); P(empathy);
    P(impulsiveness); P(riskTolerance); P(ambition); P(patience); P(sociability); P(curiosity); P(orderliness); P(adaptability);
#undef P
#define E(field) if(a.emotion.field!=b.emotion.field) return false
    E(joy); E(sadness); E(anger); E(fear); E(embarrassment); E(pride); E(jealousy); E(affection); E(anxiety); E(relief); E(grief); E(valence); E(arousal);
#undef E
    if(a.memory.entries.size()!=b.memory.entries.size()) return false;
    for(std::size_t i=0;i<a.memory.entries.size();++i) if(!sameMemory(a.memory.entries[i],b.memory.entries[i])) return false;
    if(a.beliefs.beliefs.size()!=b.beliefs.beliefs.size()) return false;
    for(std::size_t i=0;i<a.beliefs.beliefs.size();++i) if(!sameBelief(a.beliefs.beliefs[i],b.beliefs.beliefs[i])) return false;
#define G(field) if(a.genetics.field!=b.genetics.field) return false
    G(faceShape); G(eyePigment); G(hairPigment); G(skinTone); G(heightPotential); G(buildPotential); G(healthPotential); G(learningPotential); G(temperamentSensitivity);
#undef G
#define D(field) if(a.development.field!=b.development.field) return false
    D(attachment); D(confidence); D(stress); D(socialSkill); D(emotionalSecurity); D(disciplineInternalization); D(learningSupport); D(health);
#undef D
#define L(field) if(a.lifeCondition.field!=b.lifeCondition.field) return false
    L(physicalHealth); L(energyCapacity); L(movementCapacity); L(reproductivePotential); L(workCapacity); L(appearanceAgeFactor); L(lifeGoalFamilyFocus); L(familyRoleSalience);
#undef L
    if(a.parentIds!=b.parentIds || a.childrenIds!=b.childrenIds || a.lifeHistory.size()!=b.lifeHistory.size()) return false;
    for(std::size_t i=0;i<a.lifeHistory.size();++i){
        const auto& x=a.lifeHistory[i]; const auto& y=b.lifeHistory[i];
        if(x.type!=y.type || x.minute!=y.minute || x.relatedCharacters!=y.relatedCharacters || x.value!=y.value) return false;
    }
    return a.hasBirthMinute==b.hasBirthMinute && a.birthMinute==b.birthMinute && a.lifeStage==b.lifeStage &&
        a.alive==b.alive && a.deathMinute==b.deathMinute && a.baseMetabolism==b.baseMetabolism &&
        a.baseSleepTendency==b.baseSleepTendency && a.metabolism==b.metabolism && a.sleepTendency==b.sleepTendency;
}

static bool sameSnapshot(const SimulationStateSnapshot& a,const SimulationStateSnapshot& b)
{
    CHECK(a.version==b.version);
    CHECK(a.world.minute==b.world.minute && a.world.seed==b.world.seed);
    std::ostringstream arng,brng; arng<<a.world.rng; brng<<b.world.rng; CHECK(arng.str()==brng.str());
    CHECK(a.world.characters.size()==b.world.characters.size());
    for(std::size_t i=0;i<a.world.characters.size();++i) CHECK(sameCharacter(a.world.characters[i],b.world.characters[i]));
    CHECK(a.world.objects.size()==b.world.objects.size());
    for(std::size_t i=0;i<a.world.objects.size();++i){
        const auto& x=a.world.objects[i]; const auto& y=b.world.objects[i];
        CHECK(x.id==y.id && x.kind==y.kind && x.pos.x==y.pos.x && x.pos.y==y.pos.y && x.reservedBy==y.reservedBy);
        CHECK(x.effectPerTick.hunger==y.effectPerTick.hunger && x.effectPerTick.thirst==y.effectPerTick.thirst &&
              x.effectPerTick.sleep==y.effectPerTick.sleep && x.effectPerTick.bladder==y.effectPerTick.bladder &&
              x.effectPerTick.hygiene==y.effectPerTick.hygiene && x.useDurationTicks==y.useDurationTicks);
    }
    CHECK(a.relationships.all().size()==b.relationships.all().size());
    for(std::size_t i=0;i<a.relationships.all().size();++i){
        const auto& x=a.relationships.all()[i]; const auto& y=b.relationships.all()[i];
        CHECK(x.from==y.from && x.to==y.to && x.affection==y.affection && x.trust==y.trust && x.respect==y.respect &&
              x.comfort==y.comfort && x.familiarity==y.familiarity && x.attraction==y.attraction &&
              x.romanticInterest==y.romanticInterest && x.sexualAttraction==y.sexualAttraction && x.commitment==y.commitment &&
              x.conflict==y.conflict && x.jealousy==y.jealousy && x.fear==y.fear && x.grudge==y.grudge);
    }
    CHECK(a.genealogy.all().size()==b.genealogy.all().size());
    for(std::size_t i=0;i<a.genealogy.all().size();++i){
        const auto& x=a.genealogy.all()[i]; const auto& y=b.genealogy.all()[i];
        CHECK(x.characterId==y.characterId && x.parents==y.parents && x.children==y.children && x.spouses==y.spouses);
    }
    CHECK(a.romances.all().size()==b.romances.all().size());
    for(std::size_t i=0;i<a.romances.all().size();++i){
        const auto& x=a.romances.all()[i]; const auto& y=b.romances.all()[i];
        CHECK(x.first==y.first && x.second==y.second && x.initiator==y.initiator && x.stage==y.stage &&
              x.startedMinute==y.startedMinute && x.engagedMinute==y.engagedMinute && x.marriedMinute==y.marriedMinute && x.endedMinute==y.endedMinute);
    }
    CHECK(a.households.all().size()==b.households.all().size());
    for(std::size_t i=0;i<a.households.all().size();++i){
        const auto& x=a.households.all()[i]; const auto& y=b.households.all()[i];
        CHECK(x.id==y.id && x.home==y.home && x.resources==y.resources && x.sharedMoney==y.sharedMoney && x.sharedObjects==y.sharedObjects);
        CHECK(x.members.size()==y.members.size());
        for(std::size_t j=0;j<x.members.size();++j){
            const auto& m=x.members[j]; const auto& n=y.members[j];
            CHECK(m.characterId==n.characterId && m.contributionWeight==n.contributionWeight);
            CHECK(m.responsibilities.cooking==n.responsibilities.cooking && m.responsibilities.cleaning==n.responsibilities.cleaning &&
                  m.responsibilities.shopping==n.responsibilities.shopping && m.responsibilities.maintenance==n.responsibilities.maintenance &&
                  m.responsibilities.caregiving==n.responsibilities.caregiving);
        }
    }
    CHECK(a.pregnancies.all().size()==b.pregnancies.all().size());
    for(std::size_t i=0;i<a.pregnancies.all().size();++i){
        const auto& x=a.pregnancies.all()[i]; const auto& y=b.pregnancies.all()[i];
        CHECK(x.gestationalParent==y.gestationalParent && x.geneticPartner==y.geneticPartner && x.conceptionMinute==y.conceptionMinute &&
              x.dueMinute==y.dueMinute && x.lastUpdateMinute==y.lastUpdateMinute && x.stage==y.stage && x.health==y.health &&
              x.fatigue==y.fatigue && x.stress==y.stress && x.nutrition==y.nutrition);
    }
    CHECK(a.births.all().size()==b.births.all().size());
    for(std::size_t i=0;i<a.births.all().size();++i){
        const auto& x=a.births.all()[i]; const auto& y=b.births.all()[i];
        CHECK(x.childId==y.childId && x.parentA==y.parentA && x.parentB==y.parentB && x.birthMinute==y.birthMinute && x.householdId==y.householdId);
    }
    CHECK(a.runtime.size()==b.runtime.size());
    for(const auto& item:a.runtime){
        const auto it=b.runtime.find(item.first); CHECK(it!=b.runtime.end());
        const auto& x=item.second; const auto& y=it->second;
        CHECK(x.goal==y.goal && x.actionIndex==y.actionIndex && x.pos.x==y.pos.x && x.pos.y==y.pos.y && x.announced==y.announced &&
              x.lastGoal==y.lastGoal && x.repeatCount==y.repeatCount && x.consecutiveFailures==y.consecutiveFailures &&
              x.penaltyUntilMinute==y.penaltyUntilMinute && x.socialCooldownUntilMinute==y.socialCooldownUntilMinute &&
              x.socialActive==y.socialActive && x.socialIntent==y.socialIntent && x.socialTarget==y.socialTarget && x.plan.size()==y.plan.size());
        for(std::size_t i=0;i<x.plan.size();++i) CHECK(x.plan[i].type==y.plan[i].type && x.plan[i].objectId==y.plan[i].objectId && x.plan[i].remainingTicks==y.plan[i].remainingTicks);
    }
    CHECK(a.logs==b.logs);
    return true;
}

int main()
{
    Simulation source(424242);
    source.setupNewGame();
    source.runMinutes(37);

    Character& first=source.world().characters[0];
    Character& second=source.world().characters[1];
    Character& third=source.world().characters[2];
    first.needs={0.91,0.82,0.73,0.64,0.55};
    first.personality.curiosity=0.987;
    first.emotion.joy=0.44; first.emotion.grief=0.13; first.emotion.refreshSummary();
    MemoryRecord memory; memory.who=third.id; memory.sourceCharacter=second.id; memory.what="save-load-memory"; memory.where="home";
    memory.minute=source.world().minute-10; memory.emotionValence=0.42; memory.emotionIntensity=0.77; memory.importance=0.91;
    memory.confidence=0.83; memory.witnessed=false; memory.source=MemorySource::ToldByOther; memory.decayPerDay=0.015; memory.tags={"family","test"};
    first.memory.add(memory);
    BeliefRecord& belief=first.beliefs.getOrCreate(third.id,"reliable"); belief.applyEvidence(true,0.77,source.world().minute);
    first.genetics.eyePigment=0.123; first.development.attachment=0.876; first.lifeCondition.lifeGoalFamilyFocus=0.934;
    recordLifeEvent(first.lifeHistory,LifeEventType::HouseholdChanged,source.world().minute,{third.id},77);

    Relationship& rel=source.relationships().getOrCreate(first.id,third.id);
    rel.affection=0.81; rel.trust=0.72; rel.respect=0.63; rel.comfort=0.54; rel.familiarity=0.45; rel.attraction=0.76;
    rel.romanticInterest=0.67; rel.sexualAttraction=0.58; rel.commitment=0.49; rel.conflict=0.14; rel.jealousy=0.23; rel.fear=0.05; rel.grudge=0.09;

    CHECK(source.genealogy().linkSpouses(first.id,third.id));
    CHECK(source.romances().startDating(first.id,third.id,first.id,source.world().minute-200));
    CHECK(source.romances().engage(first.id,third.id,source.world().minute-100));
    CHECK(source.households().create(77,{first.id,third.id},9,1234.5));
    Household* home=source.households().find(77); CHECK(home!=nullptr); home->resources=88.5; home->sharedObjects={9,10}; home->members[0].responsibilities.cooking=0.8;
    PregnancyState* pregnancy=source.pregnancies().start(third.id,first.id,source.world().minute-50); CHECK(pregnancy!=nullptr);
    pregnancy->health=0.93; pregnancy->fatigue=0.31; pregnancy->stress=0.22; pregnancy->nutrition=0.79;
    CHECK(source.births().add(BirthRecord{second.id,first.id,third.id,source.world().minute-500,77}));

    const SimulationStateSnapshot saved=source.captureSnapshot();
    CHECK(saved.version==SimulationSnapshotVersion);
    CHECK(saved.runtime.size()==saved.world.characters.size());

    Simulation restored(999999);
    std::string error;
    CHECK(restored.restoreSnapshot(saved,&error));
    CHECK(error.empty());
    CHECK(sameSnapshot(saved,restored.captureSnapshot()));

    // Bad versions are rejected before mutating the destination.
    Simulation untouched(7); untouched.setupNewGame();
    const SimulationStateSnapshot beforeBad=untouched.captureSnapshot();
    SimulationStateSnapshot bad=saved; bad.version=SimulationSnapshotVersion+1;
    CHECK(!untouched.restoreSnapshot(bad,&error));
    CHECK(!error.empty());
    CHECK(sameSnapshot(beforeBad,untouched.captureSnapshot()));

    // Exact RNG + runtime restoration must produce the same future, not merely
    // the same immediate observer projection.
    source.runMinutes(10000);
    restored.runMinutes(10000);
    CHECK(sameSnapshot(source.captureSnapshot(),restored.captureSnapshot()));

    std::cout << "core save/load snapshot roundtrip + deterministic continuation passed\n";
    return 0;
}
