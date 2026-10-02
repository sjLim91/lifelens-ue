#include <cassert>
#include <iostream>
#include <vector>

#include "lifelens/CivilizationEra.h"

using namespace lifelens;

namespace {

ConstructedFacility operationalFacility(
    FacilityId id,
    FacilityKind kind,
    GridPos pos={})
{
    ConstructedFacility facility=
        makeFacilityConstructionSite(id,kind,pos,1,0);
    for(auto& requirement:facility.requirements){
        requirement.delivered=requirement.required;
    }
    facility.constructionWork=facility.requiredWork;
    facility.state=FacilityState::Operational;
    facility.completedMinute=1;
    facility.durability=1.0;
    facility.active=true;
    return facility;
}

CivilizationTechnologyPopulationStatus technologyStatus(
    TechnologyId id,
    TechnologyPopulationState state,
    int operational=0,
    int adopted=0)
{
    CivilizationTechnologyPopulationStatus status;
    status.technology=id;
    status.state=state;
    status.livingKnowerCount=state==TechnologyPopulationState::Unknown ? 0 : 1;
    status.reproducibleKnowerCount=operational>0 ? 1 : 0;
    status.operationalResidentCount=operational;
    status.adoptedResidentCount=adopted;
    return status;
}

CivilizationTransformationStatus transformationStatus(
    CivilizationTransformationId id,
    bool active)
{
    CivilizationTransformationStatus status;
    status.transformation=id;
    status.active=active;
    status.magnitude01=active ? 1.0 : 0.0;
    status.evidenceCount=active ? 1 : 0;
    return status;
}

void upsertTechnology(
    std::vector<CivilizationTechnologyPopulationStatus>& statuses,
    CivilizationTechnologyPopulationStatus next)
{
    for(auto& status:statuses){
        if(status.technology==next.technology){
            status=next;
            return;
        }
    }
    statuses.push_back(next);
}

void upsertTransformation(
    std::vector<CivilizationTransformationStatus>& statuses,
    CivilizationTransformationStatus next)
{
    for(auto& status:statuses){
        if(status.transformation==next.transformation){
            status=next;
            return;
        }
    }
    statuses.push_back(next);
}

}

int main()
{
    World world(901001);
    std::vector<CivilizationTechnologyPopulationStatus> technologies;
    std::vector<CivilizationTransformationStatus> transformations;

    auto era=buildCivilizationEraObservation(
        world,technologies,transformations);
    assert(era.currentEra==CivilizationEraId::NaturalSurvival);
    assert(era.ordinal==0);
    assert(era.hasNextEra);
    assert(era.nextEra==CivilizationEraId::EarlySettlement);

    upsertTechnology(
        technologies,
        technologyStatus(
            TechnologyId::SharpFlake,
            TechnologyPopulationState::Observed));
    era=buildCivilizationEraObservation(
        world,technologies,transformations);
    assert(era.currentEra==CivilizationEraId::NaturalSurvival);

    world.facilities.push_back(
        operationalFacility(
            1,FacilityKind::SleepingPlace,{0,0}));
    world.facilities.push_back(
        operationalFacility(
            2,FacilityKind::PrimitiveStorage,{8,0}));
    upsertTransformation(
        transformations,
        transformationStatus(
            CivilizationTransformationId::ResourceBuffering,true));

    era=buildCivilizationEraObservation(
        world,technologies,transformations);
    assert(era.currentEra==CivilizationEraId::EarlySettlement);
    assert(era.ordinal==1);

    upsertTechnology(
        technologies,
        technologyStatus(
            TechnologyId::Cultivation,
            TechnologyPopulationState::Observed));
    era=buildCivilizationEraObservation(
        world,technologies,transformations);
    assert(era.currentEra==CivilizationEraId::EarlySettlement);

    upsertTechnology(
        technologies,
        technologyStatus(
            TechnologyId::Cultivation,
            TechnologyPopulationState::Operational,1,1));
    world.facilities.push_back(
        operationalFacility(
            3,FacilityKind::CultivatedPlot,{16,0}));
    upsertTransformation(
        transformations,
        transformationStatus(
            CivilizationTransformationId::ManagedFoodProduction,true));

    era=buildCivilizationEraObservation(
        world,technologies,transformations);
    assert(era.currentEra==CivilizationEraId::AgrarianSettlement);
    assert(era.ordinal==2);

    upsertTechnology(
        technologies,
        technologyStatus(
            TechnologyId::CopperSmelting,
            TechnologyPopulationState::Observed));
    era=buildCivilizationEraObservation(
        world,technologies,transformations);
    assert(era.currentEra==CivilizationEraId::AgrarianSettlement);

    upsertTechnology(
        technologies,
        technologyStatus(
            TechnologyId::CopperSmelting,
            TechnologyPopulationState::Operational,1,1));
    world.facilities.push_back(
        operationalFacility(
            4,FacilityKind::Furnace,{24,0}));
    upsertTransformation(
        transformations,
        transformationStatus(
            CivilizationTransformationId::MetallurgicalProduction,true));

    era=buildCivilizationEraObservation(
        world,technologies,transformations);
    assert(era.currentEra==CivilizationEraId::CopperMetallurgy);
    assert(era.ordinal==3);

    upsertTechnology(
        technologies,
        technologyStatus(
            TechnologyId::TinSmelting,
            TechnologyPopulationState::Operational,1,1));
    upsertTechnology(
        technologies,
        technologyStatus(
            TechnologyId::BronzeAlloying,
            TechnologyPopulationState::Operational,1,1));
    upsertTechnology(
        technologies,
        technologyStatus(
            TechnologyId::BronzeAxe,
            TechnologyPopulationState::Operational,1,1));
    upsertTransformation(
        transformations,
        transformationStatus(
            CivilizationTransformationId::AdvancedTooling,true));

    era=buildCivilizationEraObservation(
        world,technologies,transformations);
    assert(era.currentEra==CivilizationEraId::BronzeTechnology);
    assert(era.ordinal==4);
    assert(!era.hasNextEra);

    // Current era is an operational summary, not a historical maximum.
    upsertTechnology(
        technologies,
        technologyStatus(
            TechnologyId::TinSmelting,
            TechnologyPopulationState::Lost));
    upsertTechnology(
        technologies,
        technologyStatus(
            TechnologyId::BronzeAlloying,
            TechnologyPopulationState::Lost));
    upsertTechnology(
        technologies,
        technologyStatus(
            TechnologyId::BronzeAxe,
            TechnologyPopulationState::Lost));
    upsertTransformation(
        transformations,
        transformationStatus(
            CivilizationTransformationId::AdvancedTooling,false));

    era=buildCivilizationEraObservation(
        world,technologies,transformations);
    assert(era.currentEra==CivilizationEraId::CopperMetallurgy);

    for(auto& facility:world.facilities){
        if(facility.kind==FacilityKind::Furnace){
            facility.state=FacilityState::Ruined;
            facility.active=false;
            facility.durability=0.0;
        }
    }
    upsertTechnology(
        technologies,
        technologyStatus(
            TechnologyId::CopperSmelting,
            TechnologyPopulationState::Lost));
    upsertTransformation(
        transformations,
        transformationStatus(
            CivilizationTransformationId::MetallurgicalProduction,false));

    era=buildCivilizationEraObservation(
        world,technologies,transformations);
    assert(era.currentEra==CivilizationEraId::AgrarianSettlement);

    const CivilizationEraId beforeElapsedTime=era.currentEra;
    world.minute+=10000*SimulationMinutesPerDay;
    era=buildCivilizationEraObservation(
        world,technologies,transformations);
    assert(era.currentEra==beforeElapsedTime);

    std::cout<<"civilization era observer classifier passed\n";
    return 0;
}
