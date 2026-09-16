#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/PrimitiveFireProgression.h"
#include "lifelens/PrimitiveSmeltingProgression.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

namespace {

void learnPrerequisites(Character& character)
{
    character.civilization.knowledge.learn(TechniqueId::SharpFlake,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::ChippedStoneTool,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::FireMaking,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::FiberCordage,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::DiggingStick,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::StoneHammer,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::DugSanitationPit,KnowledgeLevel::Reproducible,0.90);
    character.civilization.knowledge.learn(TechniqueId::PrimitiveStorage,KnowledgeLevel::Reproducible,0.90);
}

ConstructedFacility* buildOperationalFirePit(World& world,Character& builder)
{
    builder.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Stone,5,0.5,1.0});
    builder.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,2,0.5,1.0});
    const PrimitiveFirePitSiteOpportunity site=choosePrimitiveFirePitSite(world,builder.id);
    assert(site.available);
    ConstructedFacility* fire=establishPrimitiveFirePitProject(world,builder,site.pos);
    assert(fire!=nullptr);
    assert(deliverFacilityMaterial(*fire,builder.civilization.inventory,MaterialKind::Stone,5)==5);
    assert(deliverFacilityMaterial(*fire,builder.civilization.inventory,MaterialKind::Wood,2)==2);
    PrimitiveFirePitWorkResult work;
    while(!work.completed){
        work=workOnPrimitiveFirePit(world,builder,3.0);
        assert(work.worked);
    }
    return primitiveFirePitProject(world);
}

} // namespace

int main()
{
    const FacilityConstructionSpec furnaceSpec=facilityConstructionSpec(FacilityKind::Furnace);
    assert(furnaceSpec.requiredWork==12.0);
    assert(furnaceSpec.requirements.size()==2);
    assert(facilityKindConstructible(FacilityKind::Furnace));

    Simulation simulation(121221);
    simulation.setupNewGame();
    Character& builder=simulation.world().characters.front();
    learnPrerequisites(builder);

    // No free metal infrastructure: furnace planning requires a real operational fire pit.
    assert(!choosePrimitiveFurnaceSite(simulation.world(),builder.id).available);
    ConstructedFacility* fire=buildOperationalFirePit(simulation.world(),builder);
    assert(fire!=nullptr && fire->state==FacilityState::Operational && fire->active);

    const PrimitiveFurnaceSiteOpportunity site=choosePrimitiveFurnaceSite(simulation.world(),builder.id);
    assert(site.available);
    ConstructedFacility* furnace=establishPrimitiveFurnaceProject(simulation.world(),builder,site.pos);
    assert(furnace!=nullptr);
    const FacilityId furnaceId=furnace->id;
    assert(furnace->kind==FacilityKind::Furnace);
    assert(furnace->state==FacilityState::Planned);

    builder.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Stone,8,0.5,1.0});
    builder.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Clay,6,0.5,1.0});
    assert(deliverFacilityMaterial(*furnace,builder.civilization.inventory,MaterialKind::Stone,8)==8);
    assert(deliverFacilityMaterial(*furnace,builder.civilization.inventory,MaterialKind::Clay,6)==6);
    PrimitiveFurnaceWorkResult furnaceWork;
    while(!furnaceWork.completed){
        furnaceWork=workOnPrimitiveFurnace(simulation.world(),builder,3.0);
        assert(furnaceWork.worked);
    }
    furnace=primitiveFurnaceProject(simulation.world());
    assert(furnace!=nullptr && furnace->state==FacilityState::Operational && furnace->active);

    // Smelting knowledge can only be investigated when ore + charcoal are physically held
    // beside an operational furnace. Deterministic attempts may fail first.
    builder.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::CopperOre,64,0.5,1.0});
    builder.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Charcoal,64,0.55,1.0});
    assert(copperSmeltingOpportunityAvailable(simulation.world(),builder));

    bool discovered=false;
    for(int attempt=0;attempt<64 && !discovered;++attempt){
        ExperimentContext context;
        context.worldSeed=simulation.world().seed;
        context.actor=builder.id;
        context.attemptIndex=static_cast<std::uint64_t>(simulation.world().minute+attempt);
        context.kind=ExperimentKind::SmeltCopperOre;
        context.material=MaterialKind::CopperOre;
        context.learningSkill=0.95;
        context.curiosity=0.95;
        context.patience=0.90;
        context.smeltingOpportunityAvailable=true;
        const ExperimentResult result=attemptExperiment(
            context,builder.civilization.inventory,builder.civilization.knowledge);
        assert(result.attempted);
        discovered=result.success;
    }
    assert(discovered);
    assert(builder.civilization.knowledge.knowsAtLeast(
        TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible));
    assert(builder.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::CopperMetal)>=1);

    // Repeated production must go through the authoritative furnace runtime.
    const int copperBefore=builder.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::CopperMetal);
    assert(loadPrimitiveFurnaceCopperCharge(simulation.world(),builder,furnaceId,2)==2);
    furnace=primitiveFurnaceProject(simulation.world());
    assert(furnace->oreUnits==2 && furnace->fuelUnits==2);
    assert(ignitePrimitiveFurnace(simulation.world(),builder,furnaceId));
    assert(furnace->lit && furnace->heatLevel==1.0);

    for(int i=0;i<PrimitiveFurnaceSmeltMinutesPerCopperUnit;++i){
        ++simulation.world().minute;
        advancePrimitiveFireOneMinute(simulation.world());
    }
    furnace=primitiveFurnaceProject(simulation.world());
    assert(furnace->metalUnits==1);
    assert(furnace->oreUnits==1 && furnace->fuelUnits==1);
    assert(furnace->lit);

    for(int i=0;i<PrimitiveFurnaceSmeltMinutesPerCopperUnit;++i){
        ++simulation.world().minute;
        advancePrimitiveFireOneMinute(simulation.world());
    }
    furnace=primitiveFurnaceProject(simulation.world());
    assert(furnace->metalUnits==2);
    assert(furnace->oreUnits==0 && furnace->fuelUnits==0);
    assert(!furnace->lit);
    assert(collectPrimitiveFurnaceCopper(simulation.world(),builder,furnaceId,2)==2);
    assert(builder.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::CopperMetal)==copperBefore+2);

    // Mid-smelt v5 snapshot roundtrip preserves charge, heat and countdown exactly.
    assert(loadPrimitiveFurnaceCopperCharge(simulation.world(),builder,furnaceId,2)==2);
    assert(ignitePrimitiveFurnace(simulation.world(),builder,furnaceId));
    for(int i=0;i<12;++i){
        ++simulation.world().minute;
        advancePrimitiveFireOneMinute(simulation.world());
    }
    furnace=primitiveFurnaceProject(simulation.world());
    const int savedOre=furnace->oreUnits;
    const int savedFuel=furnace->fuelUnits;
    const int savedMetal=furnace->metalUnits;
    const int savedBurn=furnace->burnMinutesRemaining;
    const double savedHeat=furnace->heatLevel;
    assert(furnace->lit);

    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error));
    assert(error.empty());
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(error.empty());

    const ConstructedFacility* decodedFurnace=nullptr;
    for(const auto& facility:decoded.world.facilities){
        if(facility.id==furnaceId){ decodedFurnace=&facility; break; }
    }
    assert(decodedFurnace!=nullptr);
    assert(decodedFurnace->kind==FacilityKind::Furnace);
    assert(decodedFurnace->oreUnits==savedOre);
    assert(decodedFurnace->fuelUnits==savedFuel);
    assert(decodedFurnace->metalUnits==savedMetal);
    assert(decodedFurnace->burnMinutesRemaining==savedBurn);
    assert(decodedFurnace->heatLevel==savedHeat);
    assert(decodedFurnace->lit);
    assert(validConstructedFacility(*decodedFurnace));

    Simulation restored(
        decoded.world.seed,decoded.world.populationSeed,
        decoded.world.generationVersion,decoded.ruleset);
    assert(restored.restoreSnapshot(decoded,&error));
    assert(error.empty());
    restored.runMinutes(1);
    simulation.runMinutes(1);
    const ConstructedFacility* sourceAfter=primitiveFurnaceProject(simulation.world());
    const ConstructedFacility* restoredAfter=primitiveFurnaceProject(restored.world());
    assert(sourceAfter!=nullptr && restoredAfter!=nullptr);
    assert(sourceAfter->oreUnits==restoredAfter->oreUnits);
    assert(sourceAfter->fuelUnits==restoredAfter->fuelUnits);
    assert(sourceAfter->metalUnits==restoredAfter->metalUnits);
    assert(sourceAfter->burnMinutesRemaining==restoredAfter->burnMinutesRemaining);
    assert(std::abs(sourceAfter->heatLevel-restoredAfter->heatLevel)<1e-12);

    return 0;
}
