#include <cassert>
#include <iostream>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/SocietyEconomy.h"

using namespace lifelens;

static Character makeResident(CharacterId id)
{
    Character c;
    c.id=id;
    c.name="Resident"+std::to_string(id);
    c.alive=true;
    c.civilization.character=id;
    c.civilization.gatheringSkill=0.10;
    c.civilization.craftingSkill=0.10;
    c.civilization.learningSkill=0.10;
    c.personality.curiosity=0.10;
    c.personality.patience=0.10;
    c.personality.adaptability=0.10;
    c.personality.conscientiousness=0.10;
    c.personality.empathy=0.10;
    c.personality.sociability=0.10;
    c.personality.agreeableness=0.10;
    c.personality.orderliness=0.10;
    return c;
}

int main()
{
    Character farmer=makeResident(1);
    farmer.personality.conscientiousness=0.95;
    farmer.personality.patience=0.95;
    farmer.civilization.knowledge.learn(
        TechniqueId::Cultivation,KnowledgeLevel::Mastered,0.98);
    for(int i=0;i<8;++i){
        farmer.civilization.knowledge.recordSuccessfulUse(
            TechniqueId::Cultivation);
    }
    const ResidentSocietyStatus farmerStatus=
        observeResidentSocietyStatus(farmer);
    assert(farmerStatus.role==SocietyRole::Farmer);
    assert(farmerStatus.roleStrength01>=0.50);

    World demandWorld(7001);
    demandWorld.characters.clear();
    demandWorld.storageSites.clear();
    demandWorld.facilities.clear();
    for(CharacterId id=10;id<14;++id){
        demandWorld.characters.push_back(makeResident(id));
    }
    const SocietyDemandSignal hungryFood=
        observeSocietyMaterialDemand(
            demandWorld,MaterialKind::PlantFood);
    assert(hungryFood.desiredUnits==24);
    assert(hungryFood.availableUnits==0);
    assert(hungryFood.demand01>0.99);

    demandWorld.characters[0].civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::PlantFood,24,0.8,1.0});
    const SocietyDemandSignal stockedFood=
        observeSocietyMaterialDemand(
            demandWorld,MaterialKind::PlantFood);
    assert(stockedFood.deficitUnits==0);
    assert(stockedFood.demand01==0.0);

    Character crafter=makeResident(20);
    crafter.civilization.craftingSkill=1.0;
    crafter.personality.conscientiousness=1.0;
    crafter.personality.patience=0.9;
    crafter.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::PlantFood,6,0.8,1.0});

    Character hungry=makeResident(21);
    hungry.needs.hunger=0.95;
    hungry.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Wood,6,0.6,1.0});

    RelationshipBook relationships;
    relationships.getOrCreate(crafter.id,hungry.id).trust=0.75;
    relationships.getOrCreate(hungry.id,crafter.id).trust=0.75;

    const SocietyExchangePlan plan=
        bestMutualExchangePlan(crafter,hungry,relationships);
    assert(plan.valid());
    assert(plan.firstGives==MaterialKind::PlantFood);
    assert(plan.secondGives==MaterialKind::Wood);

    const int crafterFoodBefore=crafter.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood);
    const int crafterWoodBefore=crafter.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood);
    assert(executeMutualExchange(crafter,hungry,plan));
    assert(crafter.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)
        ==crafterFoodBefore-1);
    assert(crafter.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood)
        ==crafterWoodBefore+1);

    SocialKnowledgeBook history;
    assert(registerSocietyExchangeFact(
        history,crafter,hungry,plan,600,7001)!=nullptr);
    assert(societyExchangeFactCount(history)==1);
    assert(history.receipts().size()==2);

    SocietyExchangePlan secondPlan=plan;
    assert(registerSocietyExchangeFact(
        history,crafter,hungry,secondPlan,660,7001)!=nullptr);
    assert(societyExchangeFactCount(history)==2);

    World societyWorld(7002);
    societyWorld.characters.clear();
    societyWorld.storageSites.clear();
    societyWorld.facilities.clear();
    societyWorld.characters.push_back(crafter);
    societyWorld.characters.push_back(hungry);
    StorageSite storage;
    storage.id=1;
    storage.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::PlantFood,5,0.8,1.0});
    societyWorld.storageSites.push_back(storage);

    const SocietyWorldObservation observation=
        buildSocietyWorldObservation(societyWorld,history);
    assert(observation.exchangeFactCount==2);
    bool exchangeNetwork=false;
    bool storageCommons=false;
    for(const auto& institution:observation.institutions){
        if(institution.kind==SocietyInstitutionKind::ExchangeNetwork){
            exchangeNetwork=institution.active;
        }
        if(institution.kind==SocietyInstitutionKind::StorageCommons){
            storageCommons=institution.active;
        }
    }
    assert(exchangeNetwork);
    assert(storageCommons);

    Character forager=makeResident(30);
    forager.civilization.gatheringSkill=1.0;
    forager.personality.curiosity=0.9;
    forager.personality.patience=0.9;
    CivilizationUtilityDecision gather;
    gather.intent=CivilizationIntent::Gather;
    gather.material=MaterialKind::Wood;
    gather.utility=0.40;
    const CivilizationUtilityDecision boosted=
        applySocietyRoleAndDemandUtility(
            demandWorld,forager,gather);
    assert(boosted.utility>gather.utility);

    std::cout<<"society economy foundation passed\n";
    return 0;
}
