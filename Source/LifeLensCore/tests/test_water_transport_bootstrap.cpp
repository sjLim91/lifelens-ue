#include <cassert>
#include <iostream>

#include "lifelens/CivilizationDecision.h"

using namespace lifelens;

static ResourceNode node(
    ResourceNodeId id,
    MaterialKind material,
    int quantity,
    GridPos pos)
{
    ResourceNode value;
    value.id=id;
    value.material=material;
    value.quantity=quantity;
    value.maxQuantity=quantity;
    value.pos=pos;
    return value;
}

int main()
{
    World world(773311);
    world.resourceNodes.clear();
    world.storageSites.clear();
    world.facilities.clear();
    world.primitiveSanitationSites.clear();

    world.resourceNodes.push_back(
        node(101,MaterialKind::Water,200,{40,0}));
    world.resourceNodes.push_back(
        node(102,MaterialKind::Clay,80,{4,0}));

    Character resident;
    resident.id=1;
    resident.name="Carrier";
    resident.civilization.character=resident.id;
    resident.needs={0.10,0.55,0.10,0.10,0.45};
    resident.personality.curiosity=0.60;
    resident.personality.openness=0.60;
    resident.personality.patience=0.55;
    resident.personality.adaptability=0.55;
    resident.civilization.learningSkill=0.55;
    resident.civilization.gatheringSkill=0.55;

    const GridPos home{0,0};

    assert(PortableProvisionCarryTarget==2);
    assert(SimpleContainerPersonalStockTarget==
        PortableProvisionCarryTarget);
    assert(SettlementWaterReserveTarget==8);
    assert(desiredTechniqueOutputStock(TechniqueId::SimpleContainer)==
        SimpleContainerPersonalStockTarget);

    // A resident repeatedly walking more than a chunk for water has a real
    // transport problem, but the pressure only changes priorities. It does not
    // grant knowledge, a container, or a successful experiment.
    const double pressure=
        waterTransportInnovationPressure(world,resident,home);
    assert(pressure>0.75);
    assert(simpleContainerCount(resident.civilization.inventory)==0);
    assert(!resident.civilization.knowledge.knowsAtLeast(
        TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible));

    // Before the resident can experiment, the same observed burden makes Clay
    // a meaningful gather target instead of leaving all non-survival time to
    // unrelated materials.
    const CivilizationUtilityDecision gather=
        bestGatherDecisionAtPosition(world,resident,home);
    assert(gather.intent==CivilizationIntent::Gather);
    assert(gather.material==MaterialKind::Clay);
    assert(gather.resourceNode==102);

    resident.civilization.inventory.add({
        ItemKind::RawMaterial,
        MaterialKind::Clay,
        3,
        0.5,
        1.0});

    const CivilizationUtilityDecision experiment=
        bestExperimentDecisionAtPosition(world,resident,home,nullptr);
    assert(experiment.intent==CivilizationIntent::Experiment);
    assert(experiment.experiment==ExperimentKind::ShapeClay);
    assert(experiment.technique==TechniqueId::SimpleContainer);

    // Owning a reusable empty vessel solves the innovation problem even before
    // it is filled. Ordinary Water Gather/Drink now owns the refill loop.
    resident.civilization.inventory.add({
        ItemKind::SimpleContainer,
        MaterialKind::Clay,
        1,
        0.5,
        1.0});
    resident.civilization.knowledge.learn(
        TechniqueId::SimpleContainer,
        KnowledgeLevel::Reproducible,
        0.8);
    assert(waterTransportInnovationPressure(
        world,resident,home)==0.0);
    assert(simpleContainerLogisticsStockPressure(
        world,resident,home)>0.45);

    // Innovation and scaling are separate. One vessel solves the discovery
    // problem, but a real stock gap keeps Clay acquisition relevant until the
    // resident can carry a reserve and contribute surplus to shared storage.
    while(resident.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Clay,1)) {}
    resident.needs.thirst=0.20;
    const CivilizationUtilityDecision logisticsGather=
        bestGatherDecisionAtPosition(world,resident,home);
    assert(logisticsGather.intent==CivilizationIntent::Gather);
    assert(logisticsGather.material==MaterialKind::Clay);

    resident.civilization.inventory.add({
        ItemKind::RawMaterial,
        MaterialKind::Clay,
        3,
        0.5,
        1.0});
    const CivilizationUtilityDecision logisticsCraft=
        bestCraftDecisionAtPosition(world,resident,home,nullptr);
    assert(logisticsCraft.intent==CivilizationIntent::Craft);
    assert(logisticsCraft.technique==TechniqueId::SimpleContainer);

    // Once a real storage exists, the logistics target expands from personal
    // carry capacity to a shared reserve. Empty containers are never banked on
    // their own; a resident fills surplus capacity first, then stores only the
    // filled Water+container pair while retaining the personal carry target.
    StorageSite reserveStorage;
    reserveStorage.id=500;
    reserveStorage.pos={2,0};
    world.storageSites.push_back(reserveStorage);
    assert(simpleContainerLogisticsStockPressure(
        world,resident,home)>0.80);
    assert(settlementContainerCapitalAtPosition(
        world,resident,home)==1);

    while(resident.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Clay,1)) {}
    resident.civilization.inventory.add({
        ItemKind::SimpleContainer,
        MaterialKind::Clay,
        2,
        0.5,
        1.0});
    assert(simpleContainerCount(resident.civilization.inventory)==3);

    const CivilizationUtilityDecision noEmptyContainerStore=
        bestStoreDecisionAtPosition(world,resident,home);
    assert(noEmptyContainerStore.intent==CivilizationIntent::None);

    ResourceNode* waterNode=nullptr;
    for(ResourceNode& resource:world.resourceNodes){
        if(resource.material==MaterialKind::Water){
            waterNode=&resource;
            break;
        }
    }
    assert(waterNode!=nullptr);
    const CivilizationEvent filled=
        gatherResource(resident.civilization,*waterNode,3);
    assert(filled.quantity==3);
    assert(portableWaterCount(resident.civilization.inventory)==3);

    const CivilizationUtilityDecision bankWater=
        bestStoreDecisionAtPosition(world,resident,home);
    assert(bankWater.intent==CivilizationIntent::Store);
    assert(bankWater.material==MaterialKind::Water);
    assert(bankWater.quantity==1);

    const CivilizationExecutionResult banked=
        executeCivilizationDecisionAtPosition(
            world,resident,bankWater,reserveStorage.pos);
    assert(banked.executed && banked.success);
    assert(portableWaterCount(resident.civilization.inventory)==
        PortableProvisionCarryTarget);
    assert(portableWaterCount(world.storageSites.front().inventory)==1);
    assert(simpleContainerLogisticsStockPressure(
        world,resident,home)>0.60);

    world.storageSites.front().inventory.add({
        ItemKind::SimpleContainer,
        MaterialKind::Clay,
        SettlementWaterReserveTarget-1,
        0.5,
        1.0});
    world.storageSites.front().inventory.add({
        ItemKind::RawMaterial,
        MaterialKind::Water,
        SettlementWaterReserveTarget-1,
        0.5,
        1.0});
    assert(portableWaterCount(world.storageSites.front().inventory)==
        SettlementWaterReserveTarget);
    assert(settlementContainerCapitalAtPosition(
        world,resident,home)==
        SimpleContainerPersonalStockTarget+SettlementWaterReserveTarget);
    assert(simpleContainerLogisticsStockPressure(
        world,resident,home)==0.0);

    // Using the shared Water reserve must not be interpreted as destruction of
    // the reusable vessel capital. Moving a filled vessel back to the resident
    // keeps total container capital unchanged.
    const int capitalBeforeRetrieve=settlementContainerCapitalAtPosition(
        world,resident,home);
    CivilizationUtilityDecision retrieve;
    retrieve.intent=CivilizationIntent::Retrieve;
    retrieve.storage=world.storageSites.front().id;
    retrieve.item=ItemKind::RawMaterial;
    retrieve.material=MaterialKind::Water;
    retrieve.quantity=1;
    const CivilizationExecutionResult retrieved=
        executeCivilizationDecisionAtPosition(
            world,resident,retrieve,reserveStorage.pos);
    assert(retrieved.executed && retrieved.success);
    assert(settlementContainerCapitalAtPosition(
        world,resident,home)==capitalBeforeRetrieve);
    assert(simpleContainerLogisticsStockPressure(
        world,resident,home)==0.0);

    world.storageSites.clear();

    // Shared filled Water in a nearby real storage also solves the immediate
    // transport problem without forcing every resident to reinvent a vessel.
    resident.civilization.inventory.remove(
        ItemKind::SimpleContainer,
        MaterialKind::Unknown,
        1,
        true);
    StorageSite storage;
    storage.id=501;
    storage.pos={2,0};
    storage.inventory.add({
        ItemKind::SimpleContainer,
        MaterialKind::Clay,
        1,
        0.5,
        1.0});
    storage.inventory.add({
        ItemKind::RawMaterial,
        MaterialKind::Water,
        1,
        0.5,
        1.0});
    world.storageSites.push_back(storage);
    assert(waterTransportInnovationPressure(
        world,resident,home)==0.0);

    std::cout<<"water transport innovation pressure PASS\n";
    return 0;
}
