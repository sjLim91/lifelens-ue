#include <cstdlib>
#include <iostream>

#include "lifelens/CivilizationCapability.h"
#include "lifelens/CivilizationKnowledgeTransmission.h"
#include "lifelens/CivilizationObserverReadModel.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\\n'; \
        return 1; \
    } \
} while(false)

static ConstructedFacility operationalFacility(
    FacilityId id,
    FacilityKind kind,
    GridPos pos,
    CharacterId builder)
{
    ConstructedFacility facility=
        makeFacilityConstructionSite(id,kind,pos,builder,0);
    for(auto& requirement:facility.requirements){
        requirement.delivered=requirement.required;
    }
    facility.constructionWork=facility.requiredWork;
    const StorageId storage=facilityProvidesStorage(kind) ? id+1000 : 0;
    if(!activateConstructedFacility(facility,storage,0)){
        std::cerr << "failed to activate fixture\\n";
        std::abort();
    }
    return facility;
}

static const CivilizationCapabilityObservation* capability(
    const std::vector<CivilizationCapabilityObservation>& observations,
    CivilizationCapabilityId id)
{
    return findCivilizationCapability(observations,id);
}

static const CivilizationTechnologyObservation* technology(
    const std::vector<CivilizationTechnologyObservation>& observations,
    CivilizationTechnologyId id)
{
    for(const auto& observed:observations){
        if(observed.technology==id) return &observed;
    }
    return nullptr;
}

int main()
{
    World world(919191);
    world.characters.clear();
    world.facilities.clear();
    world.storageSites.clear();
    world.resourceNodes.clear();

    Character metallurgist;
    metallurgist.id=1;
    metallurgist.name="Metallurgist";
    metallurgist.alive=true;
    metallurgist.civilization.character=metallurgist.id;
    metallurgist.civilization.knowledge.learn(
        TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible,0.9);
    metallurgist.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::CopperOre,3,0.5,1.0});
    metallurgist.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Charcoal,3,0.5,1.0});
    world.characters.push_back(metallurgist);

    // Knowledge alone is not an operational capability.
    auto capabilities=buildCivilizationCapabilityObservations(world);
    const auto* copper=capability(
        capabilities,CivilizationCapabilityId::CopperSmelting);
    CHECK(copper!=nullptr);
    CHECK(copper->knowledgeableResidents==1);
    CHECK(!copper->operational);

    auto technologies=buildCivilizationTechnologyObservations(
        world,capabilities);
    const auto* copperTech=technology(
        technologies,CivilizationTechnologyId::CopperSmelting);
    CHECK(copperTech!=nullptr);
    CHECK(copperTech->known);
    CHECK(copperTech->reproducible);
    CHECK(!copperTech->operational);

    // Physical facilities turn the already-known technique into a real ability.
    world.facilities.push_back(
        operationalFacility(10,FacilityKind::FirePit,{0,0},1));
    world.facilities.push_back(
        operationalFacility(11,FacilityKind::Furnace,{2,0},1));
    capabilities=buildCivilizationCapabilityObservations(world);
    copper=capability(capabilities,CivilizationCapabilityId::CopperSmelting);
    CHECK(copper!=nullptr && copper->operational);
    CHECK(copper->supportingFacilityCount==1);

    // Losing the Furnace removes capability without deleting knowledge.
    for(auto& facility:world.facilities){
        if(facility.kind==FacilityKind::Furnace){
            facility.state=FacilityState::Ruined;
            facility.active=false;
        }
    }
    capabilities=buildCivilizationCapabilityObservations(world);
    copper=capability(capabilities,CivilizationCapabilityId::CopperSmelting);
    CHECK(copper!=nullptr && !copper->operational);
    CHECK(world.characters[0].civilization.knowledge.knowsAtLeast(
        TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible));

    // A physical advanced tool can exist before its production knowledge is
    // widespread. Capability and Technology adoption therefore stay distinct.
    world.characters[0].civilization.inventory.add(
        {ItemKind::BronzeAxe,MaterialKind::Bronze,1,0.8,1.0});
    capabilities=buildCivilizationCapabilityObservations(world);
    const auto* bronzeWood=capability(
        capabilities,CivilizationCapabilityId::BronzeWoodworking);
    CHECK(bronzeWood!=nullptr && bronzeWood->operational);
    technologies=buildCivilizationTechnologyObservations(world,capabilities);
    const auto* bronzeAxeTech=technology(
        technologies,CivilizationTechnologyId::BronzeAxe);
    CHECK(bronzeAxeTech!=nullptr);
    CHECK(!bronzeAxeTech->known);
    CHECK(!bronzeAxeTech->operational);

    world.characters[0].civilization.knowledge.learn(
        TechniqueId::BronzeAxe,KnowledgeLevel::Reproducible,0.9);
    technologies=buildCivilizationTechnologyObservations(world,capabilities);
    bronzeAxeTech=technology(
        technologies,CivilizationTechnologyId::BronzeAxe);
    CHECK(bronzeAxeTech!=nullptr);
    CHECK(bronzeAxeTech->known);
    CHECK(bronzeAxeTech->reproducible);
    CHECK(bronzeAxeTech->operational);

    // C1-F techniques must participate in the same social fact / witness path
    // as all earlier techniques, not stop at the old Cultivation enum boundary.
    SocialKnowledgeBook socialKnowledge;
    Character teacher=world.characters[0];
    teacher.civilization.knowledge.learn(
        TechniqueId::BronzeAxe,KnowledgeLevel::Mastered,0.98);
    const KnowledgeReceipt* origin=registerTechniqueOrigin(
        socialKnowledge,teacher,TechniqueId::BronzeAxe,100,
        CivilizationEventType::Crafted,world.seed);
    CHECK(origin!=nullptr);
    const SocialFact* fact=socialKnowledge.findFact(origin->factId);
    CHECK(fact!=nullptr);
    CHECK(techniqueFromCivilizationFact(*fact)==TechniqueId::BronzeAxe);

    Character learner;
    learner.id=2;
    learner.name="Learner";
    learner.alive=true;
    learner.civilization.character=learner.id;
    const TechniqueTransmissionOutcome witness=applyTechniqueWitness(
        socialKnowledge,*fact,teacher,learner,world.seed,101);
    CHECK(witness.receiptAccepted);
    CHECK(witness.technique==TechniqueId::BronzeAxe);
    CHECK(learner.civilization.knowledge.knowsAtLeast(
        TechniqueId::BronzeAxe,KnowledgeLevel::Observed));

    world.characters.push_back(learner);
    const CivilizationWorldObservation observed=
        buildCivilizationWorldObservation(world,socialKnowledge);
    CHECK(observed.capabilitySchemaVersion==CivilizationCapabilitySchemaVersion);
    CHECK(observed.technologySchemaVersion==CivilizationTechnologySchemaVersion);
    CHECK(observed.capabilities.size()==16);
    CHECK(observed.technologies.size()==16);
    CHECK(observed.operationalCapabilityCount>0);
    CHECK(observed.knownTechnologyCount>0);
    CHECK(observed.reproducibleTechnologyCount>0);
    CHECK(observed.recentDiscoveries.empty()); // Crafted fact is not a discovery card.
    CHECK(observed.techniqueFactCount==1);
    CHECK(observed.transmissionReceiptCount==2);

    std::cout << "civilization operational capability/technology passed\n";
    return 0;
}
