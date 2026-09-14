#include <cassert>
#include <set>
#include <string>

#include "lifelens/Simulation.h"

using namespace lifelens;

namespace {

bool sameFounder(const Character& a,const Character& b)
{
    return a.id==b.id
        && a.name==b.name
        && a.sex==b.sex
        && a.birthMinute==b.birthMinute
        && a.lifeStage==b.lifeStage
        && a.needs.hunger==b.needs.hunger
        && a.needs.thirst==b.needs.thirst
        && a.needs.sleep==b.needs.sleep
        && a.needs.bladder==b.needs.bladder
        && a.needs.hygiene==b.needs.hygiene
        && a.personality.introversion==b.personality.introversion
        && a.personality.sociability==b.personality.sociability
        && a.personality.empathy==b.personality.empathy
        && a.personality.ambition==b.personality.ambition
        && a.genetics.faceShape==b.genetics.faceShape
        && a.genetics.eyePigment==b.genetics.eyePigment
        && a.genetics.heightPotential==b.genetics.heightPotential
        && a.genetics.healthPotential==b.genetics.healthPotential;
}

} // namespace

int main()
{
    Simulation first(874213954);
    first.setupNewGame();

    const World& world=first.world();
    assert(world.seed==874213954);
    assert(world.minute==8*60);
    assert(world.characters.size()==4);

    int males=0;
    int females=0;
    std::set<CharacterId> ids;
    std::set<std::string> names;
    for(const Character& founder:world.characters){
        ids.insert(founder.id);
        names.insert(founder.name);
        if(founder.sex==Sex::Male) ++males;
        if(founder.sex==Sex::Female) ++females;

        assert(founder.alive);
        assert(founder.hasBirthMinute);
        assert(founder.parentIds.empty());
        assert(founder.childrenIds.empty());
        const int age=ageYearsFromMinutes(founder.birthMinute,world.minute);
        assert(age>=25 && age<=34);
        assert(founder.lifeStage==LifeStage::Adult);
        assert(founder.lifeCondition.reproductivePotential>0.0);
        assert(founder.needs.hunger>=0.06 && founder.needs.hunger<=0.38);
        assert(founder.needs.thirst>=0.06 && founder.needs.thirst<=0.38);
    }
    assert(males==2);
    assert(females==2);
    assert(ids.size()==4);
    assert(names.size()==4);

    // Every direction exists, but no couple/family state is forced at birth of
    // the world. Founders begin as strangers / very low familiarity.
    assert(first.relationships().size()==12);
    for(const Relationship& relation:first.relationships().all()){
        assert(relation.from!=relation.to);
        assert(relation.familiarity>=0.0 && relation.familiarity<=0.04);
        assert(relation.affection==0.0);
        assert(relation.trust==0.0);
        assert(relation.romanticInterest==0.0);
        assert(relation.sexualAttraction==0.0);
        assert(relation.commitment==0.0);
    }

    const WorldOverviewObservation overview=first.observeWorldOverview();
    assert(overview.totalResidents==4);
    assert(overview.livingResidents==4);
    assert(overview.households==0);
    assert(overview.activeCouples==0);
    assert(overview.activePregnancies==0);

    // Same WorldSeed must reproduce the exact founder set and initial state.
    Simulation sameSeed(874213954);
    sameSeed.setupNewGame();
    assert(sameSeed.world().characters.size()==world.characters.size());
    for(std::size_t i=0;i<world.characters.size();++i){
        assert(sameFounder(world.characters[i],sameSeed.world().characters[i]));
    }
    assert(first.logs()==sameSeed.logs());

    // Different New Game seeds must be able to create a different society.
    Simulation differentSeed(874213955);
    differentSeed.setupNewGame();
    bool differs=false;
    for(std::size_t i=0;i<world.characters.size();++i){
        if(!sameFounder(world.characters[i],differentSeed.world().characters[i])){
            differs=true;
            break;
        }
    }
    assert(differs);

    // Re-running setup on the same simulation is deterministic because formal
    // New Game resets RNG state to WorldSeed rather than inheriting old draws.
    first.runMinutes(15);
    first.setupNewGame();
    for(std::size_t i=0;i<world.characters.size();++i){
        assert(sameFounder(first.world().characters[i],sameSeed.world().characters[i]));
    }

    return 0;
}
