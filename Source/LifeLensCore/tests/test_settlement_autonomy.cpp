#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <string>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/ContextAction.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static bool deliverAllAndComplete(
    World& world,
    Character& actor,
    FacilityKind kind)
{
    ConstructedFacility* project=settlementFacilityProject(world,kind);
    if(project==nullptr) return false;

    for(const auto& requirement:project->requirements){
        const int missing=facilityMissingMaterial(*project,requirement.material);
        if(missing<=0) continue;
        actor.civilization.inventory.add({
            ItemKind::RawMaterial,requirement.material,missing,0.5,1.0});
        if(deliverFacilityMaterial(
            *project,actor.civilization.inventory,requirement.material,missing)!=missing) return false;
    }

    for(int i=0;i<32 && project->state!=FacilityState::Operational;++i){
        const SettlementFacilityWorkResult work=
            workOnSettlementFacility(world,actor,project->id,4.0);
        if(!work.worked) return false;
        project=settlementFacilityProject(world,kind);
        if(project==nullptr) return false;
    }
    return project->state==FacilityState::Operational && project->active;
}

int main()
{
    Simulation simulation(770031);
    simulation.setupNewGame();
    World& world=simulation.world();
    CHECK(world.characters.size()==4);
    CHECK(world.facilities.empty());

    Character& actor=world.characters[0];
    actor.needs.hunger=0.10;
    actor.needs.thirst=0.10;
    actor.needs.bladder=0.10;
    actor.needs.hygiene=0.10;
    actor.needs.sleep=0.58;

    const GridPos center=world.initialStartRegionCenterGrid();

    // Sleep pressure creates a real SleepingPlace construction plan.
    const double sleepPressure=settlementFacilityNeedPressure(
        world,actor,center,FacilityKind::SleepingPlace);
    CHECK(sleepPressure>=0.24);

    const UnifiedUtilityDecision unifiedSleep=
        chooseUnifiedUtilityDecisionAtPosition(
            world,actor,simulation.relationships(),center,2.0,0.14);
    CHECK(unifiedSleep.kind==UnifiedDecisionKind::Civilization);
    CHECK(unifiedSleep.civilization.facilityKind==FacilityKind::SleepingPlace);
    CHECK(unifiedSleep.civilization.facilityAction==FacilityBuildAction::Plan);

    CivilizationUtilityDecision sleepPlan=
        bestSettlementFoundationDecision(world,actor,center);
    CHECK(sleepPlan.intent==CivilizationIntent::Craft);
    CHECK(sleepPlan.facilityKind==FacilityKind::SleepingPlace);
    CHECK(sleepPlan.facilityAction==FacilityBuildAction::Plan);
    CHECK(sleepPlan.hasFacilityTarget);
    CHECK(civilizationContextRequiresSpatialTarget(sleepPlan));

    GridPos resolved{};
    SanitationSiteId sanitation=0;
    CHECK(resolveCivilizationContextTarget(
        world,actor,sleepPlan,center,resolved,sanitation));
    CHECK(resolved.x==sleepPlan.facilityTargetPos.x);
    CHECK(resolved.y==sleepPlan.facilityTargetPos.y);

    CivilizationExecutionResult planned=
        executeCivilizationDecision(world,actor,sleepPlan);
    CHECK(planned.executed && planned.success);
    CHECK(planned.facilityKind==FacilityKind::SleepingPlace);
    CHECK(settlementFacilityProject(world,FacilityKind::SleepingPlace)!=nullptr);
    CHECK(!hasOperationalSettlementFacility(
        world,FacilityKind::SleepingPlace));

    // Missing project materials create autonomous Gather demand from real nodes.
    world.resourceNodes.erase(
        std::remove_if(
            world.resourceNodes.begin(),
            world.resourceNodes.end(),
            [](const ResourceNode& node){
                return node.material!=MaterialKind::Wood
                    && node.material!=MaterialKind::Fiber;
            }),
        world.resourceNodes.end());
    CHECK(!world.resourceNodes.empty());

    CivilizationUtilityDecision gather=bestGatherDecision(world,actor);
    CHECK(gather.intent==CivilizationIntent::Gather);
    CHECK(settlementConstructionMissingMaterial(world,gather.material)>0);
    const int heldBefore=actor.civilization.inventory.count(
        ItemKind::RawMaterial,gather.material);
    CivilizationExecutionResult gathered=
        executeCivilizationDecision(world,actor,gather);
    CHECK(gathered.executed && gathered.success);
    CHECK(actor.civilization.inventory.count(
        ItemKind::RawMaterial,gather.material)>heldBefore);

    // Once materials exist, the same utility path delivers and works the project.
    bool sawDelivery=false;
    bool sawWork=false;
    ConstructedFacility* sleepProject=
        settlementFacilityProject(world,FacilityKind::SleepingPlace);
    CHECK(sleepProject!=nullptr);
    for(const auto& requirement:sleepProject->requirements){
        const int missing=facilityMissingMaterial(*sleepProject,requirement.material);
        if(missing>0){
            actor.civilization.inventory.add({
                ItemKind::RawMaterial,requirement.material,missing,0.5,1.0});
        }
    }

    for(int step=0;step<32
        && !hasOperationalSettlementFacility(world,FacilityKind::SleepingPlace);
        ++step){
        CivilizationUtilityDecision decision=
            bestSettlementFoundationDecision(world,actor,center);
        CHECK(decision.intent==CivilizationIntent::Craft);
        CHECK(decision.facilityKind==FacilityKind::SleepingPlace);
        CHECK(decision.facilityAction==FacilityBuildAction::DeliverMaterial
            || decision.facilityAction==FacilityBuildAction::Work);
        if(decision.facilityAction==FacilityBuildAction::DeliverMaterial) sawDelivery=true;
        if(decision.facilityAction==FacilityBuildAction::Work) sawWork=true;

        GridPos target{};
        SanitationSiteId targetSanitation=0;
        CHECK(resolveCivilizationContextTarget(
            world,actor,decision,center,target,targetSanitation));
        CHECK(target.x==decision.facilityTargetPos.x);
        CHECK(target.y==decision.facilityTargetPos.y);

        CivilizationExecutionResult result=
            executeCivilizationDecision(world,actor,decision);
        CHECK(result.executed && result.success);
    }
    CHECK(sawDelivery);
    CHECK(sawWork);
    CHECK(hasOperationalSettlementFacility(world,FacilityKind::SleepingPlace));

    // Shelter recognition uses the resident's actual local climate plus
    // accumulated resident Need burden from sustained exposure, rather than
    // reacting to one isolated weather sample. Search deterministic chunks for
    // a sufficiently harsh resident-local climate.
    actor.needs.sleep=0.42;
    actor.needs.thirst=0.36;
    actor.needs.hygiene=0.34;
    GridPos harshPosition=center;
    double harshPressure=0.0;
    for(int radius=1;radius<=96 && harshPressure<0.30;++radius){
        const ChunkCoord start=world.initialStartRegionCoord;
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
            const double pressure=settlementFacilityNeedPressure(
                world,actor,pos,FacilityKind::Shelter);
            if(pressure>harshPressure){
                harshPressure=pressure;
                harshPosition=pos;
            }
        }
    }
    CHECK(harshPressure>=0.24);

    CivilizationUtilityDecision shelterPlan=
        bestSettlementFoundationDecision(world,actor,harshPosition);
    CHECK(shelterPlan.intent==CivilizationIntent::Craft);
    CHECK(shelterPlan.facilityKind==FacilityKind::Shelter);
    CHECK(shelterPlan.facilityAction==FacilityBuildAction::Plan);
    CHECK(executeCivilizationDecision(world,actor,shelterPlan).success);
    CHECK(deliverAllAndComplete(world,actor,FacilityKind::Shelter));
    CHECK(hasOperationalSettlementFacility(world,FacilityKind::Shelter));

    // Repeated real technique use is persistent evidence of work/craft demand,
    // so a WorkSurface becomes valuable without an era/unlock switch.
    actor.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Reproducible,0.95);
    for(int i=0;i<6;++i){
        actor.civilization.knowledge.recordSuccessfulUse(
            TechniqueId::SharpFlake);
    }
    const double workPressure=settlementFacilityNeedPressure(
        world,actor,center,FacilityKind::WorkSurface);
    CHECK(workPressure>=0.24);

    CivilizationUtilityDecision workPlan=
        bestSettlementFoundationDecision(world,actor,center);
    CHECK(workPlan.intent==CivilizationIntent::Craft);
    CHECK(workPlan.facilityKind==FacilityKind::WorkSurface);
    CHECK(workPlan.facilityAction==FacilityBuildAction::Plan);
    CHECK(executeCivilizationDecision(world,actor,workPlan).success);

    // No facility was free: every foundation facility observed above started as
    // an explicit project and required real delivery/work before operation.
    CHECK(world.facilities.size()>=3);

    // C1-E: site selection should form an activity cluster around real existing
    // facilities, while terrain/water suitability can override a marginally
    // closer but physically worse tile.
    Simulation layoutSimulation(991731);
    layoutSimulation.setupNewGame();
    World& layout=layoutSimulation.world();
    layout.resourceNodes.clear();
    layout.storageSites.clear();
    layout.primitiveSanitationSites.clear();
    layout.facilities.clear();
    Character& planner=layout.characters.front();
    const GridPos layoutCenter=layout.initialStartRegionCenterGrid();
    // Spawn is only an entry coordinate. Move the lived activity anchor away
    // from it and prove new settlement infrastructure follows activity instead.
    const GridPos emergentCenter{layoutCenter.x+12,layoutCenter.y};
    ConstructedFacility anchorFacility=makeFacilityConstructionSite(
        1,FacilityKind::SleepingPlace,
        {emergentCenter.x+3,emergentCenter.y},planner.id,layout.minute);
    CHECK(anchorFacility.id!=0);
    for(auto& requirement:anchorFacility.requirements){
        requirement.delivered=requirement.required;
    }
    anchorFacility.constructionWork=anchorFacility.requiredWork;
    CHECK(activateConstructedFacility(anchorFacility,0,layout.minute));
    layout.facilities.push_back(anchorFacility);

    const SettlementFacilitySiteOpportunity clustered=
        chooseSettlementFacilitySite(
            layout,planner.id,FacilityKind::Shelter,emergentCenter);
    CHECK(clustered.available);
    // Immediate overlap is forbidden (<=2). Terrain authority may now prefer a
    // slightly farther site, but the new foundation must remain a local cluster
    // and never occupy an authoritative surface-water footprint.
    CHECK(manhattan(clustered.pos,anchorFacility.pos)>=3);
    CHECK(manhattan(clustered.pos,anchorFacility.pos)<=8);
    CHECK(manhattan(clustered.pos,emergentCenter)<=10);
    CHECK(manhattan(clustered.pos,layoutCenter)>=6);
    const HydrologyFacts clusteredWater=deriveHydrologyFacts(
        layout.genesisIdentity(),
        chunkCoordForGrid(clustered.pos));
    CHECK(!surfaceWaterGroundContainsGrid(clusteredWater,clustered.pos));

    // World-generation v3 uses the continuous terrain field. Verify that the
    // exact same autonomous settlement path chooses a viable physical site on
    // that terrain rather than ignoring slope/hydrology.
    Simulation terrainSimulation(991731,0,3);
    terrainSimulation.setupNewGame();
    World& terrainWorld=terrainSimulation.world();
    terrainWorld.resourceNodes.clear();
    terrainWorld.storageSites.clear();
    terrainWorld.primitiveSanitationSites.clear();
    terrainWorld.facilities.clear();
    Character& terrainPlanner=terrainWorld.characters.front();
    const SettlementFacilitySiteOpportunity terrainSite=
        chooseSettlementFacilitySite(
            terrainWorld,
            terrainPlanner.id,
            FacilityKind::Shelter,
            terrainWorld.initialStartRegionCenterGrid());
    CHECK(terrainSite.available);
    CHECK(settlementTerrainHabitabilityScore(
        terrainWorld,
        terrainSite.pos,
        FacilityKind::Shelter)>-100.0);
    const HydrologyFacts terrainSiteWater=deriveHydrologyFacts(
        terrainWorld.genesisIdentity(),
        chunkCoordForGrid(terrainSite.pos));
    CHECK(!surfaceWaterGroundContainsGrid(
        terrainSiteWater,
        terrainSite.pos));

    std::cout << "Stage C-S1 autonomous settlement recognition + terrain-aware clustering passed\n";
    return 0;
}
