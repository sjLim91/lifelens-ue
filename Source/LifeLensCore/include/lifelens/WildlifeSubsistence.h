#pragma once

#include "World.h"
#include "LifeStage.h"
#include "ToolEffectiveness.h"

namespace lifelens {

// Food is consumed at the kill site in this first subsistence loop. There is
// no invented PlantFood/meat stack, remote slaughter or carcass save state.
inline const WildlifeAnimal* findSubsistenceAnimal(
    const World& world,std::uint64_t id)
{
    if(!world.wildlifeEnabled ||id==0) return nullptr;
    for(const auto& population:world.wildlifePopulations)
        for(const auto& animal:population.animals)
            if(animal.id==id &&animal.alive) return &animal;
    return nullptr;
}

inline bool residentCanHuntSpecies(const Character& actor,WildlifeSpecies species)
{
    if(!actor.alive ||!lifeStageProfile(actor.lifeStage).canWork) return false;
    if(species==WildlifeSpecies::Hare) return true;
    if(species!=WildlifeSpecies::Deer) return false;
    return inspectGatherTool(actor.civilization.inventory,MaterialKind::Hide).available
        ||inspectGatherTool(actor.civilization.inventory,MaterialKind::Wood).available;
}

inline bool subsistenceHuntSafe(const World& world,GridPos target)
{
    for(const auto& population:world.wildlifePopulations)
        for(const auto& animal:population.animals){
            if(!animal.alive ||(animal.species!=WildlifeSpecies::Wolf
                &&animal.species!=WildlifeSpecies::Tiger)) continue;
            if(std::abs(animal.position.x-target.x)
               +std::abs(animal.position.y-target.y)<=8) return false;
        }
    return true;
}

struct SubsistenceHuntResult {
    bool attempted=false;
    bool caught=false;
    double hungerRelief=0.0;
};

inline SubsistenceHuntResult completeSubsistenceHunt(
    World& world,Character& actor,std::uint64_t id,GridPos position)
{
    SubsistenceHuntResult result;
    const WildlifeAnimal* target=findSubsistenceAnimal(world,id);
    if(!target ||!residentCanHuntSpecies(actor,target->species)
       ||actor.needs.hunger<0.20 ||!subsistenceHuntSafe(world,target->position)
       ||std::abs(position.x-target->position.x)>1
       ||std::abs(position.y-target->position.y)>1) return result;
    const auto species=target->species;
    const auto toolMaterial=inspectGatherTool(
        actor.civilization.inventory,MaterialKind::Hide).available
            ?MaterialKind::Hide:MaterialKind::Wood;
    const bool tool=inspectGatherTool(actor.civilization.inventory,toolMaterial).available;
    const double chance=std::clamp(
        (species==WildlifeSpecies::Hare?0.40:0.30)
        +0.35*actor.civilization.gatheringSkill+(tool?0.15:0.0),0.0,0.90);
    const double roll=macroUnitFromWord(wildlifeHash(
        world.seed,chunkCoordForGrid(target->position),id^actor.id,
        static_cast<std::uint64_t>(world.minute/20),0x48554e54454154ULL));
    result.attempted=true;
    if(tool) consumeGatherToolUse(actor.civilization.inventory,toolMaterial);
    if(roll>=chance) return result;
    for(auto& population:world.wildlifePopulations){
        const auto found=std::find_if(population.animals.begin(),population.animals.end(),
            [id](const WildlifeAnimal& animal){return animal.id==id &&animal.alive;});
        if(found==population.animals.end()) continue;
        // Remove the actual individual immediately; dead cached animals are
        // forbidden by the existing wildlife binary codec.
        population.animals.erase(found);
        result.caught=true;
        const double before=actor.needs.hunger;
        actor.needs.hunger=std::max(0.0,before
            -(species==WildlifeSpecies::Hare?0.55:0.70));
        result.hungerRelief=before-actor.needs.hunger;
        actor.civilization.gatheringSkill=std::min(
            1.0,actor.civilization.gatheringSkill+0.003);
        break;
    }
    return result;
}

} // namespace lifelens
