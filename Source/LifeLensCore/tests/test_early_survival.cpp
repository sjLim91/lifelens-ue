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

    assert(containsLog(urgent.logs(),actorName+" -> Civilization Gather Water"));
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
        assert(containsLog(natural.logs(),prefix+"-> Civilization Gather Water"));
        assert(containsLog(natural.logs(),prefix+"completed Drink via emergency fallback"));
        assert(containsLog(natural.logs(),prefix+"-> Civilization Gather PlantFood"));
        assert(containsLog(natural.logs(),prefix+"completed Eat via emergency fallback"));

        // These bounds are deliberately loose: the resident may be nearing the
        // next meal/drink at the exact day-four sample, but must not be pinned at
        // the hard clamp by an acquisition deadlock.
        assert(resident->needs.hunger<0.999);
        assert(resident->needs.thirst<0.999);
    }

    const EnvironmentObservation environment=natural.observeEnvironment();
    assert(environment.humanWasteResidues>0);
    assert(containsLog(natural.logs(),"completed UseToilet via emergency fallback"));

    std::cout << "early survival provisioning + outdoor sanitation passed\n";
    return 0;
}
