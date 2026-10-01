#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

#include "lifelens/Simulation.h"

using namespace lifelens;

namespace {

bool containsLog(const Simulation& sim,const std::string& token)
{
    for(const auto& line:sim.logs()){
        if(line.find(token)!=std::string::npos) return true;
    }
    return false;
}

Character* onlyResident(Simulation& sim)
{
    return sim.world().characters.empty()
        ? nullptr
        : &sim.world().characters.front();
}

void addReachableFood(Simulation& sim,CharacterId id,ResourceNodeId nodeId)
{
    GridPos pos{};
    assert(sim.runtimePosition(id,pos));

    ResourceNode food;
    food.id=nodeId;
    food.material=MaterialKind::PlantFood;
    food.quantity=20;
    food.maxQuantity=20;
    food.pos={pos.x+1,pos.y};
    sim.world().resourceNodes.push_back(food);
}

void addDesignatedSanitationSite(
    Simulation& sim,
    CharacterId actor,
    GridPos pos,
    SanitationSiteId siteId)
{
    PrimitiveSanitationSite site;
    site.id=siteId;
    site.kind=PrimitiveSanitationSiteKind::DesignatedArea;
    site.pos=pos;
    site.establishedBy=actor;
    site.establishedMinute=sim.world().minute;
    site.active=true;
    site.useCount=1;
    sim.world().primitiveSanitationSites.clear();
    sim.world().primitiveSanitationSites.push_back(site);
}

bool waitForMovingPhysical(
    Simulation& sim,
    CharacterId id,
    Goal goal,
    int maxMinutes)
{
    for(int i=0;i<maxMinutes;++i){
        sim.step();
        const auto observed=sim.observeResidentPresentation(id);
        if(observed.active
           && observed.kind==PresentationActionKind::Physical
           && observed.physicalGoal==goal
           && observed.phase==PresentationActionPhase::Moving){
            return true;
        }
    }
    return false;
}

bool waitForInteractingPhysical(
    Simulation& sim,
    CharacterId id,
    Goal goal,
    int maxMinutes)
{
    for(int i=0;i<maxMinutes;++i){
        sim.step();
        const auto observed=sim.observeResidentPresentation(id);
        if(observed.active
           && observed.kind==PresentationActionKind::Physical
           && observed.physicalGoal==goal
           && observed.phase==PresentationActionPhase::Interacting){
            return true;
        }
    }
    return false;
}

}

int main()
{
    SimulationRuleset rules=DefaultSimulationRuleset;
    rules.needs.hungerPerMinute=0.0;
    rules.needs.thirstPerMinute=0.0;
    rules.needs.sleepPerMinute=0.0;
    rules.needs.bladderPerMinute=0.0;
    rules.needs.hygienePerMinute=0.0;

    // Movement commitment: crossing the Critical Hunger band must not cause
    // toilet -> food -> toilet ping-pong when the resident is already travelling
    // to a nearby sanitation site and can finish well before Hunger saturates.
    // The production estimator uses worst-case weather travel cadence, so this
    // scenario deliberately leaves a wide real time budget rather than relying
    // on an arbitrary distance exception.
    SimulationRuleset movingRules=rules;
    movingRules.needs.hungerPerMinute=0.0010;
    Simulation movingToilet(
        874219000,0,CurrentWorldGenerationVersion,movingRules);
    movingToilet.setupNewGame();
    movingToilet.world().characters.resize(1);
    movingToilet.world().resourceNodes.clear();
    movingToilet.world().storageSites.clear();

    Character* movingActor=onlyResident(movingToilet);
    assert(movingActor!=nullptr);
    const CharacterId movingId=movingActor->id;
    movingActor->needs={0.10,0.10,0.10,0.95,0.10};

    GridPos movingStart{};
    assert(movingToilet.runtimePosition(movingId,movingStart));
    addDesignatedSanitationSite(
        movingToilet,
        movingId,
        {movingStart.x+4,movingStart.y},
        990900);
    addReachableFood(movingToilet,movingId,991000);

    assert(waitForMovingPhysical(
        movingToilet,movingId,Goal::UseToilet,40));

    const std::string movingName=movingActor->name;
    movingActor->needs.hunger=CriticalSurvivalPreemptThreshold;

    bool movingToiletCompleted=false;
    bool foodGatherAfterToilet=false;
    for(int i=0;i<120;++i){
        movingToilet.step();
        if(containsLog(
            movingToilet,
            movingName+" completed UseToilet via sanitation site")){
            movingToiletCompleted=true;
        }

        const auto observed=
            movingToilet.observeResidentPresentation(movingId);
        if(observed.active
           && observed.kind==PresentationActionKind::Civilization
           && observed.civilizationIntent==CivilizationIntent::Gather
           && observed.civilizationMaterial==MaterialKind::PlantFood){
            foodGatherAfterToilet=true;
            assert(movingToiletCompleted);
            break;
        }
    }
    assert(movingToiletCompleted);
    assert(foodGatherAfterToilet);

    // Once actual toilet use has started, a newly-critical food need may wait a
    // couple of minutes for the atomic interaction to finish. Movement toward
    // the site is still interruptible; this only protects the real use phase.
    Simulation toilet(874219001,0,CurrentWorldGenerationVersion,rules);
    toilet.setupNewGame();
    toilet.world().characters.resize(1);
    toilet.world().resourceNodes.clear();
    toilet.world().storageSites.clear();

    Character* toiletActor=onlyResident(toilet);
    assert(toiletActor!=nullptr);
    const CharacterId toiletId=toiletActor->id;
    toiletActor->needs={0.10,0.10,0.10,0.95,0.10};
    addReachableFood(toilet,toiletId,991001);

    assert(waitForInteractingPhysical(
        toilet,toiletId,Goal::UseToilet,40));

    const std::string toiletName=toiletActor->name;
    toiletActor->needs.hunger=1.0;

    bool toiletCompleted=false;
    bool foodGatherStarted=false;
    int minutesToToiletCompletion=-1;
    for(int i=0;i<12;++i){
        toilet.step();
        if(!toiletCompleted
           && containsLog(
               toilet,
               toiletName+" completed UseToilet via emergency fallback")){
            toiletCompleted=true;
            minutesToToiletCompletion=i+1;
        }

        const auto observed=toilet.observeResidentPresentation(toiletId);
        if(observed.active
           && observed.kind==PresentationActionKind::Civilization
           && observed.civilizationIntent==CivilizationIntent::Gather
           && observed.civilizationMaterial==MaterialKind::PlantFood){
            foodGatherStarted=true;
            break;
        }
    }
    assert(toiletCompleted);
    assert(minutesToToiletCompletion>0);
    assert(minutesToToiletCompletion<=3);
    assert(foodGatherStarted);

    // Sleep is a long action and therefore remains interruptible, but equal
    // severity must not create one-minute sleep/replan churn. The resident gets
    // real fatigue recovery first; once hunger clearly dominates, survival
    // acquisition takes over.
    Simulation sleep(874219002,0,CurrentWorldGenerationVersion,rules);
    sleep.setupNewGame();
    sleep.world().characters.resize(1);
    sleep.world().resourceNodes.clear();
    sleep.world().storageSites.clear();

    Character* sleeper=onlyResident(sleep);
    assert(sleeper!=nullptr);
    const CharacterId sleepId=sleeper->id;
    sleeper->needs={0.10,0.10,1.0,0.10,0.10};
    addReachableFood(sleep,sleepId,991002);

    assert(waitForInteractingPhysical(
        sleep,sleepId,Goal::Sleep,40));

    const double sleepBeforeCritical=sleeper->needs.sleep;
    sleeper->needs.hunger=1.0;

    bool gatheredAfterRest=false;
    int criticalSleepMinutes=0;
    for(int i=0;i<120;++i){
        sleep.step();
        const auto observed=sleep.observeResidentPresentation(sleepId);
        if(observed.active
           && observed.kind==PresentationActionKind::Physical
           && observed.physicalGoal==Goal::Sleep){
            ++criticalSleepMinutes;
        }
        if(observed.active
           && observed.kind==PresentationActionKind::Civilization
           && observed.civilizationIntent==CivilizationIntent::Gather
           && observed.civilizationMaterial==MaterialKind::PlantFood){
            gatheredAfterRest=true;
            break;
        }
    }

    assert(criticalSleepMinutes>=20);
    assert(sleeper->needs.sleep<sleepBeforeCritical-0.02);
    assert(gatheredAfterRest);

    std::cout<<"action commitment + relative sleep preemption PASS\n";
    return 0;
}
