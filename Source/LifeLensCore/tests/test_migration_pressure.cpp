#include <iostream>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/ContextAction.h"
#include "lifelens/MigrationPressure.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeResident(CharacterId id)
{
    Character resident;
    resident.id=id;
    resident.name="Migrant";
    resident.alive=true;
    resident.lifeStage=LifeStage::Adult;
    resident.civilization.character=id;
    resident.personality.curiosity=0.82;
    resident.personality.adaptability=0.78;
    resident.needs.hunger=0.24;
    resident.needs.thirst=0.94;
    resident.needs.sleep=0.20;
    resident.needs.bladder=0.20;
    resident.needs.hygiene=0.20;
    return resident;
}

static void addResource(
    World& world,
    ResourceNodeId id,
    MaterialKind material,
    int quantity,
    GridPos pos)
{
    ResourceNode node;
    node.id=id;
    node.material=material;
    node.quantity=quantity;
    node.maxQuantity=quantity;
    node.renewable=false;
    node.regenerationPerDay=0;
    node.pos=pos;
    world.resourceNodes.push_back(node);
}

int main()
{
    const GridPos anchor{0,0};

    // A locally supplied camp has no reason to become a migration candidate.
    World supplied(606001);
    supplied.characters.clear();
    supplied.resourceNodes.clear();
    supplied.storageSites.clear();
    supplied.facilities.clear();
    supplied.generatedNaturalChunks.clear();
    supplied.characters.push_back(makeResident(1));

    addResource(supplied,1,MaterialKind::Water,32,{2,0});
    addResource(supplied,2,MaterialKind::PlantFood,32,{3,0});
    addResource(supplied,3,MaterialKind::Wood,24,{4,0});
    addResource(supplied,4,MaterialKind::Stone,20,{5,0});
    addResource(supplied,5,MaterialKind::Fiber,20,{6,0});
    addResource(supplied,6,MaterialKind::Clay,16,{7,0});

    SettlementPopulation suppliedPopulation{{1,anchor}};
    const MigrationPressureObservation stable=
        observeMigrationPressure(
            supplied,supplied.characters.front(),anchor,&suppliedPopulation);
    CHECK(!stable.candidate);
    CHECK(stable.resourceScarcity01<0.10);
    CHECK(stable.pressure01<MigrationCandidatePressureThreshold);

    // C6-A: exhaust a fully explored six-chunk neighborhood. The Core must
    // distinguish "there may be resources somewhere" from "this lived area is
    // locally depleted", escalate to a farther frontier, and surface a
    // migration candidate rather than fabricating a new settlement.
    World scarce(606002);
    scarce.characters.clear();
    scarce.resourceNodes.clear();
    scarce.storageSites.clear();
    scarce.facilities.clear();
    scarce.generatedNaturalChunks.clear();
    scarce.characters.push_back(makeResident(1));

    const ChunkCoord center=chunkCoordForGrid(anchor);
    for(int dx=-ResourceExplorationMaxRadiusChunks;
        dx<=ResourceExplorationMaxRadiusChunks;++dx){
        for(int dy=-ResourceExplorationMaxRadiusChunks;
            dy<=ResourceExplorationMaxRadiusChunks;++dy){
            scarce.materializeNaturalChunk(
                {center.x+dx,center.y+dy});
        }
    }
    for(ResourceNode& node:scarce.resourceNodes){
        node.quantity=0;
    }

    SettlementPopulation scarcePopulation{{1,anchor}};
    const MigrationPressureObservation pressure=
        observeMigrationPressure(
            scarce,scarce.characters.front(),anchor,&scarcePopulation);

    CHECK(pressure.bottleneckMaterial==MaterialKind::Water);
    CHECK(pressure.resourceScarcity01>0.95);
    CHECK(pressure.travelBurden01>=0.69);
    CHECK(pressure.pressure01>=MigrationCandidatePressureThreshold);
    CHECK(pressure.candidate);
    CHECK(pressure.hasFrontierTarget);
    CHECK(pressure.frontierDistanceChunks>=7);

    const CivilizationUtilityDecision exploration=
        bestResourceExplorationDecisionAtPosition(
            scarce,scarce.characters.front(),anchor,&scarcePopulation);
    CHECK(exploration.intent==CivilizationIntent::Explore);
    CHECK(exploration.material==MaterialKind::Water);

    GridPos resolved{};
    SanitationSiteId sanitation=0;
    CHECK(resolveCivilizationContextTarget(
        scarce,
        scarce.characters.front(),
        exploration,
        anchor,
        resolved,
        sanitation,
        &scarcePopulation));
    CHECK(sanitation==0);
    const ChunkCoord resolvedChunk=chunkCoordForGrid(resolved);
    CHECK(std::max(
        std::abs(resolvedChunk.x-center.x),
        std::abs(resolvedChunk.y-center.y))>=7);
    CHECK(resolved.x==pressure.frontierTarget.x);
    CHECK(resolved.y==pressure.frontierTarget.y);

    // Reaching the frontier still only materializes the explored natural
    // chunk. C6-A must not teleport the resident or conjure a settlement.
    const std::size_t facilitiesBefore=scarce.facilities.size();
    const std::size_t storagesBefore=scarce.storageSites.size();
    CHECK(scarce.findGeneratedNaturalChunk(resolvedChunk)==nullptr);
    const CivilizationExecutionResult explored=
        executeCivilizationDecisionAtPosition(
            scarce,
            scarce.characters.front(),
            exploration,
            resolved,
            &scarcePopulation);
    CHECK(explored.executed && explored.success);
    CHECK(explored.event.type==CivilizationEventType::Explored);
    CHECK(scarce.findGeneratedNaturalChunk(resolvedChunk)!=nullptr);
    CHECK(scarce.facilities.size()==facilitiesBefore);
    CHECK(scarce.storageSites.size()==storagesBefore);

    // Existing infrastructure acts as real attachment/friction. It can reduce
    // migration pressure, but it does not erase the underlying scarcity fact.
    World attached=scarce;
    for(int index=0;index<4;++index){
        ConstructedFacility facility;
        facility.id=100+index;
        facility.kind=FacilityKind::Shelter;
        facility.state=FacilityState::Operational;
        facility.active=true;
        facility.durability=1.0;
        facility.pos={anchor.x+index,anchor.y+2};
        attached.facilities.push_back(facility);
    }
    const MigrationPressureObservation attachedPressure=
        observeMigrationPressure(
            attached,attached.characters.front(),anchor,&scarcePopulation);
    CHECK(attachedPressure.settlementAttachment01>0.90);
    CHECK(attachedPressure.pressure01<pressure.pressure01-0.12);
    CHECK(attachedPressure.resourceScarcity01>0.95);

    std::cout
        << "C6-A scarcity -> long-range exploration -> migration pressure passed\n";
    return 0;
}
