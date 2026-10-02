#include <iostream>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/SettlementNetwork.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character resident(CharacterId id)
{
    Character c;
    c.id=id;
    c.name="Settler"+std::to_string(id);
    c.alive=true;
    c.lifeStage=LifeStage::Adult;
    c.civilization.character=id;
    c.civilization.knowledge.learn(
        TechniqueId::PrimitiveStorage,KnowledgeLevel::Reproducible,0.95);
    c.civilization.knowledge.learn(
        TechniqueId::FireMaking,KnowledgeLevel::Reproducible,0.95);
    c.civilization.knowledge.learn(
        TechniqueId::StoneHammer,KnowledgeLevel::Reproducible,0.95);
    c.civilization.knowledge.learn(
        TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible,0.95);
    return c;
}

static ConstructedFacility operationalFacility(
    FacilityId id,
    FacilityKind kind,
    GridPos pos,
    CharacterId owner,
    StorageId storage=0)
{
    ConstructedFacility facility=
        makeFacilityConstructionSite(id,kind,pos,owner,0);
    for(auto& requirement:facility.requirements){
        requirement.delivered=requirement.required;
    }
    facility.constructionWork=facility.requiredWork;
    const bool active=activateConstructedFacility(facility,storage,1);
    if(!active){
        std::cerr << "failed to activate fixture facility\n";
    }
    return facility;
}

int main()
{
    World world(606101);
    world.characters.clear();
    world.facilities.clear();
    world.storageSites.clear();
    world.resourceNodes.clear();
    world.primitiveSanitationSites.clear();

    Character first=resident(1);
    Character migrant=resident(2);
    world.characters={first,migrant};

    const GridPos firstCenter{0,0};
    const GridPos secondCenter{
        SettlementServiceRadiusGrid*4,
        0
    };

    StorageSite firstStorage;
    firstStorage.id=1;
    firstStorage.pos=firstCenter;
    world.storageSites.push_back(firstStorage);
    world.facilities.push_back(operationalFacility(
        1,FacilityKind::PrimitiveStorage,firstCenter,first.id,1));
    world.facilities.push_back(operationalFacility(
        2,FacilityKind::FirePit,{4,0},first.id));
    world.facilities.push_back(operationalFacility(
        3,FacilityKind::Furnace,{8,0},first.id));

    CHECK(hasOperationalPrimitiveStorage(world));
    CHECK(hasOperationalFirePit(world));
    CHECK(hasOperationalFurnace(world));

    // C6-B: world-global legacy helpers still see the original settlement,
    // but a distant resident has no local service from it.
    CHECK(!hasOperationalPrimitiveStorageNear(world,secondCenter));
    CHECK(!hasOperationalFirePitNear(world,secondCenter));
    CHECK(!hasOperationalFurnaceNear(world,secondCenter));

    const PrimitiveStorageSiteOpportunity storageSite=
        choosePrimitiveStorageSite(world,migrant.id,secondCenter);
    CHECK(storageSite.available);
    CHECK(manhattan(storageSite.pos,firstCenter)>SettlementServiceRadiusGrid);
    ConstructedFacility* secondStorageProject=
        establishPrimitiveStorageProject(
            world,migrant.id,storageSite.pos);
    CHECK(secondStorageProject!=nullptr);
    const FacilityId secondStorageId=secondStorageProject->id;
    CHECK(secondStorageId>3);

    const PrimitiveFirePitSiteOpportunity fireSite=
        choosePrimitiveFirePitSite(world,migrant.id,secondCenter);
    CHECK(fireSite.available);
    CHECK(manhattan(fireSite.pos,firstCenter)>SettlementServiceRadiusGrid);
    ConstructedFacility* secondFireProject=
        establishPrimitiveFirePitProject(
            world,migrant,fireSite.pos);
    CHECK(secondFireProject!=nullptr);
    const FacilityId secondFireId=secondFireProject->id;

    // Complete only the local fire fixture so metallurgy can recognize a
    // genuinely local heat base. The distant first settlement's fire is not
    // enough to authorize a furnace here.
    for(auto& requirement:secondFireProject->requirements){
        requirement.delivered=requirement.required;
    }
    secondFireProject->constructionWork=secondFireProject->requiredWork;
    CHECK(activateConstructedFacility(
        *secondFireProject,0,world.minute));
    CHECK(hasOperationalFirePitForSmeltingNear(
        world,secondCenter));

    const PrimitiveFurnaceSiteOpportunity furnaceSite=
        choosePrimitiveFurnaceSite(world,migrant.id,secondCenter);
    CHECK(furnaceSite.available);
    CHECK(manhattan(furnaceSite.pos,firstCenter)>SettlementServiceRadiusGrid);
    ConstructedFacility* secondFurnaceProject=
        establishPrimitiveFurnaceProject(
            world,migrant,furnaceSite.pos);
    CHECK(secondFurnaceProject!=nullptr);

    // Exact facility-id work prevents one settlement from accidentally
    // progressing another settlement's project of the same kind.
    for(auto& facility:world.facilities){
        if(facility.id!=secondStorageId) continue;
        for(auto& requirement:facility.requirements){
            requirement.delivered=requirement.required;
        }
    }
    const std::size_t storagesBefore=world.storageSites.size();
    const PrimitiveStorageWorkResult storageWork=
        workOnPrimitiveStorage(
            world,migrant,secondStorageId,1000.0);
    CHECK(storageWork.worked);
    CHECK(storageWork.completed);
    CHECK(world.storageSites.size()==storagesBefore+1);
    CHECK(hasOperationalPrimitiveStorageNear(
        world,secondCenter));

    SettlementPopulation population{
        {first.id,firstCenter},
        {migrant.id,secondCenter}
    };
    const SettlementNetworkObservation network=
        observeSettlementNetwork(world,&population);
    CHECK(network.settlementCount==2);
    CHECK(network.activeSettlementCount==2);
    CHECK(network.residentAssignedCount==2);
    CHECK(network.settlements.size()==2);

    bool sawFirst=false;
    bool sawSecond=false;
    for(const SettlementClusterObservation& settlement:network.settlements){
        CHECK(settlement.residentCount==1);
        CHECK(settlement.established);
        if(manhattan(settlement.anchor,firstCenter)
            <=SettlementServiceRadiusGrid){
            sawFirst=true;
            CHECK(settlement.storageSiteCount>=1);
            CHECK(settlement.operationalFacilityCount>=3);
        }
        if(manhattan(settlement.anchor,secondCenter)
            <=SettlementServiceRadiusGrid){
            sawSecond=true;
            CHECK(settlement.storageSiteCount>=1);
            CHECK(settlement.operationalFacilityCount>=2);
            CHECK(settlement.plannedFacilityCount>=1);
        }
    }
    CHECK(sawFirst);
    CHECK(sawSecond);

    std::cout
        << "C6-B local infrastructure and multiple settlements passed\n";
    return 0;
}
