#include <iostream>

#include "lifelens/SettlementKnowledge.h"

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
    return c;
}

static void master(
    Character& resident,
    TechniqueId technique,
    int uses)
{
    resident.civilization.knowledge.learn(
        technique,KnowledgeLevel::Mastered,0.99);
    for(int i=0;i<uses;++i){
        resident.civilization.knowledge.recordSuccessfulUse(
            technique);
    }
}

int main()
{
    World world(606401);
    world.characters.clear();
    world.facilities.clear();
    world.storageSites.clear();

    Character farmA=resident(1,"FarmA");
    Character farmB=resident(2,"FarmB");
    Character metalA=resident(3,"MetalA");
    Character metalB=resident(4,"MetalB");

    master(farmA,TechniqueId::Cultivation,12);
    master(farmB,TechniqueId::Cultivation,8);
    master(metalA,TechniqueId::BronzeAlloying,12);
    master(metalB,TechniqueId::BronzeAlloying,8);

    world.characters={farmA,farmB,metalA,metalB};

    const GridPos farmCenter{0,0};
    const GridPos metalCenter{
        SettlementServiceRadiusGrid*4,
        0
    };

    StorageSite farmStorage;
    farmStorage.id=1;
    farmStorage.pos=farmCenter;
    world.storageSites.push_back(farmStorage);

    StorageSite metalStorage;
    metalStorage.id=2;
    metalStorage.pos=metalCenter;
    world.storageSites.push_back(metalStorage);

    SettlementPopulation population{
        {1,farmCenter},
        {2,{1,0}},
        {3,metalCenter},
        {4,{metalCenter.x+1,metalCenter.y}}
    };

    const SettlementNetworkObservation network=
        observeSettlementNetwork(world,&population);
    CHECK(network.settlementCount==2);
    CHECK(network.residentAssignedCount==4);

    const SettlementKnowledgeNetworkObservation knowledgeNetwork=
        observeSettlementKnowledgeNetwork(
            world,population);
    CHECK(knowledgeNetwork.settlementCount==2);
    CHECK(knowledgeNetwork.inhabitedSettlementCount==2);
    CHECK(knowledgeNetwork.specializedSettlementCount==2);
    CHECK(knowledgeNetwork.divergentTechnologyCount>=2);

    SettlementClusterId farmSettlement=0;
    SettlementClusterId metalSettlement=0;
    for(const SettlementKnowledgeProfile& settlement:
        knowledgeNetwork.settlements){
        CHECK(settlement.residentCount==2);
        CHECK(settlement.specialized);
        if(settlement.dominantTechnology==TechnologyId::Cultivation){
            farmSettlement=settlement.settlementId;
            CHECK(settlement.dominantStrength01>0.90);
            CHECK(settlement.specialization01>0.70);
        }
        if(settlement.dominantTechnology==TechnologyId::BronzeAlloying){
            metalSettlement=settlement.settlementId;
            CHECK(settlement.dominantStrength01>0.90);
            CHECK(settlement.specialization01>0.70);
        }
    }
    CHECK(farmSettlement!=0);
    CHECK(metalSettlement!=0);
    CHECK(farmSettlement!=metalSettlement);

    SocialKnowledgeBook social;

    const SettlementTeachingConnection local=
        observeSettlementTeachingConnection(
            network,population,social,1,2);
    CHECK(local.allowed);
    CHECK(local.sameSettlement);
    CHECK(local.teacherSettlement==farmSettlement);
    CHECK(local.learnerSettlement==farmSettlement);
    CHECK(settlementTeachingConnectionBonus(local)>0.0);

    const SettlementTeachingConnection remoteBlocked=
        observeSettlementTeachingConnection(
            network,population,social,1,3);
    CHECK(!remoteBlocked.allowed);
    CHECK(!remoteBlocked.sameSettlement);
    CHECK(remoteBlocked.teacherSettlement==farmSettlement);
    CHECK(remoteBlocked.learnerSettlement==metalSettlement);

    SocialFact partnership;
    partnership.id=9001;
    partnership.subject=1;
    partnership.proposition=
        societyTradePartnershipProposition(1,3);
    partnership.where="exchange-network";
    partnership.eventMinute=100;
    partnership.supports=true;
    partnership.importance=0.70;
    partnership.confidence=0.97;
    CHECK(social.registerFact(partnership));

    const SettlementTeachingConnection remoteLinked=
        observeSettlementTeachingConnection(
            network,population,social,1,3);
    CHECK(remoteLinked.allowed);
    CHECK(remoteLinked.tradePartnership);
    CHECK(!remoteLinked.sameSettlement);
    CHECK(settlementTeachingConnectionBonus(remoteLinked)>0.0);

    CHECK(settlementTechniqueTeachingBoost(
        knowledgeNetwork,
        farmSettlement,
        TechniqueId::Cultivation)>0.0);
    CHECK(settlementTechniqueTeachingBoost(
        knowledgeNetwork,
        farmSettlement,
        TechniqueId::BronzeAlloying)==0.0);
    CHECK(settlementTechniqueTeachingBoost(
        knowledgeNetwork,
        metalSettlement,
        TechniqueId::BronzeAlloying)>0.0);

    // Before infrastructure exists, nearby founding-camp residents can still
    // teach each other. Regionalization must not deadlock the first camp.
    SettlementNetworkObservation noSettlements;
    SettlementPopulation campPopulation{
        {10,{5,5}},
        {11,{6,5}}
    };
    const SettlementTeachingConnection camp=
        observeSettlementTeachingConnection(
            noSettlements,campPopulation,social,10,11);
    CHECK(camp.allowed);
    CHECK(camp.localProximity);
    CHECK(camp.teacherSettlement==0);
    CHECK(camp.learnerSettlement==0);

    std::cout
        << "C6-E regional knowledge specialization and teaching locality passed\n";
    return 0;
}
