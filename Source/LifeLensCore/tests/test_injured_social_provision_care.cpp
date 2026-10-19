#include <cassert>
#include <string>

#include "lifelens/ResidentInjuryProvisionCare.h"
#include "lifelens/SocialUtility.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

static Character resident(CharacterId id)
{
    Character c;
    c.id=id;
    c.alive=true;
    c.civilization.character=id;
    c.lifeStage=LifeStage::Adult;
    c.needs={0.12,0.10,0.10,0.10,0.10};
    return c;
}

static void verifyActualGiftMemory(const Character& who,bool giver,
                                   const std::string& material)
{
    bool found=false;
    for(const auto& m:who.memory.entries){
        if(m.tags.size()!=3
           ||m.tags[0]!="injury_care_provision"
           ||m.tags[1]!=material) continue;
        found=true;
        assert(m.tags[2]==(giver?"given":"received"));
        assert(m.source==MemorySource::DirectWitness);
        assert(m.witnessed &&m.sourceCharacter==who.id);
    }
    assert(found);
}

int main()
{
    Character helper=resident(11),injured=resident(12);
    injured.health.injurySeverity=0.62;
    injured.needs.hunger=0.85;
    helper.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,3,0.77,0.96});
    const double needsBefore=injured.needs.hunger;
    const double injuryBefore=injured.health.injurySeverity;
    const int total=helper.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)
      +injured.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood);

    const auto kind=provideInjuryRecoveryProvisions(helper,injured,480);
    assert(kind==InjuryProvisionGift::PlantFood);
    assert(helper.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==2);
    assert(injured.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==1);
    assert(total==helper.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)
      +injured.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood));
    assert(injured.civilization.inventory.stacks().front().quality==0.77);
    assert(injured.needs.hunger==needsBefore);  // no free nutrition
    assert(injured.health.injurySeverity==injuryBefore); // no instant healing
    verifyActualGiftMemory(helper,true,"PlantFood");
    verifyActualGiftMemory(injured,false,"PlantFood");
    assert(provideInjuryRecoveryProvisions(helper,injured,481)
        ==InjuryProvisionGift::None); // recipient already holds food

    Character waterHelper=resident(31),thirsty=resident(32);
    thirsty.health.injurySeverity=0.54;
    thirsty.needs.hunger=0.20;
    thirsty.needs.thirst=0.91;
    waterHelper.civilization.inventory.add({
        ItemKind::SimpleContainer,MaterialKind::Unknown,2,0.7,0.9});
    waterHelper.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Water,2,0.5,1.0});
    assert(portableWaterCount(waterHelper.civilization.inventory)==2);
    assert(provideInjuryRecoveryProvisions(waterHelper,thirsty,482)
        ==InjuryProvisionGift::PortableWater);
    assert(portableWaterCount(waterHelper.civilization.inventory)==1);
    assert(portableWaterCount(thirsty.civilization.inventory)==1);
    assert(thirsty.needs.thirst==0.91);
    assert(waterHelper.civilization.inventory.count(
        ItemKind::SimpleContainer,MaterialKind::Unknown,true)==1);
    verifyActualGiftMemory(thirsty,false,"PortableWater");

    // Same input world state produces exactly the same physical transfer
    // and personal witness evidence; no stochastic or paid-model outcome.
    Character again=resident(31),other=resident(32);
    other.health=resident(32).health;
    other.health.injurySeverity=0.54;
    other.needs.thirst=0.91;
    other.needs.hunger=0.20;
    again.civilization.inventory.add({
        ItemKind::SimpleContainer,MaterialKind::Unknown,2,0.7,0.9});
    again.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Water,2,0.5,1.0});
    assert(provideInjuryRecoveryProvisions(again,other,482)
        ==InjuryProvisionGift::PortableWater);
    assert(other.civilization.inventory.stacks().size()
        ==thirsty.civilization.inventory.stacks().size());
    assert(other.memory.entries.size()==thirsty.memory.entries.size());

    // Injured people without acute need, healthy people, dependent babies,
    // and hungry/goods-poor helpers cannot create free gifts.
    Character healthy=resident(40),poor=resident(41);
    healthy.needs.hunger=0.95;
    healthy.health.injurySeverity=0.0;
    poor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,1,0.5,1.0});
    assert(provideInjuryRecoveryProvisions(poor,healthy,483)
        ==InjuryProvisionGift::None);
    healthy.health.injurySeverity=0.65;
    assert(provideInjuryRecoveryProvisions(poor,healthy,483)
        ==InjuryProvisionGift::None);  // reserve unavailable
    poor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,1,0.5,1.0});
    poor.needs.hunger=0.99;
    assert(provideInjuryRecoveryProvisions(poor,healthy,483)
        ==InjuryProvisionGift::None);
    poor.needs.hunger=0.10;
    healthy.lifeStage=LifeStage::Baby;
    assert(provideInjuryRecoveryProvisions(poor,healthy,483)
        ==InjuryProvisionGift::None); // babies use dependent-care system

    // Visible injury can motivate *choice* of social comfort even if
    // the patient is not crying; generosity is still personality dependent.
    Character compassionate=resident(90),quietPatient=resident(91);
    compassionate.personality.empathy=0.96;
    compassionate.personality.agreeableness=0.96;
    compassionate.personality.sociability=0.70;
    quietPatient.health.injurySeverity=0.82;
    quietPatient.emotion.fear=0.0;
    quietPatient.emotion.anxiety=0.0;
    quietPatient.emotion.sadness=0.0;
    quietPatient.emotion.grief=0.0;
    RelationshipBook relations;
    auto& bond=relations.getOrCreate(90,91);
    bond.affection=0.80;
    bond.trust=0.8;
    bond.comfort=0.8;
    bond.familiarity=0.8;
    assert(scoreComfortIntent(compassionate,quietPatient,relations)>0.18);
    const double withInjury=scoreComfortIntent(
        compassionate,quietPatient,relations);
    quietPatient.health.injurySeverity=0.0;
    assert(scoreComfortIntent(compassionate,quietPatient,relations)==0.0);
    assert(withInjury>0.0);

    // Integration: utility chooses Comfort, Core brings the helper into
    // physical range and COMPLETES it before any real supply is transferred.
    // Two identical seeds must have the same transactions and logs.
    auto setup=[&](Simulation& sim){
        sim.setupSocialDemo();
        auto& provider=sim.world().characters[0];
        auto& patient=sim.world().characters[1];
        provider.civilization.inventory.add({
            ItemKind::RawMaterial,MaterialKind::PlantFood,3,0.7,0.95});
        patient.health.injurySeverity=0.75;
        patient.needs.hunger=0.56;
        patient.emotion.sadness=0.98;
        patient.emotion.refreshSummary();
    };
    Simulation live(777),againLive(777);
    setup(live);
    setup(againLive);
    live.runMinutes(40);
    againLive.runMinutes(40);
    assert(live.logs()==againLive.logs());
    bool witnessedCare=false;
    for(const auto& m:live.world().characters[1].memory.entries){
        if(m.tags.size()==3
           &&m.tags[0]=="injury_care_provision"
           &&m.tags[1]=="PlantFood"
           &&m.tags[2]=="received") witnessedCare=true;
    }
    assert(witnessedCare && "live Social Comfort must physically transfer food");
    assert(live.world().characters[0].civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)<=2);
    return 0;
}
