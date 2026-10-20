#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "Hydrology.h"
#include "Civilization.h"

// Core wildlife domain. Production ticks receive actual resource nodes for
// terrestrial grazing; standalone calls without resource input retain the
// original isolated habitat-potential model. No global RNG or inventory drops.
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

// Actual Core resident positions (not observer/camera positions) constrain
// wild-animal movement. Ordinary wildlife avoids humans. Only severely hungry
// v4+ wolves and tigers may approach a nearby resident. The already existing
// ResidentPredatorResponse Core injury rule alone determines real attacks:
// this movement helper never changes resident health or invents a kill.
// Radii are logical grid cells, not physical metres.
inline int wildlifeHumanAvoidanceRadius(WildlifeSpecies s)
{
    switch(s){
        case WildlifeSpecies::Tiger: return 18;
        case WildlifeSpecies::Wolf: return 14;
        case WildlifeSpecies::Deer: return 10;
        case WildlifeSpecies::Hare: return 8;
        case WildlifeSpecies::FreshwaterFish: return 0;
    }
    return 0;
}

inline std::int64_t wildlifeNearestResidentDistance(
    GridPos pos,const std::vector<GridPos>& residents)
{
    std::int64_t nearest=std::numeric_limits<std::int64_t>::max();
    for(const GridPos other:residents){
        const std::int64_t dx=static_cast<std::int64_t>(pos.x)-other.x;
        const std::int64_t dy=static_cast<std::int64_t>(pos.y)-other.y;
        nearest=std::min(nearest,std::abs(dx)+std::abs(dy));
    }
    return nearest;
}

inline bool wildlifeResidentClose(
    const WildlifeAnimal& animal,const std::vector<GridPos>& residents)
{
    const int radius=wildlifeHumanAvoidanceRadius(animal.species);
    return animal.alive &&radius>0 &&!residents.empty()
        &&wildlifeNearestResidentDistance(animal.position,residents)<=radius;
}

// An already close animal moves away in bounded *single-grid* steps, never
// teleports, never enters a neighbouring unmaterialized chunk, never invents
// an attack or changes the human's position/health. Returns "danger detected"
// even if a border prevents escape, so roaming/hunting cannot walk into humans.
inline bool wildlifeRetreatFromResidents(
    const WildlifePopulation& population,WildlifeAnimal& animal,
    const std::vector<GridPos>& residents,int steps)
{
    if(!wildlifeResidentClose(animal,residents)) return false;
    const GridPos origin=chunkOriginGrid(population.habitat.coord);
    for(int i=0;i<std::max(0,steps);++i){
        const GridPos current=animal.position;
        GridPos best=current;
        std::int64_t bestDistance=wildlifeNearestResidentDistance(
            current,residents);
        for(int dx=-1;dx<=1;++dx){
            for(int dy=-1;dy<=1;++dy){
                const GridPos candidate{
                    std::clamp(current.x+dx,origin.x,
                               origin.x+WildlifeCellsPerChunk-1),
                    std::clamp(current.y+dy,origin.y,
                               origin.y+WildlifeCellsPerChunk-1)
                };
                const auto distance=wildlifeNearestResidentDistance(
                    candidate,residents);
                if(distance>bestDistance){
                    best=candidate;
                    bestDistance=distance;
                }
            }
        }
        if(best.x==current.x &&best.y==current.y) break;
        animal.position=best;
        if(!wildlifeResidentClose(animal,residents)) break;
    }
    return true;
}

// A desperate animal can sometimes take the risk of approaching humans
// instead of indefinitely fleeing them. Ordinary satiated animals retain
// their existing avoidance contract. This is a drive, not a scripted scene,
// and remains disabled for historical pre-v4 snapshots.
inline int wildlifeHungryHumanPursuitRadius(
    const WildlifePopulation& population,const WildlifeAnimal& animal)
{
    if(population.generationVersion<4 ||!animal.alive) return 0;
    if(animal.species==WildlifeSpecies::Tiger)
        return animal.hunger01>=0.82 ?6:0;
    if(animal.species==WildlifeSpecies::Wolf)
        return animal.hunger01>=0.78 ?4:0;
    return 0;
}

inline bool wildlifeHungryHumanPursuitReady(
    const WildlifePopulation& population,const WildlifeAnimal& animal,
    const std::vector<GridPos>& residents)
{
    const int radius=wildlifeHungryHumanPursuitRadius(population,animal);
    return radius>0 &&!residents.empty()
        &&wildlifeNearestResidentDistance(animal.position,residents)<=radius;
}

// One traversable cardinal cell at most per hour. Real positions and
// hydrology decide the result; no teleport, target-inventing proxy,
// cross-chunk chase, water crossing, resident damage or RNG consumption.
inline bool wildlifeApproachHungryResident(
    const WildlifePopulation& population,WildlifeAnimal& animal,
    const std::vector<GridPos>& residents)
{
    if(!wildlifeHungryHumanPursuitReady(
           population,animal,residents)) return false;
    const GridPos before=animal.position;
    const GridPos origin=chunkOriginGrid(population.habitat.coord);
    const auto hydrology=deriveHydrologyFacts(
        makeWorldGenesisIdentity(population.worldSeed,0,
            population.generationVersion),population.habitat.coord);
    constexpr std::array<GridPos,4> directions={{
        {1,0},{0,1},{-1,0},{0,-1}
    }};
    const auto previous=wildlifeNearestResidentDistance(before,residents);
    std::int64_t closest=previous;
    GridPos best=before;
    for(const GridPos direction:directions){
        const GridPos candidate{
            before.x+direction.x,before.y+direction.y
        };
        if(candidate.x<origin.x ||candidate.y<origin.y
           ||candidate.x>=origin.x+WildlifeCellsPerChunk
           ||candidate.y>=origin.y+WildlifeCellsPerChunk
           ||surfaceWaterGroundContainsGrid(hydrology,candidate))
            continue;
        const auto distance=wildlifeNearestResidentDistance(
            candidate,residents);
        if(distance<closest){
            closest=distance;
            best=candidate;
        }
    }
    if(closest>=previous) return false;
    animal.position=best;
    return true;
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

// Apex predators do not occupy every vegetation-rich 32-cell chunk. A
// deterministic, species-specific territory admits at most one founding pack
// or breeding pair across a 2x2 wolf or 4x4 tiger chunk region. This is
// ecological density, not a founder-protection exemption: humans may still
// encounter resident predators and existing Core contact outcomes remain.
// Only NEW v4+ population construction uses these territories; a loaded
// population, v3 fixture, daily survival and saved wildlife are untouched.
inline bool wildlifePredatorFoundingTerritory(
    WorldSeed worldSeed,ChunkCoord coord,WildlifeSpecies species)
{
    if(species!=WildlifeSpecies::Tiger &&species!=WildlifeSpecies::Wolf)
        return true;
    const int width=species==WildlifeSpecies::Tiger ?4:2;
    // C++ division truncates toward zero. Use floor division so territories
    // tile negative and positive chunk coordinates without discontinuities.
    const auto territoryIndex=[width](int coordinate)->int {
        const std::int64_t value=coordinate;
        return static_cast<int>(value>=0
            ?value/width :(value-width+1)/width);
    };
    const ChunkCoord territory{
        territoryIndex(coord.x),territoryIndex(coord.y)
    };
    const std::uint64_t placement=wildlifeHash(
        worldSeed,territory,static_cast<std::uint64_t>(species),0,
        0x5445525249544f52ULL); // TERRITOR
    const int localX=static_cast<int>(
        static_cast<std::int64_t>(coord.x)
        -static_cast<std::int64_t>(territory.x)*width);
    const int localY=static_cast<int>(
        static_cast<std::int64_t>(coord.y)
        -static_cast<std::int64_t>(territory.y)*width);
    return localX==static_cast<int>(placement%width)
        &&localY==static_cast<int>((placement>>16U)%width);
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
        const auto species=static_cast<WildlifeSpecies>(s);
        if(state.generationVersion>=4
           &&!wildlifePredatorFoundingTerritory(
               state.worldSeed,habitat.coord,species))
            continue;
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

// Herbivores share the same finite plant stock used by residents. Mature
// timber is not food: only deer browsing young regrowth may remove wood units.
inline int wildlifeEdiblePlantUnits(
    const ResourceNode& node,WildlifeSpecies species,int minute)
{
    if(species!=WildlifeSpecies::Hare && species!=WildlifeSpecies::Deer)
        return 0;
    if(node.material==MaterialKind::PlantFood || node.material==MaterialKind::Fiber)
        return std::max(0,node.quantity);
    if(species!=WildlifeSpecies::Deer || node.material!=MaterialKind::Wood)
        return 0;
    int units=0;
    for(const auto& tree:node.trees){
        const auto age=static_cast<std::int64_t>(minute)-tree.bornMinute;
        if(age>=0 && age<365LL*1440) units+=std::max(0,tree.remainingWood);
    }
    return std::min(std::max(0,node.quantity),units);
}

inline const ResourceNode* wildlifeNearestPlant(
    const WildlifePopulation& population,const WildlifeAnimal& animal,
    const std::vector<ResourceNode>& resources,int minute,int radius)
{
    const ResourceNode* best=nullptr;
    int nearest=radius+1;
    for(const auto& node:resources){
        if(!(chunkCoordForGrid(node.pos)==population.habitat.coord)
           ||wildlifeEdiblePlantUnits(node,animal.species,minute)<=0) continue;
        const int distance=std::abs(node.pos.x-animal.position.x)
            +std::abs(node.pos.y-animal.position.y);
        if(distance<=radius && (distance<nearest
           ||(distance==nearest && (!best ||node.id<best->id)))){
            best=&node;
            nearest=distance;
        }
    }
    return best;
}

inline int wildlifeConsumePlants(
    WildlifePopulation& population,const WildlifeAnimal& animal,
    std::vector<ResourceNode>& resources,int minute)
{
    const ResourceNode* source=wildlifeNearestPlant(
        population,animal,resources,minute,2);
    if(source==nullptr) return 0;
    const auto index=static_cast<std::size_t>(source-resources.data());
    auto& node=resources[index];
    const int taken=std::min(animal.species==WildlifeSpecies::Deer?3:1,
        wildlifeEdiblePlantUnits(node,animal.species,minute));
    if(node.material==MaterialKind::Wood){
        int left=taken;
        for(auto& tree:node.trees){
            const auto age=static_cast<std::int64_t>(minute)-tree.bornMinute;
            if(age<0 || age>=365LL*1440) continue;
            const int bite=std::min(left,std::max(0,tree.remainingWood));
            tree.remainingWood-=bite;
            left-=bite;
            if(bite>0 &&tree.remainingWood==0) tree.felledMinute=minute;
            if(left==0) break;
        }
    }
    node.quantity-=taken;
    // Refresh reproductive visuals from current stock; growth remains owned
    // by the existing natural ecology/regeneration cadence.
    const double stock=node.maxQuantity>0
        ?std::clamp(static_cast<double>(node.quantity)/node.maxQuantity,0.0,1.0):0.0;
    node.flower01=std::min(node.flower01,stock);
    node.fruit01=std::min(node.fruit01,stock);
    return taken;
}

inline bool wildlifeApproachPlants(
    WildlifePopulation& population,WildlifeAnimal& animal,
    const std::vector<ResourceNode>& resources,int minute,
    const std::vector<GridPos>& residents)
{
    if(animal.hunger01<0.12) return false;
    const ResourceNode* source=wildlifeNearestPlant(
        population,animal,resources,minute,2*WildlifeCellsPerChunk);
    if(source==nullptr) return false;
    const auto distance=[&](GridPos p){
        return std::abs(source->pos.x-p.x)+std::abs(source->pos.y-p.y);
    };
    if(distance(animal.position)<=2) return true;
    const auto hydro=deriveHydrologyFacts(
        makeWorldGenesisIdentity(population.worldSeed,0,population.generationVersion),
        population.habitat.coord);
    GridPos best=animal.position;
    int nearest=distance(best);
    constexpr std::array<GridPos,4> directions={{{1,0},{0,1},{-1,0},{0,-1}}};
    for(const GridPos d:directions){
        const GridPos candidate{animal.position.x+d.x,animal.position.y+d.y};
        if(!(chunkCoordForGrid(candidate)==population.habitat.coord)
           ||surfaceWaterGroundContainsGrid(hydro,candidate)
           ||(!residents.empty() &&wildlifeNearestResidentDistance(candidate,residents)
                <=wildlifeHumanAvoidanceRadius(animal.species))) continue;
        if(distance(candidate)<nearest){best=candidate;nearest=distance(candidate);}
    }
    const bool moved=best.x!=animal.position.x ||best.y!=animal.position.y;
    animal.position=best;
    return moved;
}

// Explicit one-day Core-domain transition. Calling twice for the same day
// never produces extra food, offspring, or RNG draws. No time-skips allowed.
inline WildlifeStepResult stepWildlifeDay(
    WildlifePopulation& state,std::int64_t targetDay,
    const std::vector<GridPos>& residentPositions={},
    std::vector<ResourceNode>* resources=nullptr)
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
        const bool humanNear=wildlifeResidentClose(animal,residentPositions);
        if(predator && !humanNear){
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
        }else if(!humanNear && !predator){
            if(!fish &&resources!=nullptr){
                const int minute=static_cast<int>(std::min<std::int64_t>(
                    targetDay,std::numeric_limits<int>::max()/1440)*1440);
                const int eaten=wildlifeConsumePlants(state,animal,*resources,minute);
                const int required=animal.species==WildlifeSpecies::Deer?3:1;
                animal.hunger01=std::max(0.0,animal.hunger01
                    -0.28*static_cast<double>(eaten)/required);
                if(eaten>0) ++report.foraged;
            }else{
                const double chance=fish?0.68:std::clamp(
                    0.15+0.68*state.habitat.forage01,0.0,0.90);
                ate=wildlifeRoll(state,animal.id,targetDay,0x464f52414745ULL)<chance;
            }
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
        if(humanNear){
            const GridPos before=animal.position;
            wildlifeRetreatFromResidents(state,animal,residentPositions,3);
            if(before.x!=animal.position.x ||before.y!=animal.position.y)
                ++report.moved;
            continue;
        }
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
    const WildlifePopulation& population,const WildlifeAnimal& animal,
    const std::vector<GridPos>& residentPositions={})
{
    if(animal.species==WildlifeSpecies::FreshwaterFish)
        return WildlifeBehavior::Swim;
    if(wildlifeHungryHumanPursuitReady(
           population,animal,residentPositions))
        return WildlifeBehavior::Hunt;
    if(wildlifeResidentClose(animal,residentPositions))
        return WildlifeBehavior::Flee;
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
    // Water present somewhere in a chunk does not prove the animal reached
    // a drinkable shore. Only report Drink after a real source/path gate exists.
    return WildlifeBehavior::Rest;
}

// One authoritative spatial step per simulation hour. Age, hunger, births,
// hunting kills and capacity remain day-owned; no new snapshot fields or
// camera-driven ecology. Call only on the absolute Core hour boundary.
inline int stepWildlifeHour(
    WildlifePopulation& population,std::int64_t hourIndex,
    const std::vector<GridPos>& residentPositions={},
    const std::vector<ResourceNode>* resources=nullptr)
{
    if(!population.habitat.available || hourIndex<=0) return 0;
    const GridPos origin=chunkOriginGrid(population.habitat.coord);
    int moved=0;
    for(auto& animal:population.animals){
        if(!animal.alive || animal.species==WildlifeSpecies::FreshwaterFish)
            continue;
        const GridPos before=animal.position;
        if(wildlifeApproachHungryResident(
               population,animal,residentPositions)){
            ++moved;
            continue;
        }
        if(wildlifeRetreatFromResidents(
               population,animal,residentPositions,3)){
            if(before.x!=animal.position.x ||before.y!=animal.position.y)
                ++moved;
            continue;
        }
        const auto behavior=wildlifeBehavior(population,animal,residentPositions);
        if(resources!=nullptr &&behavior!=WildlifeBehavior::Flee
           &&wildlifeApproachPlants(population,animal,*resources,
                static_cast<int>(std::min<std::int64_t>(
                    hourIndex,std::numeric_limits<int>::max()/60)*60),residentPositions)){
            if(before.x!=animal.position.x ||before.y!=animal.position.y) ++moved;
            continue;
        }
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
