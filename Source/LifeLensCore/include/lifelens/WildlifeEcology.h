#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "Hydrology.h"

// ECO-A1: opt-in, isolated Core wildlife domain prototype.
// No implicit construction, mutation of World/Simulation, RNG calls, resource
// drops, actor DTOs, inventory writes or snapshot-schema changes.
// The habitat source below is *procedural Core terrain*, NOT real-world
// species distribution, actual weather, or geographical climate conditioning.
namespace lifelens {

enum class WildlifeSpecies : std::uint8_t {
    Hare=0, Deer=1, Wolf=2, FreshwaterFish=3, Tiger=4
};
enum class WildlifeSex : std::uint8_t { Female=0, Male=1 };

inline constexpr std::size_t WildlifeSpeciesCount=5;
inline constexpr int WildlifeCellsPerChunk=WorldChunkSpanGridCells;
inline constexpr std::uint64_t WildlifeDomain=0x45434f57494c4431ULL; // ECOWILD1

struct WildlifeHabitat {
    bool available=false;
    ChunkCoord coord{};
    // Never promote procedural habitats to verified actual-Earth habitats.
    bool geographicallyConditioned=false;
    bool regionalWeatherApplied=false;
    bool freshwaterAvailable=false;
    double forage01=0.0;
    double shelter01=0.0;
    double moisture01=0.0;
    std::array<int,WildlifeSpeciesCount> carryingCapacity{};
};

inline WildlifeHabitat deriveWildlifeHabitat(
    const WorldGenesisIdentity& identity,ChunkCoord coord)
{
    WildlifeHabitat result{};
    const MacroRegionFacts facts=deriveMacroRegionFacts(identity,coord);
    const MacroSurfaceFacts surface=deriveMacroSurfaceFacts(identity,coord);
    const HydrologyFacts water=deriveHydrologyFacts(identity,coord);
    result.available=true;
    result.coord=coord;
    result.freshwaterAvailable=isFreshSurfaceWater(water);
    result.forage01=clampMacro01(facts.foodPotential);
    result.shelter01=clampMacro01(facts.woodPotential);
    result.moisture01=clampMacro01(facts.moisture);
    // Macro ocean is NOT a freshwater hunting/fishing ground.
    if(surface.surfaceClass==MacroSurfaceClass::Ocean) return result;

    const double forage=result.forage01;
    const double shelter=result.shelter01;
    const double comfort=clampMacro01(1.0-std::abs(facts.temperature-0.55)*1.7);
    const double hareSuitability=forage*comfort*(0.65+0.35*shelter);
    const double deerSuitability=forage*comfort
        *(result.freshwaterAvailable?1.0:0.5);
    result.carryingCapacity[0]=hareSuitability>0.15
        ? std::min(24,2+static_cast<int>(20*hareSuitability)):0;
    result.carryingCapacity[1]=deerSuitability>0.22
        ? std::min(12,2+static_cast<int>(10*deerSuitability)):0;
    const int preyCapacity=result.carryingCapacity[0]+result.carryingCapacity[1];
    result.carryingCapacity[2]=preyCapacity>=8
        ? std::min(4,preyCapacity/7):0;
    result.carryingCapacity[3]=result.freshwaterAvailable
        ? std::min(32,4+static_cast<int>(25*water.surfaceAvailability)):0;
    // Tigers are rare, solitary apex predators of wooded deer habitats.
    // Historical four-species snapshot populations retain zero tiger capacity.
    result.carryingCapacity[4]=
        deerSuitability>0.50 && shelter>0.65 && preyCapacity>=13
        ? 2 : 0;
    return result;
}

struct WildlifeAnimal {
    std::uint64_t id=0;
    WildlifeSpecies species=WildlifeSpecies::Hare;
    WildlifeSex sex=WildlifeSex::Female;
    GridPos position{};
    int ageDays=0;
    double hunger01=0.0;
    bool alive=true;
};

struct WildlifePopulation {
    WorldSeed worldSeed=0;
    WorldGenerationVersion generationVersion=0;
    WildlifeHabitat habitat{};
    std::int64_t dayIndex=0;
    std::uint32_t nextSerial=1;
    std::vector<WildlifeAnimal> animals;
};

struct WildlifeStepResult {
    bool advanced=false;
    int births=0;
    int naturalDeaths=0;
    int predationDeaths=0;
    int foraged=0;
    int moved=0;
};

inline std::uint64_t wildlifeHash(
    WorldSeed seed,ChunkCoord chunk,
    std::uint64_t entity,std::uint64_t day,std::uint64_t domain)
{
    std::uint64_t state=worldGenesisMix64(
        normalizeWorldSeed(seed)^WildlifeDomain);
    state=combineWorldGenesisWord(state,stableSignedCoordinateWord(chunk.x));
    state=combineWorldGenesisWord(state,stableSignedCoordinateWord(chunk.y));
    state=combineWorldGenesisWord(state,entity);
    state=combineWorldGenesisWord(state,day);
    return worldGenesisMix64(state^domain);
}

inline double wildlifeRoll(
    const WildlifePopulation& world,std::uint64_t entity,
    std::int64_t day,std::uint64_t domain)
{
    return macroUnitFromWord(wildlifeHash(world.worldSeed,world.habitat.coord,
        entity,static_cast<std::uint64_t>(day),domain));
}

inline std::size_t wildlifeSpeciesIndex(WildlifeSpecies s)
{
    return static_cast<std::size_t>(s);
}

inline int wildlifeAliveCount(const WildlifePopulation& state,WildlifeSpecies s)
{
    return static_cast<int>(std::count_if(
        state.animals.begin(),state.animals.end(),
        [s](const WildlifeAnimal& a){return a.alive && a.species==s;}));
}

inline int wildlifeMaturityDays(WildlifeSpecies s)
{
    switch(s){
        case WildlifeSpecies::Hare: return 35;
        case WildlifeSpecies::Deer: return 420;
        case WildlifeSpecies::Wolf: return 600;
        case WildlifeSpecies::FreshwaterFish: return 75;
        case WildlifeSpecies::Tiger: return 900;
    }
    return 1000000;
}

inline int wildlifeMaximumAgeDays(WildlifeSpecies s)
{
    switch(s){
        case WildlifeSpecies::Hare: return 1100;
        case WildlifeSpecies::Deer: return 4700;
        case WildlifeSpecies::Wolf: return 4200;
        case WildlifeSpecies::FreshwaterFish: return 1500;
        case WildlifeSpecies::Tiger: return 5800;
    }
    return 1;
}

inline double wildlifeBirthProbability(WildlifeSpecies s)
{
    switch(s){
        case WildlifeSpecies::Hare: return 0.035;
        case WildlifeSpecies::Deer: return 0.004;
        case WildlifeSpecies::Wolf: return 0.003;
        case WildlifeSpecies::FreshwaterFish: return 0.025;
        case WildlifeSpecies::Tiger: return 0.0015;
    }
    return 0.0;
}

inline WildlifeAnimal wildlifeCreateAnimal(
    WildlifePopulation& state,WildlifeSpecies s,bool adult)
{
    WildlifeAnimal animal;
    // Unique within a population, even after dead animals are compacted.
    // The high 32 bits are world+chunk bound, the low 32 bits are a serial.
    const std::uint64_t prefix=wildlifeHash(
        state.worldSeed,state.habitat.coord,0,0,0x4944454e54495459ULL);
    animal.id=(prefix&0xffffffff00000000ULL)|state.nextSerial++;
    animal.species=s;
    animal.sex=(wildlifeHash(state.worldSeed,state.habitat.coord,animal.id,0,
        0x534558ULL)&1ULL)?WildlifeSex::Male:WildlifeSex::Female;
    const std::uint64_t offset=wildlifeHash(
        state.worldSeed,state.habitat.coord,animal.id,0,0x504f534954494f4eULL);
    const GridPos origin=chunkOriginGrid(state.habitat.coord);
    animal.position={
        origin.x+static_cast<int>(offset%WildlifeCellsPerChunk),
        origin.y+static_cast<int>((offset>>16U)%WildlifeCellsPerChunk)
    };
    // Fish are confined to the verified Core fresh-water source chunk.
    // Water-body swimming/pathfinding is a separate ECO-A2 spatial gate.
    if(s==WildlifeSpecies::FreshwaterFish){
        animal.position={origin.x+WildlifeCellsPerChunk/2,
                         origin.y+WildlifeCellsPerChunk/2};
    }
    animal.ageDays=adult
        ? wildlifeMaturityDays(s)+static_cast<int>((offset>>32U)%37U)
        : 0;
    animal.hunger01=adult?0.2:0.1;
    return animal;
}

inline WildlifePopulation startWildlifePopulation(
    const WorldGenesisIdentity& identity,
    const WildlifeHabitat& habitat,std::int64_t startDay=0)
{
    WildlifePopulation state;
    state.worldSeed=normalizeWorldSeed(identity.worldSeed);
    state.generationVersion=normalizeWorldGenerationVersion(identity.generationVersion);
    state.habitat=habitat;
    state.dayIndex=std::max<std::int64_t>(0,startDay);
    if(!habitat.available) return state;
    for(std::size_t s=0;s<WildlifeSpeciesCount;++s){
        const int cap=std::max(0,habitat.carryingCapacity[s]);
        const int count=cap<2?0:std::min(cap,std::max(2,cap/3));
        for(int i=0;i<count;++i){
            auto animal=wildlifeCreateAnimal(state,static_cast<WildlifeSpecies>(s),true);
            // Guarantee a founding mating pair; subsequent offspring sex is
            // derived from a stable independent random domain.
            if(i==0) animal.sex=WildlifeSex::Female;
            if(i==1) animal.sex=WildlifeSex::Male;
            state.animals.push_back(animal);
        }
    }
    return state;
}

// Explicit one-day Core-domain transition. Calling twice for the same day
// never produces extra food, offspring, or RNG draws. No time-skips allowed.
inline WildlifeStepResult stepWildlifeDay(
    WildlifePopulation& state,std::int64_t targetDay)
{
    WildlifeStepResult report;
    if(!state.habitat.available
       ||targetDay<=state.dayIndex
       ||targetDay-state.dayIndex!=1
       ||state.nextSerial==0) return report;
    state.dayIndex=targetDay;
    report.advanced=true;

    for(auto& animal:state.animals){
        if(!animal.alive) continue;
        ++animal.ageDays;
        if(animal.ageDays>wildlifeMaximumAgeDays(animal.species)){
            animal.alive=false;
            ++report.naturalDeaths;
            continue;
        }

        const bool tiger=animal.species==WildlifeSpecies::Tiger;
        const bool predator=animal.species==WildlifeSpecies::Wolf || tiger;
        const bool fish=animal.species==WildlifeSpecies::FreshwaterFish;
        animal.hunger01=std::min(1.0,animal.hunger01+(tiger?0.16:predator?0.20:fish?0.085:0.12));
        bool ate=false;
        bool pursuedPrey=false;
        if(predator){
            WildlifeAnimal* prey=nullptr;
            int nearestDistance=std::numeric_limits<int>::max();
            for(auto& candidate:state.animals){
                if(!candidate.alive
                   ||(candidate.species!=WildlifeSpecies::Hare
                      &&candidate.species!=WildlifeSpecies::Deer)) continue;
                const int distance=std::abs(candidate.position.x-animal.position.x)
                    +std::abs(candidate.position.y-animal.position.y);
                if(distance<nearestDistance
                   ||(distance==nearestDistance
                      &&(prey==nullptr || candidate.id<prey->id))){
                    nearestDistance=distance;
                    prey=&candidate;
                }
            }
            // A wolf may smell nearby prey, but cannot hunt at infinite range.
            // Spatial chase is still limited to one step per axis and remains
            // inside the same chunk until verified multi-chunk pathing exists.
            if(prey!=nullptr && nearestDistance<=(tiger?8:12)){
                pursuedPrey=true;
                const int dx=(prey->position.x>animal.position.x)
                    -(prey->position.x<animal.position.x);
                const int dy=(prey->position.y>animal.position.y)
                    -(prey->position.y<animal.position.y);
                animal.position.x+=dx;
                animal.position.y+=dy;
                if(dx!=0 || dy!=0) ++report.moved;
                const int afterDistance=
                    std::abs(prey->position.x-animal.position.x)
                    +std::abs(prey->position.y-animal.position.y);
                if(afterDistance<=2
                   &&wildlifeRoll(state,animal.id,targetDay,
                        0x50524544415445ULL)<(tiger?0.54:0.40)){
                    prey->alive=false;
                    ++report.predationDeaths;
                    ate=true;
                }
            }
        }else{
            const double chance=fish?0.68:std::clamp(
                0.15+0.68*state.habitat.forage01,0.0,0.90);
            ate=wildlifeRoll(state,animal.id,targetDay,0x464f52414745ULL)<chance;
        }
        if(ate){
            animal.hunger01=std::max(0.0,animal.hunger01-(tiger?0.88:predator?0.7:0.28));
            ++report.foraged;
        }
        if(animal.hunger01>=0.999){
            animal.alive=false;
            ++report.naturalDeaths;
            continue;
        }
        if(fish || pursuedPrey) continue;
        const std::uint64_t choice=wildlifeHash(
            state.worldSeed,state.habitat.coord,
            animal.id,static_cast<std::uint64_t>(targetDay),0x4d4f5645ULL);
        const GridPos origin=chunkOriginGrid(state.habitat.coord);
        // Movement is intentionally confined to a known procedural chunk.
        // Not a pathfinder or geography-aware migration yet.
        const int dx=static_cast<int>(choice%3)-1;
        const int dy=static_cast<int>((choice>>8U)%3)-1;
        const int x=animal.position.x;
        const int y=animal.position.y;
        animal.position.x=std::clamp(x+dx,origin.x,origin.x+WildlifeCellsPerChunk-1);
        animal.position.y=std::clamp(y+dy,origin.y,origin.y+WildlifeCellsPerChunk-1);
        if(x!=animal.position.x || y!=animal.position.y) ++report.moved;
    }

    // Old deaths leave no immortal cached corpses in the active array. The
    // birth decisions operate on the survivors only and respect exact caps.
    state.animals.erase(std::remove_if(state.animals.begin(),state.animals.end(),
        [](const WildlifeAnimal& a){return !a.alive;}),state.animals.end());
    const std::size_t originalCount=state.animals.size();
    for(std::size_t i=0;i<originalCount;++i){
        const WildlifeAnimal parent=state.animals[i];
        const auto s=parent.species;
        if(parent.sex!=WildlifeSex::Female
            ||parent.ageDays<wildlifeMaturityDays(s)
            ||parent.hunger01>0.55
            ||wildlifeAliveCount(state,s)>=state.habitat.carryingCapacity[
                wildlifeSpeciesIndex(s)]) continue;
        const bool hasMate=std::any_of(state.animals.begin(),state.animals.end(),
            [s](const WildlifeAnimal& other){
                return other.species==s && other.sex==WildlifeSex::Male
                    && other.ageDays>=wildlifeMaturityDays(s);
            });
        if(!hasMate) continue;
        if(wildlifeRoll(state,parent.id,targetDay,0x4249525448ULL)
            >=wildlifeBirthProbability(s)) continue;
        if(state.nextSerial==std::numeric_limits<std::uint32_t>::max()) continue;
        state.animals.push_back(wildlifeCreateAnimal(state,s,false));
        ++report.births;
    }
    return report;
}

enum class WildlifeBehavior : std::uint8_t {
    Rest, Roam, Forage, Drink, Flee, Hunt, Swim
};

// Read-only Core explanation for observers; never fabricate browser agency.
inline WildlifeBehavior wildlifeBehavior(
    const WildlifePopulation& population,const WildlifeAnimal& animal)
{
    if(animal.species==WildlifeSpecies::FreshwaterFish)
        return WildlifeBehavior::Swim;
    const bool predator=animal.species==WildlifeSpecies::Wolf
        || animal.species==WildlifeSpecies::Tiger;
    int nearestPrey=1000000,nearestThreat=1000000;
    for(const auto& other:population.animals){
        if(!other.alive || other.id==animal.id) continue;
        const int d=std::abs(other.position.x-animal.position.x)
            +std::abs(other.position.y-animal.position.y);
        if(predator && (other.species==WildlifeSpecies::Deer
                         ||other.species==WildlifeSpecies::Hare))
            nearestPrey=std::min(nearestPrey,d);
        if(!predator && (other.species==WildlifeSpecies::Tiger
                          ||other.species==WildlifeSpecies::Wolf))
            nearestThreat=std::min(nearestThreat,d);
    }
    if(!predator && nearestThreat<=7) return WildlifeBehavior::Flee;
    if(predator && nearestPrey<=(animal.species==WildlifeSpecies::Tiger?10:14)
       && animal.hunger01>=0.18) return WildlifeBehavior::Hunt;
    if(animal.hunger01>=0.40) return predator
        ? WildlifeBehavior::Roam : WildlifeBehavior::Forage;
    if(!predator && population.habitat.freshwaterAvailable
       && animal.hunger01>.28) return WildlifeBehavior::Drink;
    return WildlifeBehavior::Rest;
}

// One authoritative spatial step per simulation hour. Age, hunger, births,
// hunting kills and capacity remain day-owned; no new snapshot fields or
// camera-driven ecology. Call only on the absolute Core hour boundary.
inline int stepWildlifeHour(WildlifePopulation& population,std::int64_t hourIndex)
{
    if(!population.habitat.available || hourIndex<=0) return 0;
    const GridPos origin=chunkOriginGrid(population.habitat.coord);
    int moved=0;
    for(auto& animal:population.animals){
        if(!animal.alive || animal.species==WildlifeSpecies::FreshwaterFish)
            continue;
        const auto behavior=wildlifeBehavior(population,animal);
        const std::uint64_t roll=wildlifeHash(
            population.worldSeed,population.habitat.coord,animal.id,
            static_cast<std::uint64_t>(hourIndex),0x57494c44484f5552ULL);
        // Rest is meaningful: not all animals wander without purpose.
        if(behavior==WildlifeBehavior::Rest && (roll&3ULL)!=0) continue;
        if(behavior==WildlifeBehavior::Forage && (roll&1ULL)==0) continue;
        int dx=static_cast<int>(roll%3)-1;
        int dy=static_cast<int>((roll>>8U)%3)-1;
        int nearest=1000000;
        for(const auto& target:population.animals){
            if(!target.alive || target.id==animal.id) continue;
            const bool prey=(target.species==WildlifeSpecies::Hare
                ||target.species==WildlifeSpecies::Deer);
            const bool predator=(target.species==WildlifeSpecies::Wolf
                ||target.species==WildlifeSpecies::Tiger);
            if(!((behavior==WildlifeBehavior::Hunt && prey)
                ||(behavior==WildlifeBehavior::Flee && predator))) continue;
            const int d=std::abs(target.position.x-animal.position.x)
                +std::abs(target.position.y-animal.position.y);
            if(d<nearest || (d==nearest
                && (wildlifeHash(population.worldSeed,population.habitat.coord,
                    target.id,static_cast<std::uint64_t>(hourIndex),0x544945ULL)&1ULL))){
                nearest=d;
                const int sign=behavior==WildlifeBehavior::Flee?-1:1;
                dx=sign*((target.position.x>animal.position.x)
                     -(target.position.x<animal.position.x));
                dy=sign*((target.position.y>animal.position.y)
                     -(target.position.y<animal.position.y));
            }
        }
        const int range=behavior==WildlifeBehavior::Flee?2:1;
        const int oldX=animal.position.x,oldY=animal.position.y;
        animal.position.x=std::clamp(oldX+dx*range,
            origin.x,origin.x+WildlifeCellsPerChunk-1);
        animal.position.y=std::clamp(oldY+dy*range,
            origin.y,origin.y+WildlifeCellsPerChunk-1);
        if(oldX!=animal.position.x || oldY!=animal.position.y) ++moved;
    }
    return moved;
}

} // namespace lifelens
