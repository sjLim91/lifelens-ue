#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/SimulationCalendar.h"

using namespace lifelens;

namespace {

bool containsLog(const std::vector<std::string>& logs,const std::string& token)
{
    for(const auto& line:logs){
        if(line.find(token)!=std::string::npos) return true;
    }
    return false;
}

bool hasResource(const World& world,MaterialKind material)
{
    for(const auto& node:world.resourceNodes){
        if(node.material==material && node.quantity>0) return true;
    }
    return false;
}

int resourceUnits(const World& world,MaterialKind material)
{
    int total=0;
    for(const auto& node:world.resourceNodes){
        if(node.material==material) total+=std::max(0,node.quantity);
    }
    return total;
}

int countLogs(const std::vector<std::string>& logs,const std::string& token)
{
    int count=0;
    for(const auto& line:logs){
        if(line.find(token)!=std::string::npos) ++count;
    }
    return count;
}

const Character* findResident(const World& world,CharacterId id)
{
    for(const auto& resident:world.characters){
        if(resident.id==id) return &resident;
    }
    return nullptr;
}

} // namespace

int main()
{
    // C1-D durable subsistence: perishable PlantFood ages at the authoritative
    // daily boundary. Organized storage slows spoilage but never freezes food.
    SimulationRuleset spoilRules=DefaultSimulationRuleset;
    spoilRules.needs.hungerPerMinute=0.0;
    spoilRules.needs.thirstPerMinute=0.0;
    spoilRules.needs.sleepPerMinute=0.0;
    spoilRules.needs.bladderPerMinute=0.0;
    spoilRules.needs.hygienePerMinute=0.0;
    Simulation spoilage(
        4241999,0,CurrentWorldGenerationVersion,spoilRules);
    spoilage.setupNewGame();
    // Preserve normal world invariants so snapshot validation remains valid.
    // Spoilage itself is isolated below by invoking only Inventory aging.
    spoilage.world().storageSites.clear();
    Character& foodOwner=spoilage.world().characters.front();
    foodOwner.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,1,0.50,1.0});
    StorageSite foodStore;
    foodStore.id=99001;
    foodStore.pos=spoilage.world().initialStartRegionCenterGrid();
    foodStore.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,1,0.50,1.0});
    spoilage.world().storageSites.push_back(foodStore);

    // Isolate spoilage authority from autonomous Eat/Retrieve decisions.
    // Simulation::step invokes these exact Inventory transitions at the daily
    // boundary; the regression should test freshness, not AI consumption.
    for(int day=0;day<2;++day){
        foodOwner.civilization.inventory.agePlantFoodOneDay(0.085);
        spoilage.world().storageSites[0].inventory.agePlantFoodOneDay(0.045);
    }
    assert(foodOwner.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==1);
    assert(spoilage.world().storageSites[0].inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==1);
    assert(foodOwner.civilization.inventory.averagePlantFoodFreshness()
        <spoilage.world().storageSites[0].inventory.averagePlantFoodFreshness());

    std::vector<std::uint8_t> spoilBytes;
    std::string spoilError;
    assert(encodeSimulationSnapshot(
        spoilage.captureSnapshot(),spoilBytes,&spoilError));
    SimulationStateSnapshot spoilDecoded;
    assert(decodeSimulationSnapshot(
        spoilBytes,spoilDecoded,&spoilError));
    assert(spoilDecoded.world.characters.front().civilization.inventory
        .averagePlantFoodFreshness()
        ==foodOwner.civilization.inventory.averagePlantFoodFreshness());
    assert(spoilDecoded.world.storageSites[0].inventory
        .averagePlantFoodFreshness()
        ==spoilage.world().storageSites[0].inventory.averagePlantFoodFreshness());

    for(int day=0;day<3;++day){
        foodOwner.civilization.inventory.agePlantFoodOneDay(0.085);
        spoilage.world().storageSites[0].inventory.agePlantFoodOneDay(0.045);
    }
    assert(foodOwner.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==0);
    assert(spoilage.world().storageSites[0].inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==1);

    // Legacy SmartObjects are interaction locations, never free provisions.
    // This also locks future well/sink presentation against "walk there and
    // magically drink" regressions.
    SimulationRuleset provisionRules=DefaultSimulationRuleset;
    provisionRules.needs.hungerPerMinute=0.0;
    provisionRules.needs.thirstPerMinute=0.0;
    provisionRules.needs.sleepPerMinute=0.0;
    provisionRules.needs.bladderPerMinute=0.0;
    provisionRules.needs.hygienePerMinute=0.0;
    Simulation provisionProbe(
        9123401,0,CurrentWorldGenerationVersion,provisionRules);
    provisionProbe.setupDemo();
    provisionProbe.world().resourceNodes.clear();
    provisionProbe.world().storageSites.clear();
    Character& provisionActor=provisionProbe.world().characters.front();
    provisionActor.needs={0.01,0.90,0.01,0.01,0.01};
    while(provisionActor.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,1)) {}
    while(provisionActor.civilization.inventory.remove(
        ItemKind::SimpleContainer,MaterialKind::Unknown,1,true)) {}

    assert(portableWaterCount(provisionActor.civilization.inventory)==0);
    assert(!actionAvailableFor(
        provisionProbe.world(),provisionActor,Goal::Drink));
    assert(buildPlan(
        provisionProbe.world(),provisionActor,Goal::Drink,{}).empty());

    // A loose Water stack without a physical container is not a portable
    // provision and must not enable Drink/Wash.
    provisionActor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Water,1,0.5,1.0});
    assert(rawWaterUnitCount(provisionActor.civilization.inventory)==1);
    assert(portableWaterCount(provisionActor.civilization.inventory)==0);
    assert(!actionAvailableFor(
        provisionProbe.world(),provisionActor,Goal::Drink));
    assert(!actionAvailableFor(
        provisionProbe.world(),provisionActor,Goal::Wash));

    // Once a real container exists, the same Water becomes portable. Drinking
    // consumes Water while leaving the now-empty container behind.
    provisionActor.civilization.inventory.add({
        ItemKind::SimpleContainer,MaterialKind::Clay,1,0.5,1.0});
    assert(portableWaterCount(provisionActor.civilization.inventory)==1);
    assert(actionAvailableFor(
        provisionProbe.world(),provisionActor,Goal::Drink));

    const double thirstBeforeDrink=provisionActor.needs.thirst;
    for(int minute=0;
        minute<90
        && portableWaterCount(provisionActor.civilization.inventory)>0;
        ++minute){
        provisionProbe.step();
    }
    assert(portableWaterCount(provisionActor.civilization.inventory)==0);
    assert(simpleContainerCount(provisionActor.civilization.inventory)==1);
    assert(provisionActor.needs.thirst<thirstBeforeDrink);

    // Refill the existing empty container and verify portable washing consumes
    // only the Water, not the container.
    provisionActor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Water,1,0.5,1.0});
    provisionActor.needs={0.01,0.01,0.01,0.01,0.90};
    const double hygieneBeforeWash=provisionActor.needs.hygiene;
    for(int minute=0;
        minute<90
        && portableWaterCount(provisionActor.civilization.inventory)>0;
        ++minute){
        provisionProbe.step();
    }
    assert(portableWaterCount(provisionActor.civilization.inventory)==0);
    assert(simpleContainerCount(provisionActor.civilization.inventory)==1);
    assert(provisionActor.needs.hygiene<hygieneBeforeWash);

    // Pre-container survival uses the source directly. No Water or container is
    // fabricated into inventory merely because the resident drinks or washes.
    Simulation directWater(9123402);
    directWater.setupNewGame();
    directWater.world().characters.resize(1);
    Character& directActor=directWater.world().characters.front();
    const CharacterId directActorId=directActor.id;
    while(directActor.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,1)) {}
    while(directActor.civilization.inventory.remove(
        ItemKind::SimpleContainer,MaterialKind::Unknown,1,true)) {}

    GridPos directStart{};
    assert(directWater.runtimePosition(directActorId,directStart));
    assert(hasResource(directWater.world(),MaterialKind::Water));

    directActor.needs={0.01,0.93,0.01,0.01,0.01};
    const double directThirstBefore=directActor.needs.thirst;
    bool sawDirectDrink=false;
    bool sawDirectDrinkInteraction=false;
    for(int minute=0;minute<SimulationMinutesPerDay
        && directActor.needs.thirst>=directThirstBefore;++minute){
        const double thirstBeforeStep=directActor.needs.thirst;
        directWater.step();
        const ResidentPresentationObservation observed=
            directWater.observeResidentPresentation(directActorId);
        if(observed.active
           && observed.kind==PresentationActionKind::Physical
           && observed.physicalGoal==Goal::Drink
           && observed.directNaturalWaterSource){
            sawDirectDrink=true;
            assert(observed.phase!=PresentationActionPhase::Idle);
            if(observed.phase==PresentationActionPhase::Moving){
                // Walking toward the real source is not drinking. Normal need
                // pressure may increase thirst, but movement must not reduce it.
                assert(directActor.needs.thirst+1e-12>=thirstBeforeStep);
            }
            if(observed.phase==PresentationActionPhase::Interacting){
                sawDirectDrinkInteraction=true;
            }
        }
    }
    assert(sawDirectDrink);
    assert(sawDirectDrinkInteraction);
    assert(directActor.needs.thirst<directThirstBefore);
    assert(rawWaterUnitCount(directActor.civilization.inventory)==0);
    assert(simpleContainerCount(directActor.civilization.inventory)==0);

    directActor.needs={0.01,0.01,0.01,0.01,0.93};
    const double directHygieneBefore=directActor.needs.hygiene;
    const int waterUnitsBeforeDirectWash=
        resourceUnits(directWater.world(),MaterialKind::Water);
    bool sawDirectWash=false;
    for(int minute=0;minute<SimulationMinutesPerDay
        && directActor.needs.hygiene>=directHygieneBefore;++minute){
        directWater.step();
        const ResidentPresentationObservation observed=
            directWater.observeResidentPresentation(directActorId);
        if(observed.active
           && observed.kind==PresentationActionKind::Physical
           && observed.physicalGoal==Goal::Wash
           && observed.directNaturalWaterSource){
            sawDirectWash=true;
        }
    }
    assert(sawDirectWash);
    assert(directActor.needs.hygiene<directHygieneBefore);
    assert(resourceUnits(directWater.world(),MaterialKind::Water)
        < waterUnitsBeforeDirectWash);
    assert(rawWaterUnitCount(directActor.civilization.inventory)==0);
    assert(simpleContainerCount(directActor.civilization.inventory)==0);

    // Worst-case regression: a resident reaches urgent hunger/thirst without a
    // carried provision. This used to deadlock because urgent Needs suppressed
    // all civilization while Eat/Drink were unavailable with empty inventory.
    Simulation urgent(4242001);
    urgent.setupNewGame();
    assert(hasResource(urgent.world(),MaterialKind::Water));
    assert(hasResource(urgent.world(),MaterialKind::PlantFood));

    urgent.world().characters.resize(1);
    Character& actor=urgent.world().characters.front();
    const CharacterId actorId=actor.id;
    const std::string actorName=actor.name;
    actor.needs={0.92,0.96,0.05,0.05,0.05};
    assert(actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Water)==0);
    assert(actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==0);

    int criticalInactiveStreak=0;
    int maxCriticalInactiveStreak=0;
    bool sawCriticalProvisionPresentation=false;
    for(int minute=0;
        minute<SimulationMinutesPerDay
        && (
            !containsLog(
                urgent.logs(),
                actorName+" completed Drink via emergency fallback")
            || !containsLog(
                urgent.logs(),
                actorName+" completed Eat via emergency fallback")
        );
        ++minute){
        urgent.step();
        const ResidentPresentationObservation observed=
            urgent.observeResidentPresentation(actorId);
        const bool criticalNow=
            actor.needs.hunger>=CriticalSurvivalPreemptThreshold
            || actor.needs.thirst>=CriticalSurvivalPreemptThreshold;
        if(criticalNow && !observed.active){
            ++criticalInactiveStreak;
            maxCriticalInactiveStreak=std::max(
                maxCriticalInactiveStreak,criticalInactiveStreak);
        }else{
            criticalInactiveStreak=0;
        }
        if(criticalNow
           && observed.active
           && observed.kind==PresentationActionKind::Civilization
           && (
               observed.civilizationMaterial==MaterialKind::PlantFood
               || observed.civilizationMaterial==MaterialKind::Water
           )){
            sawCriticalProvisionPresentation=true;
            assert(observed.phase!=PresentationActionPhase::Idle);
        }
    }

    // The normal planner cadence is five simulated minutes. Critical survival
    // may wait only within that bounded cadence; it must never disappear into
    // a long inactive/Idle state while acquisition is actually in progress.
    assert(maxCriticalInactiveStreak<=5);
    assert(sawCriticalProvisionPresentation);
    assert(!containsLog(urgent.logs(),actorName+" -> Civilization Gather Water"));
    assert(containsLog(urgent.logs(),actorName+" completed Drink via emergency fallback"));
    assert(containsLog(urgent.logs(),actorName+" -> Civilization Gather PlantFood"));
    assert(containsLog(urgent.logs(),actorName+" completed Eat via emergency fallback"));

    const Character* recovered=findResident(urgent.world(),actorId);
    assert(recovered!=nullptr);
    assert(recovered->needs.hunger<0.92);
    assert(recovered->needs.thirst<0.96);

    // Urgent carried provisions must beat sleep/social/civilization work.
    // This protects survival when settlement bedding changes alter timing.
    Simulation urgentPriority(874213955);
    urgentPriority.setupNewGame();
    urgentPriority.world().characters.resize(1);
    Character& priorityActor=urgentPriority.world().characters.front();
    const std::string priorityName=priorityActor.name;
    priorityActor.needs={0.75,0.10,0.95,0.10,0.10};
    priorityActor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,1,0.5,1.0});
    urgentPriority.step();
    assert(containsLog(
        urgentPriority.logs(),
        priorityName+" -> Eat"));

    // Critical survival preempts an already-active sanitation loop. Recreate
    // the long-run failure shape: hunger reaches 100%, remote storage contains
    // food, a real nearby food node exists, and the resident is already using
    // the outdoor toilet fallback. The next few ticks must transition into the
    // nearby authoritative Gather rather than Idle/toilet repetition.
    SimulationRuleset preemptRules=DefaultSimulationRuleset;
    preemptRules.needs.hungerPerMinute=0.0;
    preemptRules.needs.thirstPerMinute=0.0;
    preemptRules.needs.sleepPerMinute=0.0;
    preemptRules.needs.bladderPerMinute=0.0;
    preemptRules.needs.hygienePerMinute=0.0;
    Simulation preemptProbe(
        874213956,0,CurrentWorldGenerationVersion,preemptRules);
    preemptProbe.setupNewGame();
    preemptProbe.world().characters.resize(1);
    preemptProbe.world().resourceNodes.clear();
    preemptProbe.world().storageSites.clear();

    Character& preemptActor=preemptProbe.world().characters.front();
    const CharacterId preemptId=preemptActor.id;
    preemptActor.needs={0.10,0.10,0.10,0.95,0.10};

    GridPos preemptPos{};
    assert(preemptProbe.runtimePosition(preemptId,preemptPos));

    ResourceNode nearbyFood;
    nearbyFood.id=990801;
    nearbyFood.material=MaterialKind::PlantFood;
    nearbyFood.quantity=6;
    nearbyFood.maxQuantity=6;
    nearbyFood.pos={preemptPos.x+2,preemptPos.y};
    preemptProbe.world().resourceNodes.push_back(nearbyFood);

    StorageSite impossibleRemoteFood;
    impossibleRemoteFood.id=990701;
    impossibleRemoteFood.pos={
        preemptPos.x+SettlementServiceRadiusGrid+20,
        preemptPos.y};
    impossibleRemoteFood.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,3,0.5,1.0});
    preemptProbe.world().storageSites.push_back(impossibleRemoteFood);

    preemptProbe.step();
    ResidentPresentationObservation toiletPresentation=
        preemptProbe.observeResidentPresentation(preemptId);
    assert(toiletPresentation.active);
    assert(toiletPresentation.kind==PresentationActionKind::Physical);
    assert(toiletPresentation.physicalGoal==Goal::UseToilet);

    preemptActor.needs.hunger=1.0;
    bool sawLocalFoodGather=false;
    bool sawIdleWhileCritical=false;
    for(int minute=0;minute<6 && !sawLocalFoodGather;++minute){
        preemptProbe.step();
        const ResidentPresentationObservation observed=
            preemptProbe.observeResidentPresentation(preemptId);
        if(observed.active
           && observed.kind==PresentationActionKind::Civilization
           && observed.civilizationIntent==CivilizationIntent::Gather
           && observed.civilizationMaterial==MaterialKind::PlantFood
           && observed.civilizationResourceNode==nearbyFood.id){
            sawLocalFoodGather=true;
        }
        if(observed.active
           && observed.kind==PresentationActionKind::Physical
           && observed.physicalGoal==Goal::Idle
           && preemptActor.needs.hunger>=CriticalSurvivalPreemptThreshold){
            sawIdleWhileCritical=true;
        }
    }
    assert(sawLocalFoodGather);
    assert(!sawIdleWhileCritical);

    // Long-run frontier exhaustion: if every chunk in the ordinary
    // six-chunk search envelope is already known/depleted, critical hunger
    // must still pre-empt an active toilet plan and move into an expanded
    // PlantFood Explore instead of returning to toilet/Idle.
    SimulationRuleset frontierRules=DefaultSimulationRuleset;
    frontierRules.needs.hungerPerMinute=0.0;
    frontierRules.needs.thirstPerMinute=0.0;
    frontierRules.needs.sleepPerMinute=0.0;
    frontierRules.needs.bladderPerMinute=0.0;
    frontierRules.needs.hygienePerMinute=0.0;
    Simulation frontierProbe(
        874213957,0,CurrentWorldGenerationVersion,frontierRules);
    frontierProbe.setupNewGame();
    frontierProbe.world().characters.resize(1);
    frontierProbe.world().resourceNodes.clear();
    frontierProbe.world().storageSites.clear();
    frontierProbe.world().generatedNaturalChunks.clear();

    Character& frontierActor=frontierProbe.world().characters.front();
    const CharacterId frontierId=frontierActor.id;
    frontierActor.needs={0.10,0.10,0.10,0.95,0.10};

    GridPos frontierPos{};
    assert(frontierProbe.runtimePosition(frontierId,frontierPos));
    const ChunkCoord frontierCenter=chunkCoordForGrid(frontierPos);
    for(int dx=-ResourceExplorationMaxRadiusChunks;
        dx<=ResourceExplorationMaxRadiusChunks;++dx){
        for(int dy=-ResourceExplorationMaxRadiusChunks;
            dy<=ResourceExplorationMaxRadiusChunks;++dy){
            GeneratedNaturalChunk generated;
            generated.coord={frontierCenter.x+dx,frontierCenter.y+dy};
            frontierProbe.world().generatedNaturalChunks.push_back(generated);
        }
    }
    std::sort(
        frontierProbe.world().generatedNaturalChunks.begin(),
        frontierProbe.world().generatedNaturalChunks.end(),
        [](const GeneratedNaturalChunk& a,const GeneratedNaturalChunk& b){
            return a.coord<b.coord;
        });

    frontierProbe.step();
    ResidentPresentationObservation frontierToilet=
        frontierProbe.observeResidentPresentation(frontierId);
    assert(frontierToilet.active);
    assert(frontierToilet.kind==PresentationActionKind::Physical);
    assert(frontierToilet.physicalGoal==Goal::UseToilet);

    frontierActor.needs.hunger=1.0;
    bool sawExpandedExplore=false;
    bool returnedToToiletWhileCritical=false;
    for(int minute=0;minute<8 && !sawExpandedExplore;++minute){
        frontierProbe.step();
        const ResidentPresentationObservation observed=
            frontierProbe.observeResidentPresentation(frontierId);
        if(observed.active
           && observed.kind==PresentationActionKind::Civilization
           && observed.civilizationIntent==CivilizationIntent::Explore
           && observed.civilizationMaterial==MaterialKind::PlantFood){
            sawExpandedExplore=true;
        }
        if(observed.active
           && observed.kind==PresentationActionKind::Physical
           && observed.physicalGoal==Goal::UseToilet
           && frontierActor.needs.hunger>=CriticalSurvivalPreemptThreshold){
            returnedToToiletWhileCritical=true;
        }
    }
    assert(sawExpandedExplore);
    assert(!returnedToToiletWhileCritical);

    // Production-like natural New Game: over the first four simulation days,
    // every founder must prove actual food/water acquisition and consumption.
    // Toilet remains an outdoor fallback until a real sanitation affordance is
    // developed, and therefore must leave authoritative environmental residue.
    Simulation natural(874213954);
    natural.setupNewGame();

    std::vector<std::pair<CharacterId,std::string>> founders;
    for(const auto& resident:natural.world().characters){
        founders.push_back({resident.id,resident.name});
    }
    assert(founders.size()==4);

    natural.runMinutes(4*24*60);

    for(const auto& founder:founders){
        const Character* resident=findResident(natural.world(),founder.first);
        assert(resident!=nullptr);
        assert(resident->alive);

        const std::string prefix=founder.second+" ";
        assert(containsLog(natural.logs(),prefix+"completed Drink via emergency fallback"));
        assert(containsLog(natural.logs(),prefix+"-> Civilization Gather PlantFood"));
        assert(containsLog(natural.logs(),prefix+"completed Eat via emergency fallback"));

        // These bounds are deliberately loose: the resident may be nearing the
        // next meal/drink at the exact day-four sample, but must not be pinned at
        // the hard clamp by an acquisition deadlock.
        assert(resident->needs.hunger<0.999);
        // A single day-four sample may land exactly at the thirst clamp just
        // before the next decision tick. Prove recurrent survival instead:
        // each founder must have completed direct-source Drink repeatedly.
        assert(countLogs(
            natural.logs(),
            prefix+"completed Drink via emergency fallback")>=2);
    }

    const EnvironmentObservation environment=natural.observeEnvironment();
    assert(environment.humanWasteResidues>0);
    assert(containsLog(natural.logs(),"completed UseToilet via emergency fallback"));

    std::cout << "early survival provisioning + outdoor sanitation passed\n";
    return 0;
}
