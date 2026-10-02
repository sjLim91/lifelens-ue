#include <iostream>

#include "lifelens/SettlementRelations.h"

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

static void addRaw(
    Inventory& inventory,
    MaterialKind material,
    int quantity)
{
    inventory.add(
        {ItemKind::RawMaterial,material,quantity,0.9,1.0});
}

static void makePositive(
    RelationshipBook& relationships,
    CharacterId from,
    CharacterId to)
{
    Relationship& r=
        relationships.getOrCreate(from,to);
    r.affection=0.78;
    r.trust=0.86;
    r.respect=0.74;
    r.comfort=0.76;
    r.familiarity=0.82;
    r.conflict=0.02;
    r.fear=0.01;
    r.grudge=0.01;
}

static void makeNegative(
    RelationshipBook& relationships,
    CharacterId from,
    CharacterId to)
{
    Relationship& r=
        relationships.getOrCreate(from,to);
    r.affection=0.02;
    r.trust=0.03;
    r.respect=0.05;
    r.comfort=0.02;
    r.familiarity=0.55;
    r.conflict=0.92;
    r.fear=0.72;
    r.grudge=0.84;
}

int main()
{
    World world(606601);
    world.characters.clear();
    world.storageSites.clear();
    world.facilities.clear();
    world.resourceNodes.clear();
    world.characters={
        resident(1,"A1"),
        resident(2,"A2"),
        resident(3,"B1"),
        resident(4,"B2")
    };

    const GridPos firstCenter{0,0};
    const GridPos secondCenter{
        SettlementServiceRadiusGrid*4,
        0
    };

    StorageSite firstStorage;
    firstStorage.id=1;
    firstStorage.pos=firstCenter;
    addRaw(
        firstStorage.inventory,
        MaterialKind::Wood,10);
    world.storageSites.push_back(firstStorage);

    StorageSite secondStorage;
    secondStorage.id=2;
    secondStorage.pos=secondCenter;
    addRaw(
        secondStorage.inventory,
        MaterialKind::Bronze,8);
    world.storageSites.push_back(secondStorage);

    SettlementPopulation population{
        {1,firstCenter},
        {2,{firstCenter.x+1,firstCenter.y}},
        {3,secondCenter},
        {4,{secondCenter.x+1,secondCenter.y}}
    };
    const SettlementNetworkObservation network=
        observeSettlementNetwork(
            world,&population);
    CHECK(network.settlementCount==2);
    CHECK(network.residentAssignedCount==4);

    const SettlementClusterId firstSettlement=
        network.settlements[0].id;
    const SettlementClusterId secondSettlement=
        network.settlements[1].id;
    CHECK(firstSettlement!=secondSettlement);

    // Shared scarcity and complementary production create pressure/opportunity,
    // but no social evidence means the settlements remain neutral.
    RelationshipBook noRelationships;
    SocialKnowledgeBook noKnowledge;
    const SettlementGroupRelationsObservation neutralNetwork=
        observeSettlementGroupRelations(
            world,noRelationships,noKnowledge,
            network,population);
    CHECK(neutralNetwork.relationCount==1);
    const SettlementGroupRelationObservation* neutral=
        settlementGroupRelationBetween(
            neutralNetwork,
            firstSettlement,
            secondSettlement);
    CHECK(neutral!=nullptr);
    CHECK(neutral->state==SettlementGroupRelationState::Neutral);
    CHECK(neutral->sharedScarcityPressure01>0.90);
    CHECK(neutral->productionComplementarity01>0.20);
    CHECK(neutral->relationshipEvidenceCount==0);
    CHECK(neutralNetwork.hostileCount==0);

    // Real trust plus actual inter-settlement exchange becomes cooperation.
    RelationshipBook positiveRelationships;
    makePositive(positiveRelationships,1,3);
    makePositive(positiveRelationships,3,1);

    SocialKnowledgeBook cooperativeKnowledge;
    SocietyExchangePlan exchange;
    exchange.first=1;
    exchange.second=3;
    exchange.firstGives=MaterialKind::Wood;
    exchange.secondGives=MaterialKind::Bronze;
    exchange.quantityEach=1;
    exchange.score=0.82;
    CHECK(registerInterSettlementTradeFact(
        cooperativeKnowledge,
        world.characters[0],
        world.characters[2],
        firstSettlement,
        secondSettlement,
        exchange,
        120,
        world.seed)!=nullptr);

    const SettlementGroupRelationsObservation cooperativeNetwork=
        observeSettlementGroupRelations(
            world,positiveRelationships,cooperativeKnowledge,
            network,population);
    const SettlementGroupRelationObservation* cooperative=
        settlementGroupRelationBetween(
            cooperativeNetwork,
            firstSettlement,
            secondSettlement);
    CHECK(cooperative!=nullptr);
    CHECK(cooperative->exchangeEvidenceCount==1);
    CHECK(cooperative->relationshipEvidenceCount==2);
    CHECK(cooperative->cooperation01>0.42);
    CHECK(cooperative->cooperation01>cooperative->tension01+0.08);
    CHECK(cooperative->state==SettlementGroupRelationState::Cooperative);
    CHECK(cooperativeNetwork.cooperativeCount==1);
    CHECK(settlementGroupTradeUtilityAdjustment(
        cooperative)>0.0);

    // The same resource pressure cannot explain hostility on its own. Hostility
    // appears only when cross-settlement resident relationships carry strong
    // conflict, fear and grudges.
    RelationshipBook negativeRelationships;
    makeNegative(negativeRelationships,1,3);
    makeNegative(negativeRelationships,3,1);

    const SettlementGroupRelationsObservation hostileNetwork=
        observeSettlementGroupRelations(
            world,negativeRelationships,noKnowledge,
            network,population);
    const SettlementGroupRelationObservation* hostile=
        settlementGroupRelationBetween(
            hostileNetwork,
            firstSettlement,
            secondSettlement);
    CHECK(hostile!=nullptr);
    CHECK(hostile->relationshipEvidenceCount==2);
    CHECK(hostile->averageConflict01>0.90);
    CHECK(hostile->averageGrudge01>0.80);
    CHECK(hostile->tension01>0.55);
    CHECK(hostile->state==SettlementGroupRelationState::Hostile);
    CHECK(hostileNetwork.hostileCount==1);
    CHECK(settlementGroupTradeUtilityAdjustment(
        hostile)<0.0);

    // Mixed but not extreme negative evidence forms a strained relation rather
    // than jumping directly from neutral to hostile.
    RelationshipBook strainedRelationships;
    Relationship& aToB=
        strainedRelationships.getOrCreate(1,3);
    aToB.trust=0.18;
    aToB.familiarity=0.50;
    aToB.conflict=0.52;
    aToB.fear=0.28;
    aToB.grudge=0.40;
    Relationship& bToA=
        strainedRelationships.getOrCreate(3,1);
    bToA=aToB;
    bToA.from=3;
    bToA.to=1;

    const SettlementGroupRelationsObservation strainedNetwork=
        observeSettlementGroupRelations(
            world,strainedRelationships,noKnowledge,
            network,population);
    const SettlementGroupRelationObservation* strained=
        settlementGroupRelationBetween(
            strainedNetwork,
            firstSettlement,
            secondSettlement);
    CHECK(strained!=nullptr);
    CHECK(strained->state==SettlementGroupRelationState::Strained);
    CHECK(strainedNetwork.strainedCount==1);

    std::cout
        << "C6-G settlement cooperation and conflict foundations passed\n";
    return 0;
}
