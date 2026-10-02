#include <iostream>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/SettlementProduction.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character resident(CharacterId id,const char* name)
{
    Character c;
    c.id=id;
    c.name=name;
    c.alive=true;
    c.lifeStage=LifeStage::Adult;
    c.civilization.character=id;
    c.personality.conscientiousness=0.92;
    c.personality.patience=0.90;
    return c;
}

static void master(
    Character& c,
    TechniqueId technique,
    int uses)
{
    c.civilization.knowledge.learn(
        technique,KnowledgeLevel::Mastered,0.99);
    for(int i=0;i<uses;++i){
        c.civilization.knowledge.recordSuccessfulUse(
            technique);
    }
}

static ConstructedFacility operational(
    FacilityId id,
    FacilityKind kind,
    GridPos pos)
{
    ConstructedFacility facility;
    facility.id=id;
    facility.kind=kind;
    facility.pos=pos;
    facility.state=FacilityState::Operational;
    facility.active=true;
    facility.durability=1.0;
    return facility;
}

static void resource(
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
    CHECK(settlementProductionDecisionWindow(0));
    CHECK(settlementProductionDecisionWindow(
        SettlementProductionDecisionIntervalMinutes));
    CHECK(!settlementProductionDecisionWindow(1));
    CHECK(!settlementProductionDecisionWindow(
        SettlementProductionDecisionIntervalMinutes-1));

    World world(606501);
    world.characters.clear();
    world.facilities.clear();
    world.storageSites.clear();
    world.resourceNodes.clear();

    Character farmA=resident(1,"FarmA");
    Character farmB=resident(2,"FarmB");
    Character metalA=resident(3,"MetalA");
    Character metalB=resident(4,"MetalB");

    master(farmA,TechniqueId::Cultivation,12);
    master(farmB,TechniqueId::Cultivation,10);

    metalA.civilization.craftingSkill=0.94;
    metalB.civilization.craftingSkill=0.90;
    master(metalA,TechniqueId::CopperSmelting,12);
    master(metalA,TechniqueId::TinSmelting,12);
    master(metalA,TechniqueId::BronzeAlloying,12);
    master(metalB,TechniqueId::CopperSmelting,10);
    master(metalB,TechniqueId::TinSmelting,10);
    master(metalB,TechniqueId::BronzeAlloying,10);

    world.characters={farmA,farmB,metalA,metalB};

    const GridPos farmCenter{0,0};
    const GridPos metalCenter{
        SettlementServiceRadiusGrid*4,
        0
    };

    StorageSite farmStorage;
    farmStorage.id=1;
    farmStorage.pos=farmCenter;
    farmStorage.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::PlantFood,16,0.9,1.0});
    world.storageSites.push_back(farmStorage);

    StorageSite metalStorage;
    metalStorage.id=2;
    metalStorage.pos=metalCenter;
    metalStorage.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Bronze,8,0.9,1.0});
    world.storageSites.push_back(metalStorage);

    world.facilities.push_back(
        operational(
            10,FacilityKind::CultivatedPlot,
            {farmCenter.x+2,farmCenter.y}));
    world.facilities.push_back(
        operational(
            20,FacilityKind::Furnace,
            {metalCenter.x+2,metalCenter.y}));

    resource(
        world,100,MaterialKind::PlantFood,28,
        {farmCenter.x+3,farmCenter.y});
    resource(
        world,200,MaterialKind::CopperOre,18,
        {metalCenter.x+3,metalCenter.y});
    resource(
        world,201,MaterialKind::TinOre,18,
        {metalCenter.x+4,metalCenter.y});

    SettlementPopulation population{
        {1,farmCenter},
        {2,{farmCenter.x+1,farmCenter.y}},
        {3,metalCenter},
        {4,{metalCenter.x+1,metalCenter.y}}
    };

    const SettlementProductionNetworkObservation production=
        observeSettlementProductionNetwork(
            world,population);
    CHECK(production.settlementCount==2);
    CHECK(production.inhabitedSettlementCount==2);
    CHECK(production.specializedSettlementCount==2);

    const SettlementProductionProfile* farm=nullptr;
    const SettlementProductionProfile* metal=nullptr;
    for(const SettlementProductionProfile& profile:
        production.settlements){
        CHECK(profile.residentCount==2);
        CHECK(profile.specialized);
        if(profile.dominantKind==SettlementProductionKind::Food){
            farm=&profile;
        }
        if(profile.dominantKind==SettlementProductionKind::Metallurgy){
            metal=&profile;
        }
    }

    CHECK(farm!=nullptr);
    CHECK(metal!=nullptr);
    CHECK(farm->food01>farm->metallurgy01+0.20);
    CHECK(metal->metallurgy01>metal->food01+0.20);
    CHECK(farm->dominantSurplusMaterial==MaterialKind::PlantFood);
    CHECK(metal->dominantSurplusMaterial==MaterialKind::Bronze);
    CHECK(farm->dominantSurplusUnits>0);
    CHECK(metal->dominantSurplusUnits>0);

    CHECK(settlementProductionMaterialAffinity(
        *farm,MaterialKind::PlantFood)==farm->food01);
    CHECK(settlementProductionMaterialAffinity(
        *metal,MaterialKind::Bronze)==metal->metallurgy01);
    CHECK(settlementProductionTechniqueAffinity(
        *farm,TechniqueId::Cultivation)==farm->food01);
    CHECK(settlementProductionTechniqueAffinity(
        *metal,TechniqueId::BronzeAlloying)==metal->metallurgy01);

    CivilizationUtilityDecision cultivate;
    cultivate.intent=CivilizationIntent::Craft;
    cultivate.technique=TechniqueId::Cultivation;
    cultivate.utility=0.40;
    const CivilizationUtilityDecision farmBoosted=
        applySettlementProductionSpecializationUtility(
            farm,cultivate);
    CHECK(farmBoosted.utility>cultivate.utility);

    CivilizationUtilityDecision smelt;
    smelt.intent=CivilizationIntent::Craft;
    smelt.technique=TechniqueId::BronzeAlloying;
    smelt.utility=0.40;
    const CivilizationUtilityDecision metalBoosted=
        applySettlementProductionSpecializationUtility(
            metal,smelt);
    CHECK(metalBoosted.utility>smelt.utility);

    // Specialization must not manufacture exploration/innovation preference.
    CivilizationUtilityDecision explore;
    explore.intent=CivilizationIntent::Explore;
    explore.material=MaterialKind::PlantFood;
    explore.utility=0.40;
    CHECK(applySettlementProductionSpecializationUtility(
        farm,explore).utility==explore.utility);

    CivilizationUtilityDecision experiment;
    experiment.intent=CivilizationIntent::Experiment;
    experiment.technique=TechniqueId::Cultivation;
    experiment.utility=0.40;
    CHECK(applySettlementProductionSpecializationUtility(
        farm,experiment).utility==experiment.utility);

    // Mismatched production still receives less reinforcement than the local
    // specialization, so the read model expresses a real regional difference.
    const CivilizationUtilityDecision farmSmelt=
        applySettlementProductionSpecializationUtility(
            farm,smelt);
    CHECK(farmBoosted.utility>farmSmelt.utility);

    std::cout
        << "C6-F settlement resource and production specialization passed\n";
    return 0;
}
