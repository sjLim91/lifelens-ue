#include <cassert>
#include <iostream>

#include "lifelens/CivilizationSnapshotCodec.h"
#include "lifelens/SocialUtility.h"
#include "lifelens/SocietyEconomy.h"

using namespace lifelens;

static Character makeAdult(CharacterId id)
{
    Character c;
    c.id=id;
    c.name="Resident"+std::to_string(id);
    c.alive=true;
    c.lifeStage=LifeStage::Adult;
    c.civilization.character=id;
    c.personality.curiosity=0.7;
    c.personality.openness=0.7;
    c.personality.patience=0.7;
    c.personality.conscientiousness=0.7;
    c.personality.orderliness=0.7;
    c.personality.adaptability=0.7;
    c.civilization.gatheringSkill=0.5;
    c.civilization.craftingSkill=0.5;
    c.civilization.learningSkill=0.5;
    return c;
}

int main()
{
    constexpr std::uint64_t seed=919292;

    World world(seed);
    world.characters.clear();
    world.resourceNodes.clear();
    world.storageSites.clear();
    world.facilities.clear();

    Character forager=makeAdult(1);
    forager.civilization.gatheringSkill=1.0;
    forager.personality.curiosity=1.0;
    forager.personality.patience=1.0;
    forager.personality.adaptability=1.0;

    Character storekeeper=makeAdult(2);
    storekeeper.civilization.knowledge.learn(
        TechniqueId::PrimitiveStorage,KnowledgeLevel::Mastered,0.98);
    storekeeper.personality.orderliness=1.0;
    storekeeper.personality.conscientiousness=1.0;
    storekeeper.civilization.craftingSkill=0.8;

    world.characters={forager,storekeeper};

    StorageSite storage;
    storage.id=1;
    storage.pos={0,0};
    storage.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Wood,2,0.5,1.0});
    world.storageSites.push_back(storage);

    SocialKnowledgeBook book;
    assert(registerSocietyInstitutionMembership(
        book,world.characters[0],
        SocietyInstitutionKind::ProductionNetwork,60,seed)!=nullptr);
    assert(registerSocietyInstitutionMembership(
        book,world.characters[1],
        SocietyInstitutionKind::StorageCommons,60,seed)!=nullptr);

    HouseholdBook households;
    assert(households.create(
        1,{world.characters[1].id,world.characters[0].id}));

    world.characters[1].civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::PlantFood,3,0.7,1.0});
    assert(observeSocietyResourceDisposition(
        world.characters[1],MaterialKind::PlantFood,
        book,&households)==SocietyResourceDisposition::HouseholdReserve);

    world.characters[1].civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::PlantFood,2,0.7,1.0});
    assert(observeSocietyResourceDisposition(
        world.characters[1],MaterialKind::PlantFood,
        book,&households)==SocietyResourceDisposition::SharedSurplus);

    const SocietyCoordinationDirective directive=
        observeSocietyCoordinationDirective(
            world,book,world.characters[0],&households);
    assert(directive.task==SocietyCoordinationTask::ProvisionFood);
    assert(directive.material==MaterialKind::PlantFood);
    assert(directive.institutionBacked);

    CivilizationUtilityDecision gather;
    gather.intent=CivilizationIntent::Gather;
    gather.material=MaterialKind::PlantFood;
    gather.utility=0.40;
    const CivilizationUtilityDecision coordinated=
        applySocietyCoordinationBias(
            world,world.characters[0],gather,&book,&households);
    assert(coordinated.utility>gather.utility);

    CivilizationUtilityDecision storeFood;
    storeFood.intent=CivilizationIntent::Store;
    storeFood.material=MaterialKind::PlantFood;
    storeFood.utility=0.40;
    const CivilizationUtilityDecision sharedStore=
        applySocietyCoordinationBias(
            world,world.characters[1],storeFood,&book,&households);
    assert(sharedStore.utility>storeFood.utility);

    Character protectedResident=world.characters[1];
    protectedResident.civilization.inventory=Inventory{};
    protectedResident.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::PlantFood,3,0.7,1.0});
    const CivilizationUtilityDecision protectedStore=
        applySocietyCoordinationBias(
            world,protectedResident,storeFood,&book,&households);
    assert(protectedStore.utility<storeFood.utility);

    assert(registerSocietySharedContributionFact(
        book,world.characters[1],MaterialKind::PlantFood,
        2,120,seed)!=nullptr);
    assert(societyFactCountWithPrefix(
        book,"shared-contribution:")==1);

    SocialFact target;
    target.id=777;
    target.subject=world.characters[0].id;
    target.proposition="tested-collective-knowledge";
    target.where="settlement";
    target.eventMinute=100;
    target.supports=true;
    target.importance=0.80;
    target.confidence=0.95;
    assert(book.registerFact(target));

    const SocialFact* candidate=
        bestSocietyDurableRecordCandidate(book);
    assert(candidate!=nullptr);
    const SocialFactId candidateId=candidate->id;
    assert(registerSocietyDurableRecordFact(
        book,world.characters[0],*candidate,
        MaterialKind::Clay,180,seed)!=nullptr);
    assert(societyFactAlreadyDurablyRecorded(
        book,candidateId));

    const SocialFact* candidateAfter=book.findFact(candidateId);
    assert(candidateAfter!=nullptr);
    assert(registerSocietyDurableRecordFact(
        book,world.characters[0],*candidateAfter,
        MaterialKind::Clay,200,seed)==nullptr);

    world.characters[0].civilization.inventory.add(
        {ItemKind::RecordTablet,MaterialKind::Clay,1,0.72,0.92});
    assert(validateInventoryState(
        world.characters[0].civilization.inventory));
    assert(societyRecordTabletUnits(world)==1);

    const SocietyWorldObservation observation=
        buildSocietyWorldObservation(world,book,&households);
    assert(observation.sharedContributionFactCount==1);
    assert(observation.durableRecordFactCount==1);
    assert(observation.recordMediaUnits==1);
    assert(observation.coordinatedResidentCount>=1);
    assert(!observation.coordination.empty());

    std::cout<<"society coordination + ownership + durable records passed\n";
    return 0;
}
