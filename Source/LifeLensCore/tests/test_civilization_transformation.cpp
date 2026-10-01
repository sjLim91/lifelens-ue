#include <iostream>
#include <string>

#include "lifelens/CivilizationObserverReadModel.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeResident(CharacterId id)
{
    Character c;
    c.id=id;
    c.name="Resident"+std::to_string(id);
    c.alive=true;
    c.civilization.character=id;
    return c;
}

static ConstructedFacility operationalFacility(
    FacilityId id,
    FacilityKind kind)
{
    ConstructedFacility facility;
    facility.id=id;
    facility.kind=kind;
    facility.state=FacilityState::Operational;
    facility.active=true;
    facility.durability=1.0;
    return facility;
}

static const CivilizationTechnologyPopulationStatus* technologyStatus(
    const CivilizationWorldObservation& observation,
    TechnologyId technology)
{
    return findTechnologyPopulationStatus(
        observation.technologyPopulation,technology);
}

static const CivilizationTransformationStatus* transformationStatus(
    const CivilizationWorldObservation& observation,
    CivilizationTransformationId transformation)
{
    for(const auto& status:observation.transformations){
        if(status.transformation==transformation) return &status;
    }
    return nullptr;
}

int main()
{
    World world(313131);
    world.characters.clear();
    world.facilities.clear();
    world.storageSites.clear();
    world.resourceNodes.clear();

    world.characters.push_back(makeResident(1));
    world.characters.push_back(makeResident(2));
    world.characters.push_back(makeResident(3));
    world.characters.push_back(makeResident(4));

    for(int index=0;index<2;++index){
        Character& c=world.characters[index];
        c.civilization.knowledge.learn(
            TechniqueId::SharpFlake,KnowledgeLevel::Reproducible,0.95);
        c.civilization.knowledge.recordSuccessfulUse(TechniqueId::SharpFlake);
        c.civilization.knowledge.recordSuccessfulUse(TechniqueId::SharpFlake);
        c.civilization.inventory.add(
            {ItemKind::SharpFlake,MaterialKind::Flint,1,0.8,1.0});
    }

    Character& chipper=world.characters[0];
    chipper.civilization.knowledge.learn(
        TechniqueId::ChippedStoneTool,KnowledgeLevel::Reproducible,0.9);
    chipper.civilization.knowledge.recordSuccessfulUse(
        TechniqueId::ChippedStoneTool);
    chipper.civilization.knowledge.recordSuccessfulUse(
        TechniqueId::ChippedStoneTool);
    chipper.civilization.inventory.add(
        {ItemKind::StoneCuttingTool,MaterialKind::Flint,1,0.8,1.0});

    Character& metallurgist=world.characters[1];
    metallurgist.civilization.knowledge.learn(
        TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible,0.9);

    Character& formerSmith=world.characters[2];
    formerSmith.civilization.knowledge.learn(
        TechniqueId::BronzeAxe,KnowledgeLevel::Practiced,0.95);

    Character& observer=world.characters[3];
    observer.civilization.knowledge.learn(
        TechniqueId::BronzePick,KnowledgeLevel::Observed,0.6);

    chipper.civilization.knowledge.learn(
        TechniqueId::PrimitiveStorage,KnowledgeLevel::Reproducible,0.9);
    ConstructedFacility storageFacility=
        operationalFacility(100,FacilityKind::PrimitiveStorage);
    storageFacility.linkedStorage=500;
    world.facilities.push_back(storageFacility);

    StorageSite storage;
    storage.id=500;
    storage.pos={0,0};
    storage.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Wood,32,0.5,1.0});
    storage.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::CopperMetal,12,0.7,1.0});
    storage.inventory.add(
        {ItemKind::BronzePick,MaterialKind::Bronze,1,0.8,1.0});
    world.storageSites.push_back(storage);

    ConstructedFacility plot=
        operationalFacility(101,FacilityKind::CultivatedPlot);
    plot.cropPlanted=true;
    plot.cropHarvestUnits=4;
    world.facilities.push_back(plot);

    world.facilities.push_back(
        operationalFacility(102,FacilityKind::Furnace));

    SocialKnowledgeBook socialKnowledge;
    Character formerTinDiscoverer=makeResident(99);
    formerTinDiscoverer.alive=false;
    formerTinDiscoverer.civilization.knowledge.learn(
        TechniqueId::TinSmelting,KnowledgeLevel::Mastered,0.99);
    CHECK(registerTechniqueOrigin(
        socialKnowledge,formerTinDiscoverer,TechniqueId::TinSmelting,10,
        CivilizationEventType::Discovered,world.seed)!=nullptr);

    const CivilizationWorldObservation observation=
        buildCivilizationWorldObservation(world,socialKnowledge);

    CHECK(observation.technologyPopulation.size()==TechnologyRegistry.size());

    const auto* common=technologyStatus(
        observation,TechnologyId::SharpFlake);
    CHECK(common!=nullptr);
    CHECK(common->state==TechnologyPopulationState::Common);
    CHECK(common->livingKnowerCount==2);
    CHECK(common->adoptedResidentCount==2);
    CHECK(common->operationalResidentCount==2);
    CHECK(common->diffusion01>=0.49 && common->diffusion01<=0.51);

    const auto* diffusing=technologyStatus(
        observation,TechnologyId::ChippedStoneTool);
    CHECK(diffusing!=nullptr);
    CHECK(diffusing->state==TechnologyPopulationState::Diffusing);

    const auto* reproducible=technologyStatus(
        observation,TechnologyId::CopperSmelting);
    CHECK(reproducible!=nullptr);
    CHECK(reproducible->state==TechnologyPopulationState::Reproducible);

    const auto* declining=technologyStatus(
        observation,TechnologyId::BronzeAxe);
    CHECK(declining!=nullptr);
    CHECK(declining->state==TechnologyPopulationState::Declining);

    const auto* observed=technologyStatus(
        observation,TechnologyId::BronzePick);
    CHECK(observed!=nullptr);
    CHECK(observed->state==TechnologyPopulationState::Observed);

    const auto* lost=technologyStatus(
        observation,TechnologyId::TinSmelting);
    CHECK(lost!=nullptr);
    CHECK(lost->state==TechnologyPopulationState::Lost);
    CHECK(lost->historicallyKnown);
    CHECK(lost->historicalFactCount==1);
    CHECK(lost->firstEvidenceMinute==10);
    CHECK(lost->latestEvidenceMinute==10);

    const auto* storageTechnology=technologyStatus(
        observation,TechnologyId::PrimitiveStorage);
    CHECK(storageTechnology!=nullptr);
    CHECK(storageTechnology->state==TechnologyPopulationState::Operational);

    CHECK(observation.commonTechnologyCount>=1);
    CHECK(observation.decliningTechnologyCount>=1);
    CHECK(observation.lostTechnologyCount>=1);

    const auto* buffering=transformationStatus(
        observation,CivilizationTransformationId::ResourceBuffering);
    CHECK(buffering!=nullptr && buffering->active);
    CHECK(buffering->evidenceCount>=44);
    CHECK(buffering->magnitude01>0.99);

    const auto* food=transformationStatus(
        observation,CivilizationTransformationId::ManagedFoodProduction);
    CHECK(food!=nullptr && food->active);
    CHECK(food->evidenceCount>=5);
    CHECK(food->magnitude01>0.0);

    const auto* metallurgy=transformationStatus(
        observation,CivilizationTransformationId::MetallurgicalProduction);
    CHECK(metallurgy!=nullptr && metallurgy->active);
    CHECK(metallurgy->evidenceCount>=12);
    CHECK(metallurgy->magnitude01>0.0);

    const auto* tooling=transformationStatus(
        observation,CivilizationTransformationId::AdvancedTooling);
    CHECK(tooling!=nullptr && tooling->active);
    CHECK(tooling->evidenceCount==1);
    CHECK(tooling->magnitude01>0.0);

    const auto* diffusion=transformationStatus(
        observation,CivilizationTransformationId::KnowledgeDiffusion);
    CHECK(diffusion!=nullptr && diffusion->active);
    CHECK(diffusion->evidenceCount>=1);
    CHECK(diffusion->magnitude01>0.0);

    CHECK(observation.activeTransformationCount==5);

    world.storageSites.clear();
    for(auto& facility:world.facilities){
        if(facility.kind==FacilityKind::PrimitiveStorage
           || facility.kind==FacilityKind::Furnace
           || facility.kind==FacilityKind::CultivatedPlot){
            facility.state=FacilityState::Ruined;
            facility.active=false;
        }
    }
    const CivilizationWorldObservation regressed=
        buildCivilizationWorldObservation(world,socialKnowledge);
    const auto* regressedBuffering=transformationStatus(
        regressed,CivilizationTransformationId::ResourceBuffering);
    CHECK(regressedBuffering!=nullptr && !regressedBuffering->active);
    const auto* regressedFood=transformationStatus(
        regressed,CivilizationTransformationId::ManagedFoodProduction);
    CHECK(regressedFood!=nullptr && !regressedFood->active);
    const auto* regressedMetallurgy=transformationStatus(
        regressed,CivilizationTransformationId::MetallurgicalProduction);
    CHECK(regressedMetallurgy!=nullptr && !regressedMetallurgy->active);
    CHECK(world.characters[1].civilization.knowledge.knowsAtLeast(
        TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible));

    std::cout << "civilization technology lifecycle/transformation passed\n";
    return 0;
}
