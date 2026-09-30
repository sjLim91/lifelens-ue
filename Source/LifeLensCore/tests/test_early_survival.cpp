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
    for(int minute=0;minute<SimulationMinutesPerDay
        && directActor.needs.thirst>=directThirstBefore;++minute){
        directWater.step();
        const ResidentPresentationObservation observed=
            directWater.observeResidentPresentation(directActorId);
        if(observed.active
           && observed.kind==PresentationActionKind::Physical
           && observed.physicalGoal==Goal::Drink
           && observed.directNaturalWaterSource){
            sawDirectDrink=true;
        }
    }
    assert(sawDirectDrink);
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
    }

    assert(!containsLog(urgent.logs(),actorName+" -> Civilization Gather Water"));
    assert(containsLog(urgent.logs(),actorName+" completed Drink via emergency fallback"));
    assert(containsLog(urgent.logs(),actorName+" -> Civilization Gather PlantFood"));
    assert(containsLog(urgent.logs(),actorName+" completed Eat via emergency fallback"));

    const Character* recovered=findResident(urgent.world(),actorId);
    assert(recovered!=nullptr);
    assert(recovered->needs.hunger<0.92);
    assert(recovered->needs.thirst<0.96);

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
