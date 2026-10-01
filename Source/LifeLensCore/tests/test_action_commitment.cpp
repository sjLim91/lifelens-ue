#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

#include "lifelens/Simulation.h"
#include "lifelens/CoreNavigation.h"

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
bool findReachableTargetAtLeastDistance(
    const Simulation& sim,
    CharacterId id,
    int minimumDistance,
    GridPos& outTarget)
{
    GridPos origin{};
    if(!sim.runtimePosition(id,origin)) return false;

    for(int radius=std::max(1,minimumDistance);
        radius<=minimumDistance+24;
        ++radius){
        const GridPos candidates[]={
            {origin.x+radius,origin.y},
            {origin.x-radius,origin.y},
            {origin.x,origin.y+radius},
            {origin.x,origin.y-radius}
        };
        for(const GridPos candidate:candidates){
            if(!coreGroundTraversable(sim.world(),candidate)) continue;
            std::vector<GridPos> route;
            if(buildCoreGroundRoute(
                sim.world(),origin,candidate,0,route)){
                outTarget=candidate;
                return true;
            }
        }
    }
    return false;
}

void installDesignatedSanitationSite(
    Simulation& sim,
    CharacterId actorId,
    GridPos position,
    SanitationSiteId siteId)
{
    sim.world().primitiveSanitationSites.clear();
    PrimitiveSanitationSite site;
    site.id=siteId;
    site.kind=PrimitiveSanitationSiteKind::DesignatedArea;
    site.pos=position;
    site.establishedBy=actorId;
    site.establishedMinute=0;
    site.active=true;
    site.useCount=2;
    sim.world().primitiveSanitationSites.push_back(site);
}

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

    // A moving toilet intent may survive entry into the critical hunger band
    // only when the authoritative path/weather projection proves it will finish
    // before hunger or thirst reaches the hard 1.0 clamp.
    SimulationRuleset movingRules=DefaultSimulationRuleset;
    movingRules.needs.hungerPerMinute=0.002;
    movingRules.needs.thirstPerMinute=0.0;
    movingRules.needs.sleepPerMinute=0.0;
    movingRules.needs.bladderPerMinute=0.0;
    movingRules.needs.hygienePerMinute=0.0;

    Simulation safeMove(
        874219010,0,CurrentWorldGenerationVersion,movingRules);
    safeMove.setupNewGame();
    safeMove.world().characters.resize(1);
    safeMove.world().resourceNodes.clear();
    safeMove.world().storageSites.clear();

    Character* safeActor=onlyResident(safeMove);
    assert(safeActor!=nullptr);
    const CharacterId safeId=safeActor->id;
    const std::string safeName=safeActor->name;
    safeActor->needs={0.10,0.10,0.10,0.95,0.10};
    addReachableFood(safeMove,safeId,991010);

    GridPos nearToilet{};
    assert(findReachableTargetAtLeastDistance(
        safeMove,safeId,3,nearToilet));
    installDesignatedSanitationSite(
        safeMove,safeId,nearToilet,991010);

    safeMove.step();
    auto safePresentation=safeMove.observeResidentPresentation(safeId);
    assert(safePresentation.active);
    assert(safePresentation.kind==PresentationActionKind::Physical);
    assert(safePresentation.physicalGoal==Goal::UseToilet);
    assert(safePresentation.phase==PresentationActionPhase::Moving);

    safeActor->needs.hunger=0.94;
    bool safeToiletCompleted=false;
    for(int minute=0;minute<40 && !safeToiletCompleted;++minute){
        safeMove.step();
        safeToiletCompleted=containsLog(
            safeMove,
            safeName+" completed UseToilet via designated sanitation area");
        if(!safeToiletCompleted){
            assert(!containsLog(
                safeMove,
                safeName+" preempted current activity for critical survival need"));
        }
    }
    assert(safeToiletCompleted);
    assert(safeActor->needs.hunger<1.0);

    bool safeFoodStarted=false;
    for(int minute=0;minute<20 && !safeFoodStarted;++minute){
        safeMove.step();
        const auto observed=safeMove.observeResidentPresentation(safeId);
        safeFoodStarted=
            observed.active
            && observed.kind==PresentationActionKind::Civilization
            && observed.civilizationIntent==CivilizationIntent::Gather
            && observed.civilizationMaterial==MaterialKind::PlantFood;
    }
    assert(safeFoodStarted);

    // The same commitment must not protect a distant sanitation trip when
    // critical hunger would saturate before the authoritative route can finish.
    Simulation unsafeMove(
        874219011,0,CurrentWorldGenerationVersion,movingRules);
    unsafeMove.setupNewGame();
    unsafeMove.world().characters.resize(1);
    unsafeMove.world().resourceNodes.clear();
    unsafeMove.world().storageSites.clear();

    Character* unsafeActor=onlyResident(unsafeMove);
    assert(unsafeActor!=nullptr);
    const CharacterId unsafeId=unsafeActor->id;
    const std::string unsafeName=unsafeActor->name;
    unsafeActor->needs={0.10,0.10,0.10,0.95,0.10};
    addReachableFood(unsafeMove,unsafeId,991011);

    GridPos farToilet{};
    assert(findReachableTargetAtLeastDistance(
        unsafeMove,unsafeId,24,farToilet));
    installDesignatedSanitationSite(
        unsafeMove,unsafeId,farToilet,991011);

    unsafeMove.step();
    auto unsafePresentation=unsafeMove.observeResidentPresentation(unsafeId);
    assert(unsafePresentation.active);
    assert(unsafePresentation.kind==PresentationActionKind::Physical);
    assert(unsafePresentation.physicalGoal==Goal::UseToilet);
    assert(unsafePresentation.phase==PresentationActionPhase::Moving);

    unsafeActor->needs.hunger=0.94;
    bool unsafeFoodStarted=false;
    for(int minute=0;minute<8 && !unsafeFoodStarted;++minute){
        unsafeMove.step();
        const auto observed=unsafeMove.observeResidentPresentation(unsafeId);
        unsafeFoodStarted=
            observed.active
            && observed.kind==PresentationActionKind::Civilization
            && observed.civilizationIntent==CivilizationIntent::Gather
            && observed.civilizationMaterial==MaterialKind::PlantFood;
    }
    assert(containsLog(
        unsafeMove,
        unsafeName+" preempted current activity for critical survival need"));
    assert(unsafeFoodStarted);

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
