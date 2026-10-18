#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

#include "CoreNavigation.h"
#include "WildlifeEcology.h"

namespace lifelens {

// Resident survival response is grounded in live Core animal coordinates.
// No invented attack, health damage, animal death, or teleportation.
struct ResidentPredatorThreat {
    bool detected=false;
    WildlifeSpecies species=WildlifeSpecies::Tiger;
    std::uint64_t animalId=0;
    GridPos position{};
    std::int64_t distance=std::numeric_limits<std::int64_t>::max();
};

inline int residentPredatorDetectionRadius(WildlifeSpecies species)
{
    switch(species){
        case WildlifeSpecies::Tiger: return 7;
        case WildlifeSpecies::Wolf: return 5;
        default: return 0;
    }
}

inline std::int64_t residentPredatorDistance(GridPos from,GridPos to)
{
    return std::abs(static_cast<std::int64_t>(from.x)-to.x)
         + std::abs(static_cast<std::int64_t>(from.y)-to.y);
}

template<typename Visitor>
inline void forEachLocalResidentPredator(
    const World& world,GridPos resident,Visitor&& visitor)
{
    if(!world.wildlifeEnabled) return;
    const ChunkCoord near=chunkCoordForGrid(resident);
    for(const auto& population:world.wildlifePopulations){
        const auto coord=population.habitat.coord;
        // A distant/unmaterialized ecological area cannot be a perceived
        // animal. Animals themselves have global grid positions.
        if(std::abs(static_cast<std::int64_t>(coord.x)-near.x)>1
           ||std::abs(static_cast<std::int64_t>(coord.y)-near.y)>1)
            continue;
        for(const auto& animal:population.animals){
            if(!animal.alive
               ||residentPredatorDetectionRadius(animal.species)==0)
                continue;
            visitor(animal);
        }
    }
}

inline ResidentPredatorThreat observeResidentPredatorThreat(
    const World& world,GridPos resident)
{
    ResidentPredatorThreat threat;
    forEachLocalResidentPredator(world,resident,[&](const WildlifeAnimal& animal){
        const auto distance=residentPredatorDistance(resident,animal.position);
        if(distance>residentPredatorDetectionRadius(animal.species)) return;
        if(!threat.detected ||distance<threat.distance
           ||(distance==threat.distance &&animal.id<threat.animalId)){
            threat.detected=true;
            threat.species=animal.species;
            threat.animalId=animal.id;
            threat.position=animal.position;
            threat.distance=distance;
        }
    });
    return threat;
}

inline std::int64_t nearestResidentPredatorDistance(
    const World& world,GridPos resident)
{
    std::int64_t distance=std::numeric_limits<std::int64_t>::max();
    forEachLocalResidentPredator(world,resident,[&](const WildlifeAnimal& animal){
        distance=std::min(distance,
            residentPredatorDistance(resident,animal.position));
    });
    return distance;
}

// Propose one physically traversable cardinal step. The Simulation must
// still move through advanceNavigation (terrain/weather/health speed checks).
// Choose a step that *strictly* increases clearance from the nearest local
// predator, not a magical move to a preselected safe zone.
inline bool chooseResidentPredatorEscapeStep(
    const World& world,GridPos resident,GridPos& outNext)
{
    const auto threat=observeResidentPredatorThreat(world,resident);
    if(!threat.detected) return false;

    const std::int64_t current=nearestResidentPredatorDistance(world,resident);
    std::int64_t bestDistance=current;
    double bestCost=std::numeric_limits<double>::infinity();
    bool found=false;
    // Fixed deterministic order for ties.
    constexpr std::array<GridPos,4> steps={{{1,0},{0,1},{-1,0},{0,-1}}};
    for(const GridPos direction:steps){
        const GridPos candidate{
            resident.x+direction.x,resident.y+direction.y
        };
        if(!coreGroundTraversable(world,candidate)) continue;
        const auto clearance=nearestResidentPredatorDistance(world,candidate);
        if(clearance<=current) continue;
        const double cost=coreGroundTraversalCost(world,resident,candidate);
        if(!found ||clearance>bestDistance
           ||(clearance==bestDistance &&cost<bestCost)){
            outNext=candidate;
            bestDistance=clearance;
            bestCost=cost;
            found=true;
        }
    }
    return found;
}

inline bool recentResidentPredatorSighting(
    const Character& resident,int minute,std::uint64_t animalId)
{
    const std::string target=std::to_string(animalId);
    for(const auto& memory:resident.memory.entries){
        if(memory.source!=MemorySource::DirectWitness
           ||!memory.witnessed ||memory.sourceCharacter!=resident.id
           ||memory.minute<0 ||memory.minute>minute
           ||minute-memory.minute>60
           ||memory.tags.size()!=2
           ||memory.tags[0]!="predator_encounter"
           ||memory.tags[1]!=target) continue;
        return true;
    }
    return false;
}

inline void rememberResidentPredatorSighting(
    Character& resident,int minute,const ResidentPredatorThreat& threat)
{
    if(!resident.alive ||!threat.detected
       ||recentResidentPredatorSighting(resident,minute,threat.animalId))
        return;
    MemoryRecord memory;
    memory.who=resident.id;
    memory.sourceCharacter=resident.id;
    memory.source=MemorySource::DirectWitness;
    memory.witnessed=true;
    memory.minute=minute;
    memory.what=threat.species==WildlifeSpecies::Tiger
        ? "가까운 곳에서 야생 호랑이를 직접 목격했다"
        : "가까운 곳에서 야생 늑대를 직접 목격했다";
    memory.where="Core 야생동물 실제 위치";
    memory.importance=0.85;
    memory.confidence=1.0;
    memory.emotionValence=-0.75;
    memory.emotionIntensity=0.78;
    memory.tags={"predator_encounter",std::to_string(threat.animalId)};
    resident.memory.add(memory);
    applyEmotionEvent(resident.emotion,EmotionEventType::Threat);
}

// An encounter becomes an *injury* only after real adjacent physical
// contact, sufficient predator hunger and an independently reproducible Core
// risk roll. Seeing a distant tiger is NOT an attack. A predator cannot wound
// someone just because a camera or local model mentions it.
struct ResidentPredatorContact {
    bool found=false;
    WildlifeSpecies species=WildlifeSpecies::Tiger;
    std::uint64_t animalId=0;
    double hunger01=0.0;
    GridPos position{};
    std::int64_t distance=std::numeric_limits<std::int64_t>::max();
    double roll=1.0;
};

inline constexpr std::uint64_t ResidentPredatorAttackDomain=
    0x5245535041544b31ULL; // RESPATK1

inline ResidentPredatorContact observeResidentPredatorContact(
    const World& world,CharacterId actor,GridPos resident)
{
    ResidentPredatorContact contact;
    if(!world.wildlifeEnabled ||actor==0) return contact;
    for(const WildlifePopulation& population:world.wildlifePopulations){
        for(const WildlifeAnimal& animal:population.animals){
            if(!animal.alive
               ||(animal.species!=WildlifeSpecies::Tiger
                  &&animal.species!=WildlifeSpecies::Wolf)
               ||animal.hunger01<0.80) continue;
            const auto distance=residentPredatorDistance(
                resident,animal.position);
            if(distance>1) continue;
            if(contact.found &&(distance>contact.distance
                ||(distance==contact.distance
                   &&animal.id>=contact.animalId))) continue;
            contact.found=true;
            contact.species=animal.species;
            contact.animalId=animal.id;
            contact.hunger01=animal.hunger01;
            contact.position=animal.position;
            contact.distance=distance;
            // Stable once per in-world hour, independent of resident iteration
            // order and the general wildlife/health random streams.
            contact.roll=wildlifeRoll(population,animal.id,
                world.minute/60,ResidentPredatorAttackDomain^actor);
        }
    }
    return contact;
}

inline bool recentResidentPredatorContactInjury(
    const Character& resident,int minute,std::uint64_t animalId)
{
    const std::string target=std::to_string(animalId);
    for(const MemoryRecord& memory:resident.memory.entries){
        if(memory.source!=MemorySource::DirectWitness
           ||!memory.witnessed ||memory.sourceCharacter!=resident.id
           ||memory.minute>minute ||memory.minute<0
           ||minute-memory.minute>=60
           ||memory.tags.size()!=2
           ||memory.tags[0]!="predator_contact_injury"
           ||memory.tags[1]!=target) continue;
        return true;
    }
    return false;
}

inline bool applyResidentPredatorContactInjury(
    const World& world,Character& resident,GridPos position)
{
    if(!resident.alive ||resident.id==0
       ||world.minute<0 ||world.minute%60!=0) return false;
    const auto contact=observeResidentPredatorContact(
        world,resident.id,position);
    if(!contact.found
       ||recentResidentPredatorContactInjury(
           resident,world.minute,contact.animalId)) return false;
    // A starving tiger is more dangerous at arm's length than a wolf.
    // Outside genuine contact or at lower hunger, no injury is generated.
    const double chance=contact.species==WildlifeSpecies::Tiger
        ? 0.38+0.32*healthClamp01((contact.hunger01-0.80)/0.20)
        : 0.20+0.25*healthClamp01((contact.hunger01-0.80)/0.20);
    if(contact.roll>=chance) return false;

    const double severity=contact.species==WildlifeSpecies::Tiger
        ? 0.32 : 0.22;
    resident.health.injurySeverity=healthClamp01(
        resident.health.injurySeverity+severity);
    ++resident.health.accidentEpisodes;
    resident.health.lastAccidentMinute=world.minute;
    applyEmotionEvent(resident.emotion,EmotionEventType::Threat,1.35);

    MemoryRecord memory;
    memory.who=resident.id;
    memory.sourceCharacter=resident.id;
    memory.source=MemorySource::DirectWitness;
    memory.witnessed=true;
    memory.minute=world.minute;
    memory.what=contact.species==WildlifeSpecies::Tiger
        ? "야생 호랑이와 실제 근접 접촉 중 공격을 받아 부상했다"
        : "야생 늑대와 실제 근접 접촉 중 공격을 받아 부상했다";
    memory.where="Core 야생동물 실제 근접 접촉";
    memory.importance=1.0;
    memory.confidence=1.0;
    memory.emotionValence=-0.95;
    memory.emotionIntensity=1.0;
    memory.tags={
        "predator_contact_injury",std::to_string(contact.animalId)
    };
    resident.memory.add(memory);
    return true;
}

} // namespace lifelens
