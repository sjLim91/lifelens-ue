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
    // This isolated shelter scenario is far from the completed bed. Keep sleep
    // low so legitimate demand for a local bed doesn't mask weather exposure.
    actor.needs.sleep=0.10;
    actor.needs.thirst=0.36;
    actor.needs.hygiene=0.42;
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

    // C-S4: an actually used place becomes a stronger activity center than an
    // identical but merely existing facility. This is persisted Core evidence,
    // not a client-side "town center" tag.
    const int safeShelterDistance=facilityMinimumCenterDistanceGrid(
        FacilityKind::Shelter,FacilityKind::SleepingPlace);
    const GridPos activityScoreProbe{
        anchorFacility.pos.x+safeShelterDistance,
        anchorFacility.pos.y
    };
    const double unusedActivityScore=settlementActivityCenterScore(
        layout,activityScoreProbe,FacilityKind::Shelter);
    for(int use=0;use<8;++use){
        CHECK(recordFacilityUse(
            layout.facilities.front(),planner.id,layout.minute+use));
    }
    const double livedActivityScore=settlementActivityCenterScore(
        layout,activityScoreProbe,FacilityKind::Shelter);
    CHECK(livedActivityScore>unusedActivityScore);

    SettlementPopulation livedPopulation;
    for(std::size_t index=0;index<layout.characters.size();++index){
        livedPopulation.emplace(
            layout.characters[index].id,
            GridPos{emergentCenter.x+static_cast<int>(index),emergentCenter.y});
    }
    CHECK(settlementResidentActivityScore(
        layout,emergentCenter,&livedPopulation)>0.0);
    CHECK(settlementResidentActivityScore(
        layout,{emergentCenter.x+64,emergentCenter.y},&livedPopulation)==0.0);

    // Runtime coordinates alone must not leave a dead resident behind as a
    // phantom activity center.
    const CharacterId deadResidentId=layout.characters.back().id;
    const bool deadResidentWasAlive=layout.characters.back().alive;
    const GridPos deadResidentOriginalPos=livedPopulation.at(deadResidentId);
    const GridPos ghostProbe{emergentCenter.x+80,emergentCenter.y};
    layout.characters.back().alive=false;
    livedPopulation[deadResidentId]=ghostProbe;
    CHECK(settlementResidentActivityScore(
        layout,ghostProbe,&livedPopulation)==0.0);
    layout.characters.back().alive=deadResidentWasAlive;
    livedPopulation[deadResidentId]=deadResidentOriginalPos;

    const SettlementFacilitySiteOpportunity clustered=
        chooseSettlementFacilitySite(
            layout,planner.id,FacilityKind::Shelter,emergentCenter);
    CHECK(clustered.available);
    // Functional affinity may still cluster related facilities, but the
    // physical footprint contract is a hard boundary: "near" must never mean
    // interpenetrating structures.
    CHECK(!facilityFootprintsConflict(
        FacilityKind::Shelter,clustered.pos,
        anchorFacility.kind,anchorFacility.pos));
    CHECK(manhattan(clustered.pos,anchorFacility.pos)<=20);
    CHECK(manhattan(clustered.pos,emergentCenter)<=16);
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

    // Settlement-economy logistics: a shared stockpile must be able to feed
    // construction and restoration. Stored material is not "gone" from the
    // economy, and it must not trigger redundant gathering while a capable
    // resident can retrieve and deliver it.
    Simulation logisticsSimulation(220931);
    logisticsSimulation.setupNewGame();
    World& logisticsWorld=logisticsSimulation.world();
    logisticsWorld.facilities.clear();
    logisticsWorld.storageSites.clear();
    Character& logisticsWorker=logisticsWorld.characters.front();
    logisticsWorker.needs={0.10,0.10,0.58,0.10,0.10};
    logisticsWorker.civilization.inventory=Inventory{};

    const GridPos logisticsAnchor=
        logisticsWorld.initialStartRegionCenterGrid();
    StorageSite sharedStorage;
    sharedStorage.id=1;
    sharedStorage.pos={logisticsAnchor.x+2,logisticsAnchor.y};
    logisticsWorld.storageSites.push_back(sharedStorage);

    ConstructedFacility storageFacility=makeFacilityConstructionSite(
        1,
        FacilityKind::PrimitiveStorage,
        logisticsWorld.storageSites.front().pos,
        logisticsWorker.id,
        logisticsWorld.minute);
    CHECK(storageFacility.id!=0);
    for(auto& requirement:storageFacility.requirements){
        requirement.delivered=requirement.required;
    }
    storageFacility.constructionWork=storageFacility.requiredWork;
    CHECK(activateConstructedFacility(
        storageFacility,
        logisticsWorld.storageSites.front().id,
        logisticsWorld.minute));
    logisticsWorld.facilities.push_back(storageFacility);

    const SettlementFacilitySiteOpportunity logisticsBedSite=
        chooseSettlementFacilitySite(
            logisticsWorld,
            logisticsWorker.id,
            FacilityKind::SleepingPlace,
            logisticsAnchor);
    CHECK(logisticsBedSite.available);
    ConstructedFacility* logisticsBed=
        establishSettlementFacilityProject(
            logisticsWorld,
            logisticsWorker.id,
            FacilityKind::SleepingPlace,
            logisticsBedSite.pos);
    CHECK(logisticsBed!=nullptr);

    // Put the exact committed construction package into the shared store.
    // Uncovered demand must become zero even though the worker carries none.
    for(const auto& requirement:logisticsBed->requirements){
        const int missing=facilityMissingMaterial(
            *logisticsBed,requirement.material);
        CHECK(missing>0);
        logisticsWorld.storageSites.front().inventory.add({
            ItemKind::RawMaterial,
            requirement.material,
            missing,
            0.6,
            1.0});
        CHECK(settlementUncoveredMaterialDemand(
            logisticsWorld,requirement.material)==0);
    }

    // Repeatedly withdraw only what the live project still needs, then deliver
    // it at the actual construction site.
    int logisticsTransfers=0;
    int logisticsDeliveries=0;
    for(int step=0;step<24 && !facilityMaterialsComplete(*logisticsBed);++step){
        // If the worker already carries something the live project needs, use
        // it first. Returning to storage while useful material is still in the
        // worker's hands would create pointless logistics churn.
        const CivilizationUtilityDecision delivery=
            bestSettlementFoundationDecision(
                logisticsWorld,
                logisticsWorker,
                logisticsBed->pos);
        if(delivery.intent==CivilizationIntent::Craft
           && delivery.facility==logisticsBed->id
           && delivery.facilityAction==FacilityBuildAction::DeliverMaterial){
            CHECK(logisticsWorker.civilization.inventory.count(
                ItemKind::RawMaterial,delivery.material)>0);
            const CivilizationExecutionResult delivered=
                executeCivilizationDecisionAtPosition(
                    logisticsWorld,
                    logisticsWorker,
                    delivery,
                    logisticsBed->pos);
            CHECK(delivered.executed && delivered.success);
            ++logisticsDeliveries;
        }else{
            const CivilizationUtilityDecision retrieve=
                bestRetrieveDecisionAtPosition(
                    logisticsWorld,
                    logisticsWorker,
                    logisticsWorld.storageSites.front().pos);
            CHECK(retrieve.intent==CivilizationIntent::Retrieve);
            CHECK(retrieve.storage==logisticsWorld.storageSites.front().id);
            CHECK(retrieve.material!=MaterialKind::Unknown);
            CHECK(residentCommittedMaterialDemand(
                logisticsWorld,
                logisticsWorker,
                retrieve.material)>0);

            const CivilizationExecutionResult retrieved=
                executeCivilizationDecisionAtPosition(
                    logisticsWorld,
                    logisticsWorker,
                    retrieve,
                    logisticsWorld.storageSites.front().pos);
            CHECK(retrieved.executed && retrieved.success);
            ++logisticsTransfers;
        }

        logisticsBed=findCivilizationFacility(
            logisticsWorld,logisticsBed->id);
        CHECK(logisticsBed!=nullptr);
    }
    CHECK(logisticsTransfers>0);
    CHECK(logisticsDeliveries>0);
    CHECK(facilityMaterialsComplete(*logisticsBed));

    int logisticsWorkActions=0;
    while(logisticsBed->state!=FacilityState::Operational
          && logisticsWorkActions<16){
        const CivilizationUtilityDecision work=
            bestSettlementFoundationDecision(
                logisticsWorld,
                logisticsWorker,
                logisticsBed->pos);
        CHECK(work.facility==logisticsBed->id);
        CHECK(work.facilityAction==FacilityBuildAction::Work);
        const CivilizationExecutionResult worked=
            executeCivilizationDecisionAtPosition(
                logisticsWorld,
                logisticsWorker,
                work,
                logisticsBed->pos);
        CHECK(worked.executed && worked.success);
        ++logisticsWorkActions;
        logisticsBed=findCivilizationFacility(
            logisticsWorld,logisticsBed->id);
        CHECK(logisticsBed!=nullptr);
    }
    CHECK(logisticsBed->state==FacilityState::Operational);

    // Restoration uses the same shared logistics path. A ruined bed should
    // consume its cheaper restoration package from storage before any new bed
    // can be planned.
    CHECK(ruinConstructedFacility(*logisticsBed));
    logisticsWorker.civilization.inventory=Inventory{};
    const auto restoreRequirements=
        facilityRestorationRequirements(FacilityKind::SleepingPlace);
    CHECK(!restoreRequirements.empty());
    for(const auto& requirement:restoreRequirements){
        logisticsWorld.storageSites.front().inventory.add({
            ItemKind::RawMaterial,
            requirement.material,
            requirement.required,
            0.6,
            1.0});
        CHECK(settlementUncoveredMaterialDemand(
            logisticsWorld,requirement.material)==0);
    }

    int restorationTransfers=0;
    while(restorationTransfers<16){
        bool hasAll=true;
        for(const auto& requirement:restoreRequirements){
            if(logisticsWorker.civilization.inventory.count(
                ItemKind::RawMaterial,
                requirement.material)<requirement.required){
                hasAll=false;
                break;
            }
        }
        if(hasAll) break;

        const CivilizationUtilityDecision retrieve=
            bestRetrieveDecisionAtPosition(
                logisticsWorld,
                logisticsWorker,
                logisticsWorld.storageSites.front().pos);
        CHECK(retrieve.intent==CivilizationIntent::Retrieve);
        CHECK(retrieve.storage==logisticsWorld.storageSites.front().id);
        const CivilizationExecutionResult retrieved=
            executeCivilizationDecisionAtPosition(
                logisticsWorld,
                logisticsWorker,
                retrieve,
                logisticsWorld.storageSites.front().pos);
        CHECK(retrieved.executed && retrieved.success);
        ++restorationTransfers;
    }
    CHECK(restorationTransfers>0);

    const CivilizationUtilityDecision restore=
        bestSettlementFoundationDecision(
            logisticsWorld,
            logisticsWorker,
            logisticsBed->pos);
    CHECK(restore.intent==CivilizationIntent::Craft);
    CHECK(restore.facility==logisticsBed->id);
    CHECK(restore.facilityAction==FacilityBuildAction::Repair);
    const CivilizationExecutionResult restoredBed=
        executeCivilizationDecisionAtPosition(
            logisticsWorld,
            logisticsWorker,
            restore,
            logisticsBed->pos);
    CHECK(restoredBed.executed && restoredBed.success);
    CHECK(logisticsBed->state==FacilityState::Operational);

    std::cout << "Stage C-S5 shared settlement logistics + lived-use settlement form passed\n";
    return 0;
}
