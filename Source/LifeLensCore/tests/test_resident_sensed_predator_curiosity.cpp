#include <cassert>
#include "lifelens/ResidentSensedResourceAgency.h"

using namespace lifelens;

int main()
{
    World world(77,0,4);
    world.minute=480;
    world.resourceNodes.clear();
    world.storageSites.clear();
    world.facilities.clear();
    world.characters.clear();
    Character adult;
    adult.id=10;
    adult.civilization.character=10;
    adult.lifeStage=LifeStage::Adult;
    adult.personality.curiosity=1.0;
    adult.needs={0.05,0.05,0.05,0.05,0.05};
    world.characters.push_back(adult);
    const auto origin=chunkOriginGrid(world.initialStartRegion().region.coord);
    GridPos at{};
    bool valid=false;
    for(int x=origin.x+2;x<origin.x+28 &&!valid;++x){
        for(int y=origin.y+2;y<origin.y+28 &&!valid;++y){
            if(coreGroundTraversable(world,{x,y})
               &&coreGroundTraversable(world,{x+1,y}))
            { at={x,y};valid=true; }
        }
    }
    assert(valid);
    world.resourceNodes.push_back(
        {901,MaterialKind::Stone,12,12,false,0,{at.x+1,at.y}});
    RelationshipBook relations;
    ResidentAgencyChoice prior;
    prior.decision.kind=UnifiedDecisionKind::Civilization;
    prior.decision.civilization.intent=CivilizationIntent::Explore;
    prior.decision.utility=0.01;
    const auto originalRng=world.rng;
    const auto calm=chooseResidentSensedResourceGoal(
        world,world.characters.front(),at,relations,prior,nullptr,false);
    assert(calm.personalGoal);
    assert(calm.agency.decision.civilization.resourceNode==901);

    world.wildlifeEnabled=true;
    WildlifePopulation nearby;
    nearby.habitat.coord=chunkCoordForGrid(at);
    WildlifeAnimal tiger;
    tiger.id=707;
    tiger.species=WildlifeSpecies::Tiger;
    tiger.position={at.x+2,at.y};
    tiger.alive=true;
    nearby.animals.push_back(tiger);
    world.wildlifePopulations.push_back(nearby);
    assert(observeResidentPredatorThreat(world,at).detected);
    const auto alert=chooseResidentSensedResourceGoal(
        world,world.characters.front(),at,relations,prior,nullptr,false);
    assert(!alert.personalGoal);
    assert(alert.agency.authority==prior.authority);
    assert(alert.agency.decision.utility==prior.decision.utility);
    assert(world.resourceNodes.front().quantity==12);
    assert(world.characters.front().memory.entries.empty());
    world.wildlifePopulations.front().animals.front().position={at.x+15,at.y};
    assert(!observeResidentPredatorThreat(world,at).detected);
    assert(chooseResidentSensedResourceGoal(
        world,world.characters.front(),at,relations,prior,nullptr,false).personalGoal);
    world.wildlifePopulations.front().animals.front().position={at.x+1,at.y};
    world.wildlifePopulations.front().animals.front().species=WildlifeSpecies::Hare;
    assert(!observeResidentPredatorThreat(world,at).detected);
    assert(chooseResidentSensedResourceGoal(
        world,world.characters.front(),at,relations,prior,nullptr,false).personalGoal);
    assert(world.rng==originalRng);
}
