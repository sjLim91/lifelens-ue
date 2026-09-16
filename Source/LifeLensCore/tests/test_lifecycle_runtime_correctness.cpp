#include <cmath>
#include <iostream>

#include "lifelens/Simulation.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character* findCharacterById(Simulation& sim,CharacterId id)
{
    for(auto& character:sim.world().characters) if(character.id==id) return &character;
    return nullptr;
}

static Character* findCharacterById(SimulationStateSnapshot& snapshot,CharacterId id)
{
    for(auto& character:snapshot.world.characters) if(character.id==id) return &character;
    return nullptr;
}

static void setStrongBond(Relationship& relation)
{
    relation.affection=0.96;
    relation.trust=0.96;
    relation.respect=0.90;
    relation.comfort=0.96;
    relation.familiarity=0.98;
    relation.attraction=0.96;
    relation.romanticInterest=0.96;
    relation.sexualAttraction=0.92;
    relation.commitment=0.92;
    relation.conflict=0.0;
    relation.fear=0.0;
    relation.grudge=0.0;
}

int main()
{
    // Dead residents are quiescent: no Need decay and no runtime actions continue.
    {
        Simulation sim(10101);
        sim.setupNewGame();
        SimulationStateSnapshot snapshot=sim.captureSnapshot();
        const CharacterId id=snapshot.world.characters.front().id;
        Character* dead=findCharacterById(snapshot,id);
        CHECK(dead!=nullptr);
        dead->alive=false;
        dead->deathMinute=snapshot.world.minute;
        snapshot.runtime[id].goal=Goal::Sleep;
        snapshot.runtime[id].plan={{ActionType::Idle,0,10}};
        const Needs before=dead->needs;
        std::string error;
        CHECK(sim.restoreSnapshot(snapshot,&error));
        sim.runMinutes(20);
        Character* restored=findCharacterById(sim,id);
        CHECK(restored!=nullptr && !restored->alive);
        CHECK(restored->needs.hunger==before.hunger);
        CHECK(restored->needs.thirst==before.thirst);
        CHECK(restored->needs.sleep==before.sleep);
        CHECK(restored->needs.bladder==before.bladder);
        CHECK(restored->needs.hygiene==before.hygiene);
        const SimulationStateSnapshot after=sim.captureSnapshot();
        const auto runtimeIt=after.runtime.find(id);
        CHECK(runtimeIt!=after.runtime.end());
        CHECK(runtimeIt->second.plan.empty());
        CHECK(runtimeIt->second.goal==Goal::Idle);
        CHECK(!runtimeIt->second.socialActive);
    }

    // Babies/Toddlers do not run adult autonomous plans; parent care handles urgent Needs.
    {
        Simulation sim(20202);
        sim.setupNewGame();
        SimulationStateSnapshot snapshot=sim.captureSnapshot();
        const CharacterId parentA=snapshot.world.characters[0].id;
        const CharacterId parentB=snapshot.world.characters[2].id;
        Character child;
        child.id=99;
        child.name="RuntimeBaby";
        child.sex=Sex::Female;
        child.hasBirthMinute=true;
        child.birthMinute=snapshot.world.minute;
        child.lifeStage=LifeStage::Baby;
        child.parentIds={parentA,parentB};
        child.civilization.character=child.id;
        child.needs={0.05,0.05,0.96,0.05,0.05};
        child.development.attachment=0.65;
        child.development.confidence=0.55;
        child.development.socialSkill=0.20;
        child.development.emotionalSecurity=0.60;
        child.development.health=1.0;
        applyLifeStageProfile(child,LifeStage::Baby);
        snapshot.world.characters.push_back(child);
        SimulationRuntimeSnapshot childRuntime;
        childRuntime.pos=snapshot.runtime[parentB].pos;
        snapshot.runtime.emplace(child.id,childRuntime);
        snapshot.genealogy.registerBirth(child.id,parentA,parentB);
        setStrongBond(snapshot.relationships.getOrCreate(parentA,child.id));
        setStrongBond(snapshot.relationships.getOrCreate(child.id,parentA));
        setStrongBond(snapshot.relationships.getOrCreate(parentB,child.id));
        setStrongBond(snapshot.relationships.getOrCreate(child.id,parentB));

        std::string error;
        CHECK(sim.restoreSnapshot(snapshot,&error));
        sim.step();
        Character* cared=findCharacterById(sim,child.id);
        CHECK(cared!=nullptr);
        CHECK(cared->needs.sleep<0.50);
        const SimulationStateSnapshot after=sim.captureSnapshot();
        CHECK(after.runtime.at(child.id).plan.empty());
        CHECK(after.runtime.at(child.id).goal==Goal::Idle);
    }

    // Parent/child genealogy cannot enter autonomous romance even with extreme chemistry.
    {
        Simulation sim(30303);
        sim.setupNewGame();
        SimulationStateSnapshot snapshot=sim.captureSnapshot();
        CharacterId parent=snapshot.world.characters[0].id;
        CharacterId child=snapshot.world.characters[2].id;
        CharacterId otherParent=snapshot.world.characters[1].id;
        snapshot.genealogy.registerBirth(child,parent,otherParent);
        for(auto& resident:snapshot.world.characters){
            if(resident.id!=parent && resident.id!=child){
                resident.alive=false;
                resident.deathMinute=snapshot.world.minute;
            }
        }
        Character* parentCharacter=findCharacterById(snapshot,parent);
        Character* childCharacter=findCharacterById(snapshot,child);
        CHECK(parentCharacter!=nullptr && childCharacter!=nullptr);
        parentCharacter->lifeCondition.lifeGoalFamilyFocus=1.0;
        childCharacter->lifeCondition.lifeGoalFamilyFocus=1.0;
        setStrongBond(snapshot.relationships.getOrCreate(parent,child));
        setStrongBond(snapshot.relationships.getOrCreate(child,parent));
        snapshot.world.minute=FamilyProgressionDecisionMinuteOfDay-1;
        std::string error;
        CHECK(sim.restoreSnapshot(snapshot,&error));
        sim.step();
        CHECK(sim.romances().findActivePair(parent,child)==nullptr);
        CHECK(isRomanceProhibitedKinship(sim.genealogy().relationBetween(parent,child)));
    }

    // A birth inherits the gestational parent's authoritative runtime position.
    {
        Simulation sim(40404);
        sim.setupNewGame();
        SimulationStateSnapshot snapshot=sim.captureSnapshot();
        const CharacterId male=snapshot.world.characters[0].id;
        const CharacterId female=snapshot.world.characters[2].id;
        const GridPos expectedPosition=snapshot.runtime[female].pos;
        snapshot.world.minute=PregnancyGestationMinutes-1;
        CHECK(snapshot.households.create(700,{male,female}));
        CHECK(snapshot.pregnancies.start(female,male,0)!=nullptr);
        std::string error;
        CHECK(sim.restoreSnapshot(snapshot,&error));
        sim.step();
        CHECK(sim.world().characters.size()==5);
        const BirthRecord& birth=sim.births().all().back();
        GridPos childPosition{};
        CHECK(sim.runtimePosition(birth.childId,childPosition));
        CHECK(childPosition.x==expectedPosition.x && childPosition.y==expectedPosition.y);
        Character* child=findCharacterById(sim,birth.childId);
        CHECK(child!=nullptr);
        CHECK(child->lifeStage==LifeStage::Baby);
        CHECK(std::abs(child->baseMetabolism-1.0)<1e-12);
        CHECK(std::abs(child->baseSleepTendency-1.0)<1e-12);
        CHECK(std::abs(child->metabolism-lifeStageProfile(LifeStage::Baby).metabolismMultiplier)<1e-12);
    }

    // Daily natural death closes runtime, active gestational pregnancy and household membership.
    {
        Simulation sim(50505);
        sim.setupNewGame();
        SimulationStateSnapshot snapshot=sim.captureSnapshot();
        const CharacterId survivorId=snapshot.world.characters[0].id;
        const CharacterId deceasedId=snapshot.world.characters[2].id;
        Character* deceased=findCharacterById(snapshot,deceasedId);
        CHECK(deceased!=nullptr);
        snapshot.world.minute=FamilyProgressionDecisionMinuteOfDay-1;
        deceased->hasBirthMinute=true;
        deceased->birthMinute=snapshot.world.minute-111*LifeMinutesPerYear;
        deceased->lifeStage=LifeStage::Elderly;
        deceased->lifeCondition.physicalHealth=0.10;
        CHECK(snapshot.households.create(800,{survivorId,deceasedId}));
        CHECK(snapshot.pregnancies.start(deceasedId,survivorId,0)!=nullptr);
        snapshot.runtime[deceasedId].goal=Goal::Sleep;
        snapshot.runtime[deceasedId].plan={{ActionType::Idle,0,10}};

        std::string error;
        CHECK(sim.restoreSnapshot(snapshot,&error));
        sim.step();
        Character* dead=findCharacterById(sim,deceasedId);
        CHECK(dead!=nullptr && !dead->alive);
        CHECK(dead->deathMinute==FamilyProgressionDecisionMinuteOfDay);
        CHECK(sim.pregnancies().activeFor(deceasedId)==nullptr);
        CHECK(sim.households().householdOf(deceasedId)==nullptr);
        const Household* survivorHome=sim.households().householdOf(survivorId);
        CHECK(survivorHome!=nullptr && survivorHome->contains(survivorId));
        const SimulationStateSnapshot afterDeath=sim.captureSnapshot();
        CHECK(afterDeath.runtime.at(deceasedId).plan.empty());
        const Needs frozen=dead->needs;
        sim.runMinutes(20);
        CHECK(dead->needs.hunger==frozen.hunger);
        CHECK(dead->needs.thirst==frozen.thirst);
        CHECK(dead->needs.sleep==frozen.sleep);
        CHECK(dead->needs.bladder==frozen.bladder);
        CHECK(dead->needs.hygiene==frozen.hygiene);
    }

    return 0;
}
