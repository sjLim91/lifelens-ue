#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/ContextAction.h"
#include "lifelens/PrimitiveFireProgression.h"
#include "lifelens/SettlementProgression.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static ConstructedFacility* buildFoundation(
    World& world,
    Character& actor,
    FacilityKind kind,
    GridPos pos)
{
    ConstructedFacility* project=
        establishSettlementFacilityProject(world,actor.id,kind,pos);
    if(project==nullptr) return nullptr;

    const std::vector<FacilityMaterialRequirement> requirements=project->requirements;
    for(const auto& requirement:requirements){
        const int missing=facilityMissingMaterial(*project,requirement.material);
        if(missing<=0) continue;
        actor.civilization.inventory.add({
            ItemKind::RawMaterial,requirement.material,missing,0.5,1.0});
        if(deliverFacilityMaterial(
            *project,actor.civilization.inventory,requirement.material,missing)!=missing){
            return nullptr;
        }
    }

    for(int i=0;i<64 && project->state!=FacilityState::Operational;++i){
        const SettlementFacilityWorkResult work=
            workOnSettlementFacility(world,actor,project->id,8.0);
        if(!work.worked) return nullptr;
        project=settlementFacilityProject(world,kind);
        if(project==nullptr) return nullptr;
    }

    return facilityOperationalAndActive(*project) ? project : nullptr;
}

static GridPos findHarshUnblockedPosition(const World& world)
{
    const ChunkCoord start=world.initialStartRegionCoord;
    GridPos best=world.initialStartRegionCenterGrid();
    double bestStress=-1.0;

    for(int radius=4;radius<=96;++radius){
        const std::array<ChunkCoord,8> candidates={{
            {start.x+radius,start.y},
            {start.x-radius,start.y},
            {start.x,start.y+radius},
            {start.x,start.y-radius},
            {start.x+radius,start.y+radius},
            {start.x-radius,start.y+radius},
            {start.x+radius,start.y-radius},
            {start.x-radius,start.y-radius}
        }};
        for(const ChunkCoord coord:candidates){
            const GridPos pos=chunkOriginGrid(coord);
            if(settlementFacilitySiteBlocked(world,pos)) continue;
            const EnvironmentalConsequenceProfile consequence=
                deriveEnvironmentalConsequences(
                    deriveDynamicEnvironment(
                        world.genesisIdentity(),coord,world.minute));
            const double stress=
                consequence.heatStress01+
                consequence.coldStress01+
                consequence.wetStress01+
                consequence.outdoorWorkFriction01;
            if(stress>bestStress){
                bestStress=stress;
                best=pos;
            }
            if(stress>=0.55) return pos;
        }
    }
    return best;
}

int main()
{
    Simulation simulation(820042);
    simulation.setupNewGame();
    World& world=simulation.world();
    CHECK(world.characters.size()==4);
    Character& actor=world.characters[0];
    actor.needs={0.08,0.08,0.08,0.08,0.08};
    actor.civilization.craftingSkill=0.20;

    // SleepingPlace is a real primitive sleep target and beats exposed ground.
    const SettlementFacilitySiteOpportunity sleepSite=
        chooseSettlementFacilitySite(
            world,actor.id,FacilityKind::SleepingPlace);
    CHECK(sleepSite.available);
    ConstructedFacility* sleeping=
        buildFoundation(
            world,actor,FacilityKind::SleepingPlace,sleepSite.pos);
    CHECK(sleeping!=nullptr);
    CHECK(settlementSleepRecoveryPerTick(*sleeping)
        > -emergencyUseEffectPerTick(Goal::Sleep).sleep);

    GridPos sleepTarget{};
    FacilityId sleepFacilityId=0;
    CHECK(simulation.settlementSleepTarget(
        actor.id,sleepTarget,sleepFacilityId));
    CHECK(sleepFacilityId==sleeping->id);
    CHECK(sleepTarget.x==sleeping->pos.x);
    CHECK(sleepTarget.y==sleeping->pos.y);

    const double sleepingDurabilityBefore=sleeping->durability;
    CHECK(applyFacilityWear(
        *sleeping,facilityWearPerUse(FacilityKind::SleepingPlace)));
    CHECK(sleeping->durability<sleepingDurabilityBefore);
    CHECK(validConstructedFacility(*sleeping));

    // WorkSurface makes a real spatial Craft action, boosts effective crafting
    // quality, and wears only after a successful use.
    const SettlementFacilitySiteOpportunity workSite=
        chooseSettlementFacilitySite(
            world,actor.id,FacilityKind::WorkSurface);
    CHECK(workSite.available);
    ConstructedFacility* workSurface=
        buildFoundation(
            world,actor,FacilityKind::WorkSurface,workSite.pos);
    CHECK(workSurface!=nullptr);

    actor.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Reproducible,0.95);
    actor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Flint,4,0.5,1.0});

    CivilizationUtilityDecision craft;
    craft.intent=CivilizationIntent::Craft;
    craft.technique=TechniqueId::SharpFlake;
    craft.item=ItemKind::SharpFlake;
    craft.material=MaterialKind::Flint;
    craft.quantity=1;
    craft.facilityKind=FacilityKind::WorkSurface;
    craft.facility=workSurface->id;
    craft.hasFacilityTarget=true;
    craft.facilityTargetPos=workSurface->pos;

    CHECK(civilizationContextRequiresSpatialTarget(craft));
    GridPos craftTarget{};
    SanitationSiteId sanitation=0;
    CHECK(resolveCivilizationContextTarget(
        world,actor,craft,craftTarget,sanitation));
    CHECK(craftTarget.x==workSurface->pos.x);
    CHECK(craftTarget.y==workSurface->pos.y);

    const double baseQuality=
        0.50+0.35*clampCivilization01(actor.civilization.craftingSkill);
    const double workDurabilityBefore=workSurface->durability;
    const CivilizationExecutionResult crafted=
        executeCivilizationDecision(world,actor,craft);
    CHECK(crafted.executed && crafted.success);
    CHECK(crafted.craft.output.quantity==1);
    CHECK(crafted.craft.output.quality>baseQuality);
    CHECK(workSurface->durability<workDurabilityBefore);
    CHECK(crafted.facilityDurabilityBefore>crafted.facilityDurabilityAfter);

    // A damaged facility creates maintenance demand. Repair consumes a real
    // material unit and restores durability through a spatial labor action.
    workSurface->durability=0.55;
    CHECK(settlementFacilityNeedsMaintenance(*workSurface));
    const MaterialKind repairMaterial=
        facilityRepairMaterial(FacilityKind::WorkSurface);
    CHECK(repairMaterial==MaterialKind::Wood);

    const int woodHeld=actor.civilization.inventory.count(
        ItemKind::RawMaterial,repairMaterial);
    if(woodHeld>0){
        CHECK(actor.civilization.inventory.remove(
            ItemKind::RawMaterial,repairMaterial,woodHeld));
    }

    // With no repair material carried, maintenance contributes to real Gather
    // demand instead of synthesizing repair resources.
    world.resourceNodes.erase(
        std::remove_if(
            world.resourceNodes.begin(),
            world.resourceNodes.end(),
            [repairMaterial](const ResourceNode& node){
                return node.material!=repairMaterial;
            }),
        world.resourceNodes.end());
    CHECK(!world.resourceNodes.empty());
    const CivilizationUtilityDecision gatherRepair=
        bestGatherDecision(world,actor);
    CHECK(gatherRepair.intent==CivilizationIntent::Gather);
    CHECK(gatherRepair.material==repairMaterial);
    CHECK(settlementRepairMaterialDemand(world,repairMaterial)>0);

    actor.civilization.inventory.add({
        ItemKind::RawMaterial,repairMaterial,1,0.5,1.0});
    const int repairUnitsBefore=actor.civilization.inventory.count(
        ItemKind::RawMaterial,repairMaterial);

    const CivilizationUtilityDecision repair=
        bestSettlementFoundationDecision(world,actor,workSurface->pos);
    CHECK(repair.intent==CivilizationIntent::Craft);
    CHECK(repair.facilityKind==FacilityKind::WorkSurface);
    CHECK(repair.facilityAction==FacilityBuildAction::Repair);
    CHECK(repair.facility==workSurface->id);
    CHECK(civilizationContextRequiresSpatialTarget(repair));

    GridPos repairTarget{};
    CHECK(resolveCivilizationContextTarget(
        world,actor,repair,repairTarget,sanitation));
    CHECK(repairTarget.x==workSurface->pos.x);
    CHECK(repairTarget.y==workSurface->pos.y);

    const double damagedDurability=workSurface->durability;
    const CivilizationExecutionResult repaired=
        executeCivilizationDecision(world,actor,repair);
    CHECK(repaired.executed && repaired.success);
    CHECK(workSurface->durability>damagedDurability);
    CHECK(actor.civilization.inventory.count(
        ItemKind::RawMaterial,repairMaterial)==repairUnitsBefore-1);
    CHECK(repaired.facilityDurabilityAfter>repaired.facilityDurabilityBefore);

    // Shelter causally reduces environmental Need pressure while residents are
    // actually at the shelter position, and weather slowly wears the structure.
    const GridPos harshPos=findHarshUnblockedPosition(world);
    ConstructedFacility* shelter=
        buildFoundation(world,actor,FacilityKind::Shelter,harshPos);
    CHECK(shelter!=nullptr);
    CHECK(settlementShelterProtection01(world,harshPos)>0.0);

    Character protectedResident=actor;
    Character exposedResident=actor;
    protectedResident.needs={0.20,0.20,0.20,0.20,0.20};
    exposedResident.needs=protectedResident.needs;

    World exposedWorld=world;
    for(auto& facility:exposedWorld.facilities){
        if(facility.kind==FacilityKind::Shelter){
            facility.active=false;
        }
    }

    applyResidentEnvironmentalNeedPressure(
        world,protectedResident,harshPos);
    applyResidentEnvironmentalNeedPressure(
        exposedWorld,exposedResident,harshPos);

    const double protectedPressure=
        protectedResident.needs.hunger+
        protectedResident.needs.thirst+
        protectedResident.needs.sleep+
        protectedResident.needs.hygiene;
    const double exposedPressure=
        exposedResident.needs.hunger+
        exposedResident.needs.thirst+
        exposedResident.needs.sleep+
        exposedResident.needs.hygiene;
    CHECK(protectedPressure<exposedPressure);

    world.minute=60;
    const double shelterDurabilityBefore=shelter->durability;
    advanceSettlementFacilityWearOneMinute(world);
    CHECK(shelter->durability<shelterDurabilityBefore);

    // Durability reaching zero removes every benefit and turns the facility
    // into a ruined historical object. C-S1 can then plan a replacement.
    CHECK(applyFacilityWear(*workSurface,2.0));
    CHECK(workSurface->state==FacilityState::Ruined);
    CHECK(!workSurface->active);
    CHECK(workSurface->durability==0.0);
    CHECK(settlementWorkSurfaceSkillBonus(*workSurface)==0.0);
    CHECK(!hasOperationalSettlementFacility(
        world,FacilityKind::WorkSurface));
    CHECK(validConstructedFacility(*workSurface));

    for(int i=0;i<6;++i){
        actor.civilization.knowledge.recordSuccessfulUse(
            TechniqueId::SharpFlake);
    }
    const CivilizationUtilityDecision replacement=
        bestSettlementFoundationDecision(
            world,actor,world.initialStartRegionCenterGrid());
    CHECK(replacement.intent==CivilizationIntent::Craft);
    CHECK(replacement.facilityKind==FacilityKind::WorkSurface);
    CHECK(replacement.facilityAction==FacilityBuildAction::Plan);

    // Durability/state remains authoritative through the existing snapshot
    // codec; no presentation-only maintenance state is introduced.
    std::vector<std::uint8_t> bytes;
    std::string error;
    CHECK(encodeSimulationSnapshot(
        simulation.captureSnapshot(),bytes,&error));
    CHECK(error.empty());
    SimulationStateSnapshot decoded;
    CHECK(decodeSimulationSnapshot(bytes,decoded,&error));
    CHECK(error.empty());

    bool sawRuinedWorkSurface=false;
    bool sawWornShelter=false;
    for(const auto& facility:decoded.world.facilities){
        if(facility.kind==FacilityKind::WorkSurface
           && facility.state==FacilityState::Ruined){
            sawRuinedWorkSurface=true;
            CHECK(facility.durability==0.0);
            CHECK(!facility.active);
        }
        if(facility.kind==FacilityKind::Shelter){
            sawWornShelter=true;
            CHECK(facility.durability<1.0);
            CHECK(facility.state==FacilityState::Operational);
        }
    }
    CHECK(sawRuinedWorkSurface);
    CHECK(sawWornShelter);

    std::cout << "Stage C-S2 facility effects + maintenance passed\n";
    return 0;
}
