#include "lifelens/WebClientBridge.h"
#include "lifelens/Simulation.h"
#include "lifelens/World.h"
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

// The fixture is authoritative Core data, not a staged animal gallery.
// v4 intentionally leaves most predator territories empty, so a fixed
// WorldSeed=77 cannot guarantee that Wolf is present. Find the first real
// production world whose materialized habitats contain all four species
// required by the Web renderer comparison (including an actual male deer).
static bool hasReviewSpecies(const lifelens::World& world)
{
    bool hare=false, deerMale=false, wolf=false, fish=false;
    for(const auto& population:world.wildlifePopulations){
        for(const auto& animal:population.animals){
            if(!animal.alive) continue;
            switch(animal.species){
                case lifelens::WildlifeSpecies::Hare: hare=true; break;
                case lifelens::WildlifeSpecies::Deer:
                    deerMale=deerMale || animal.sex==lifelens::WildlifeSex::Male;
                    break;
                case lifelens::WildlifeSpecies::Wolf: wolf=true; break;
                case lifelens::WildlifeSpecies::FreshwaterFish: fish=true; break;
                default: break;
            }
        }
    }
    return hare && deerMale && wolf && fish;
}

int main(int argc,char** argv)
{
    if(argc!=2) return 2;
    std::uint64_t worldSeed=0;
    lifelens::ChunkCoord center{};
    for(std::uint64_t candidate=77;candidate<333;++candidate){
        lifelens::Simulation production(candidate,0,4);
        production.setupNewGame();
        if(!hasReviewSpecies(production.world())) continue;
        worldSeed=candidate;
        center=production.world().initialStartRegionCoord;
        break;
    }
    if(worldSeed==0){
        std::cerr << "No authoritative v4 ecology fixture with hare, male deer, wolf and fish\n";
        return 6;
    }
    lifelens::WebClientBridge bridge;
    const std::string seedText=std::to_string(worldSeed);
    if(!bridge.newGame(seedText,"",4)) return 3;
    bridge.runMinutes(2);
    const auto start=std::chrono::steady_clock::now();
    const auto terrain=bridge.terrainWindowJson(center.x,center.y,2);
    const auto first=std::chrono::steady_clock::now();
    const auto cached=bridge.terrainWindowJson(center.x,center.y,2);
    const auto end=std::chrono::steady_clock::now();
    if(terrain!=cached) return 4;
    std::ofstream out(argv[1]);
    out << "{\"terrain\":" << terrain << ",\"resources\":" << bridge.civilizationWorldWindowJson(0,center.x,center.y,2)
        << ",\"environment\":" << bridge.dynamicEnvironmentJson(center.x,center.y) << "}";
    if(!out) return 5;
    std::cout << "ECOLOGY_FIXTURE {\"worldSeed\":" << worldSeed
        << ",\"firstTerrainMs\":" << std::chrono::duration<double,std::milli>(first-start).count()
        << ",\"cachedTerrainMs\":" << std::chrono::duration<double,std::milli>(end-first).count()
        << ",\"terrainJsonBytes\":" << terrain.size() << "}\n";
}
