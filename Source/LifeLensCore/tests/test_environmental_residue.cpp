#include <cassert>
#include <string>
#include <vector>

#include "lifelens/EnvironmentalResidue.h"
#include "lifelens/Planner.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

int main()
{
    // Domain accumulation / decay / observation contract.
    EnvironmentalResidueField field;
    const GridPos site{6,-2};

    auto& first=field.deposit(EnvironmentalResidueKind::HumanWaste,site,1,480,1.0,0.40,3);
    assert(first.id!=0);
    assert(field.all().size()==1);
    assert(field.exposureAt(site)>0.0);

    const double initialAmount=first.amount;
    field.deposit(EnvironmentalResidueKind::HumanWaste,site,2,490,1.0,0.40,3);
    assert(field.all().size()==1);
    assert(field.all().front().amount>initialAmount);
    assert(field.all().front().sourceCharacter==2);

    const double accumulated=field.all().front().amount;
    field.advanceToMinute(490+24*60);
    assert(field.all().front().amount<accumulated);
    assert(field.exposureAt({100,100})==0.0);

    const auto observation=buildEnvironmentObservation(490+24*60,field);
    assert(observation.totalResidues==1);
    assert(observation.humanWasteResidues==1);
    assert(observation.aggregateAmount>0.0);
    assert(!observation.residues.empty());

    EnvironmentalResidueField restoredField;
    assert(restoredField.restoreState(field.all()));
    assert(restoredField.all().size()==field.all().size());
    assert(restoredField.all().front().id==field.all().front().id);

    // A resident receives one deterministic outdoor sanitation site relative
    // to the v1 settlement origin. Current position must not make it drift.
    const GridPos stableA=deterministicOutdoorReliefPosition(9191,7,{0,0});
    const GridPos stableB=deterministicOutdoorReliefPosition(9191,7,{99,-71});
    assert(stableA.x==stableB.x && stableA.y==stableB.y);

    // Production NEW GAME must not fabricate modern facilities.
    Simulation production(9191);
    production.setupNewGame();
    assert(production.world().objects.empty());
    assert(production.world().environmentalResidues.all().empty());

    // Emergency Eat/Drink only exist when the resident really owns a
    // consumable provision, and creating the plan consumes exactly one unit.
    World provisionWorld(42);
    provisionWorld.objects.clear();
    Character provisionResident;
    provisionResident.id=1;
    provisionResident.civilization.character=1;

    assert(buildPlan(provisionWorld,provisionResident,Goal::Eat,{}).empty());
    provisionResident.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::PlantFood,1,0.5,1.0});
    auto eatPlan=buildPlan(provisionWorld,provisionResident,Goal::Eat,{});
    assert(eatPlan.size()==1 && eatPlan.front().type==ActionType::EmergencyUse);
    assert(provisionResident.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==0);

    assert(buildPlan(provisionWorld,provisionResident,Goal::Drink,{}).empty());
    provisionResident.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Water,1,0.5,1.0});
    auto drinkPlan=buildPlan(provisionWorld,provisionResident,Goal::Drink,{});
    assert(drinkPlan.size()==1 && drinkPlan.front().type==ActionType::EmergencyUse);
    assert(provisionResident.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Water)==0);

    // With no toilet object, urgent bladder pressure must eventually execute
    // the emergency path and leave authoritative Core environmental state.
    for(auto& resident:production.world().characters){
        resident.needs={0.02,0.02,0.02,0.02,0.02};
    }
    const CharacterId sanitationActor=production.world().characters.front().id;
    production.world().characters.front().needs.bladder=0.98;
    production.runMinutes(20);
    const auto runtimeEnvironment=production.observeEnvironment();
    assert(runtimeEnvironment.humanWasteResidues>=1);
    bool foundActorResidue=false;
    for(const auto& residue:runtimeEnvironment.residues){
        if(residue.sourceCharacter==sanitationActor){
            foundActorResidue=true;
            break;
        }
    }
    assert(foundActorResidue);

    // Environmental consequences are authoritative Save/Load state and must
    // continue deterministically after binary snapshot restore.
    Simulation source(44017);
    source.setupNewGame();
    const CharacterId sourceId=source.world().characters.front().id;
    const GridPos persistedSite=deterministicOutdoorReliefPosition(
        source.world().seed,sourceId,{});
    source.world().environmentalResidues.deposit(
        EnvironmentalResidueKind::HumanWaste,persistedSite,sourceId,
        source.world().minute,2.5,0.61,4);

    std::string error;
    std::vector<std::uint8_t> bytes;
    assert(encodeSimulationSnapshot(source.captureSnapshot(),bytes,&error));
    assert(error.empty());

    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(decoded.world.environmentalResidues.all().size()==1);
    const auto& decodedResidue=decoded.world.environmentalResidues.all().front();
    assert(decodedResidue.sourceCharacter==sourceId);
    assert(decodedResidue.pos.x==persistedSite.x && decodedResidue.pos.y==persistedSite.y);
    assert(decodedResidue.amount==2.5);
    assert(decodedResidue.intensity==0.61);

    Simulation restored(1);
    assert(restored.restoreSnapshot(decoded,&error));
    source.runMinutes(120);
    restored.runMinutes(120);

    std::vector<std::uint8_t> futureSource;
    std::vector<std::uint8_t> futureRestored;
    assert(encodeSimulationSnapshot(source.captureSnapshot(),futureSource,&error));
    assert(encodeSimulationSnapshot(restored.captureSnapshot(),futureRestored,&error));
    assert(futureSource==futureRestored);

    // Starting a new world clears prior residue by definition.
    restored.setupNewGame();
    assert(restored.world().environmentalResidues.all().empty());

    return 0;
}
