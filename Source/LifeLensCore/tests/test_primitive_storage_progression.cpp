#include <iostream>
#include <string>
#include <vector>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/ContextAction.h"
#include "lifelens/PrimitiveStorageProgression.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static void markOtherTechniquesKnown(Character& resident)
{
    const TechniqueId techniques[]={
        TechniqueId::SharpFlake,
        TechniqueId::ChippedStoneTool,
        TechniqueId::FireMaking,
        TechniqueId::FiberCordage,
        TechniqueId::SimpleContainer,
        TechniqueId::DesignatedSanitationArea,
        TechniqueId::DugSanitationPit
    };
    for(const TechniqueId technique:techniques){
        resident.civilization.knowledge.learn(
            technique,KnowledgeLevel::Reproducible,0.9);
    }
}

int main()
{
    Simulation simulation(808080,404040);
    simulation.setupNewGame();
    World& world=simulation.world();
    CHECK(world.storageSites.empty());
    CHECK(world.facilities.empty());
    CHECK(!world.characters.empty());

    Character& resident=world.characters.front();
    markOtherTechniquesKnown(resident);

    // The idea must not appear before the resident has actually experienced
    // carrying pressure.
    CHECK(!observePrimitiveStorageNeed(world,resident).recognized);
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Wood,4,0.5,1.0});
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Fiber,2,0.5,1.0});
    const PrimitiveStorageNeedObservation need=observePrimitiveStorageNeed(world,resident);
    CHECK(need.recognized);
    CHECK(need.carriedUnits==6);

    // With every other v1 experiment already known, storage pressure must
    // produce the stockpile-organization experiment rather than a free unlock.
    CivilizationUtilityDecision discovery=bestExperimentDecision(world,resident);
    CHECK(discovery.intent==CivilizationIntent::Experiment);
    CHECK(discovery.experiment==ExperimentKind::OrganizeStockpile);
    CHECK(discovery.technique==TechniqueId::PrimitiveStorage);

    bool discovered=false;
    for(int attempt=0;attempt<120 && !discovered;++attempt){
        discovery=bestExperimentDecision(world,resident);
        CHECK(discovery.experiment==ExperimentKind::OrganizeStockpile);
        const CivilizationExecutionResult result=
            executeCivilizationDecision(world,resident,discovery);
        CHECK(result.executed);
        discovered=result.success;
        ++world.minute;
    }
    CHECK(discovered);
    CHECK(resident.civilization.knowledge.knowsAtLeast(
        TechniqueId::PrimitiveStorage,KnowledgeLevel::Reproducible));
    CHECK(world.facilities.empty());
    CHECK(world.storageSites.empty());

    const GridPos activityAnchor=world.hasInitialStartRegionSelection
        ? world.initialStartRegionCenterGrid()
        : GridPos{};

    // Discovery only grants know-how. Planning a real Core position is a
    // separate spatial action and still creates no usable storage by itself.
    CivilizationUtilityDecision plan=
        bestPrimitiveStorageConstructionDecision(world,resident,activityAnchor);
    CHECK(plan.intent==CivilizationIntent::Craft);
    CHECK(plan.technique==TechniqueId::PrimitiveStorage);
    CHECK(plan.facilityAction==FacilityBuildAction::Plan);
    CHECK(plan.hasFacilityTarget);

    GridPos resolved{};
    SanitationSiteId sanitationSite=0;
    CHECK(resolveCivilizationContextTarget(
        world,resident,plan,activityAnchor,resolved,sanitationSite));
    CHECK(resolved.x==plan.facilityTargetPos.x);
    CHECK(resolved.y==plan.facilityTargetPos.y);
    CHECK(civilizationContextRequiresSpatialTarget(plan));

    CivilizationExecutionResult planned=
        executeCivilizationDecision(world,resident,plan);
    CHECK(planned.executed && planned.success);
    CHECK(planned.facilityId!=0);
    CHECK(world.facilities.size()==1);
    CHECK(world.storageSites.empty());
    CHECK(world.facilities[0].state==FacilityState::Planned);

    // Material delivery is real inventory transfer at the facility position.
    int deliveredWood=0;
    int deliveredFiber=0;
    for(int i=0;i<8 && !facilityMaterialsComplete(world.facilities[0]);++i){
        CivilizationUtilityDecision delivery=
            bestPrimitiveStorageConstructionDecision(world,resident,activityAnchor);
        CHECK(delivery.facilityAction==FacilityBuildAction::DeliverMaterial);
        CHECK(delivery.facility==world.facilities[0].id);
        CHECK(resolveCivilizationContextTarget(
            world,resident,delivery,activityAnchor,resolved,sanitationSite));
        const int woodBefore=resident.civilization.inventory.count(
            ItemKind::RawMaterial,MaterialKind::Wood);
        const int fiberBefore=resident.civilization.inventory.count(
            ItemKind::RawMaterial,MaterialKind::Fiber);
        const CivilizationExecutionResult result=
            executeCivilizationDecision(world,resident,delivery);
        CHECK(result.executed && result.success);
        const int woodAfter=resident.civilization.inventory.count(
            ItemKind::RawMaterial,MaterialKind::Wood);
        const int fiberAfter=resident.civilization.inventory.count(
            ItemKind::RawMaterial,MaterialKind::Fiber);
        deliveredWood+=woodBefore-woodAfter;
        deliveredFiber+=fiberBefore-fiberAfter;
    }
    CHECK(facilityMaterialsComplete(world.facilities[0]));
    CHECK(deliveredWood==4);
    CHECK(deliveredFiber==2);
    CHECK(resident.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood)==0);
    CHECK(resident.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Fiber)==0);
    CHECK(world.storageSites.empty());

    // Construction takes repeated work ACKs. Only the completion boundary
    // creates the StorageSite and makes the facility operational.
    int workActions=0;
    while(!hasOperationalPrimitiveStorage(world) && workActions<16){
        CivilizationUtilityDecision work=
            bestPrimitiveStorageConstructionDecision(world,resident,activityAnchor);
        CHECK(work.facilityAction==FacilityBuildAction::Work);
        CHECK(work.facilityWork>0.0);
        CHECK(resolveCivilizationContextTarget(
            world,resident,work,activityAnchor,resolved,sanitationSite));
        const CivilizationExecutionResult result=
            executeCivilizationDecision(world,resident,work);
        CHECK(result.executed && result.success);
        ++workActions;
    }
    CHECK(workActions>1);
    CHECK(hasOperationalPrimitiveStorage(world));
    CHECK(world.facilities[0].state==FacilityState::Operational);
    CHECK(world.facilities[0].active);
    CHECK(world.storageSites.size()==1);
    CHECK(world.facilities[0].linkedStorage==world.storageSites[0].id);
    CHECK(world.facilities[0].pos.x==world.storageSites[0].pos.x);
    CHECK(world.facilities[0].pos.y==world.storageSites[0].pos.y);

    // Once operational, existing Store utility becomes naturally reachable.
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Wood,9,0.5,1.0});
    const CivilizationUtilityDecision store=bestStoreDecision(world,resident);
    CHECK(store.intent==CivilizationIntent::Store);
    CHECK(store.storage==world.storageSites[0].id);

    // New technique knowledge and the completed facility/storage linkage must
    // survive the normal snapshot codec without inventing a second authority.
    const SimulationStateSnapshot snapshot=simulation.captureSnapshot();
    std::vector<std::uint8_t> bytes;
    std::string error;
    CHECK(encodeSimulationSnapshot(snapshot,bytes,&error));
    CHECK(error.empty());

    SimulationStateSnapshot decoded;
    CHECK(decodeSimulationSnapshot(bytes,decoded,&error));
    CHECK(error.empty());
    CHECK(decoded.world.facilities.size()==1);
    CHECK(decoded.world.storageSites.size()==1);
    CHECK(decoded.world.facilities[0].linkedStorage==decoded.world.storageSites[0].id);
    CHECK(decoded.world.characters[0].civilization.knowledge.knowsAtLeast(
        TechniqueId::PrimitiveStorage,KnowledgeLevel::Reproducible));

    std::cout << "primitive storage progression passed\n";
    return 0;
}
