#include <cassert>
#include <cstdint>
#include <string>

#include "lifelens/FamilyProgression.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

int main()
{
    constexpr std::uint64_t seed=616161;
    Simulation sim(seed);
    sim.setupNewGame();
    SimulationStateSnapshot snapshot=sim.captureSnapshot();
    assert(snapshot.world.characters.size()>=2);

    const CharacterId firstParent=snapshot.world.characters[0].id;
    const CharacterId secondParent=snapshot.world.characters[1].id;
    for(Character& resident:snapshot.world.characters){
        resident.alive=false;
    }

    DailyHealthInputs severe;
    severe.hunger01=1.0;
    severe.thirst01=1.0;
    severe.sleep01=0.5;
    severe.hygiene01=0.5;
    severe.geneticHealthPotential01=0.5;
    severe.baselinePhysicalHealth01=1.0;
    severe.directCareDependency01=1.0;
    const double chance=deprivationFatalChance(severe);
    assert(chance>0.0);

    // Daily family/health progression runs at minute 720. Choose a deterministic
    // child id whose day-0 deprivation roll is inside the fatal band so this
    // integration regression needs only one simulation step.
    CharacterId childId=0;
    for(CharacterId id=1000;id<100000;++id){
        if(deterministicHealthRoll(
               seed,id,0,0x4845414c54484450ULL)<chance){
            childId=id;
            break;
        }
    }
    assert(childId!=0);

    Character child;
    child.id=childId;
    child.name="UncaredBaby";
    child.sex=Sex::Female;
    child.alive=true;
    child.hasBirthMinute=true;
    child.birthMinute=0;
    child.lifeStage=LifeStage::Baby;
    child.parentIds={firstParent,secondParent};
    child.civilization.character=child.id;
    child.needs={1.0,1.0,0.5,0.5,0.5};
    child.lifeCondition.physicalHealth=1.0;
    child.genetics.healthPotential=0.5;
    applyLifeStageProfile(child,LifeStage::Baby);
    snapshot.world.characters.push_back(child);

    SimulationRuntimeSnapshot childRuntime;
    childRuntime.pos=snapshot.world.initialStartRegionCenterGrid();
    snapshot.runtime.emplace(child.id,childRuntime);
    assert(snapshot.genealogy.registerBirth(
        child.id,firstParent,secondParent));

    snapshot.world.minute=FamilyProgressionDecisionMinuteOfDay-1;

    std::string error;
    assert(sim.restoreSnapshot(snapshot,&error));
    assert(error.empty());

    sim.step();
    assert(sim.world().minute==FamilyProgressionDecisionMinuteOfDay);

    Character* liveChild=nullptr;
    for(Character& resident:sim.world().characters){
        if(resident.id==childId){
            liveChild=&resident;
            break;
        }
    }
    assert(liveChild!=nullptr);
    assert(!liveChild->alive);
    assert(!liveChild->lifeHistory.empty());
    const LifeHistoryEntry& death=liveChild->lifeHistory.back();
    assert(death.type==LifeEventType::Death);
    assert(death.value==static_cast<int>(DeathCause::Deprivation));

    return 0;
}
