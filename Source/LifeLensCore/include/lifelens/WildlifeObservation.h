#pragma once
#include "World.h"
#include <sstream>
#include <string>
namespace lifelens {
inline const char* wildlifeSpeciesName(WildlifeSpecies species){
    switch(species){
        case WildlifeSpecies::Hare: return "Hare";
        case WildlifeSpecies::Deer: return "Deer";
        case WildlifeSpecies::Wolf: return "Wolf";
        case WildlifeSpecies::FreshwaterFish: return "FreshwaterFish";
    }
    return "Unknown";
}
// Read-only observation of materialized Core populations. Camera movement does
// not create populations, advance ecology or consume the resident RNG.
inline void appendWildlifeObservation(std::ostringstream& out,const World& world,ChunkCoord center,int radius){
    out << "{\"available\":" << (world.wildlifeEnabled ? "true" : "false")
        << ",\"geographicallyConditioned\":false,\"regionalWeatherApplied\":false,\"animals\":[";
    bool first=true;
    for(const auto& p:world.wildlifePopulations){
        if(std::abs(static_cast<std::int64_t>(p.habitat.coord.x)-center.x)>radius
           || std::abs(static_cast<std::int64_t>(p.habitat.coord.y)-center.y)>radius) continue;
        for(const auto& animal:p.animals){
            if(!animal.alive) continue;
            if(!first) out << ',';
            first=false;
            // Prefix the ID with its chunk: standalone IDs are population-local.
            out << "{\"id\":\"" << p.habitat.coord.x << ':' << p.habitat.coord.y << ':' << animal.id
                << "\",\"species\":\"" << wildlifeSpeciesName(animal.species)
                << "\",\"sex\":\"" << (animal.sex==WildlifeSex::Male ? "Male" : "Female")
                << "\",\"gridX\":" << animal.position.x << ",\"gridY\":" << animal.position.y
                << ",\"ageDays\":" << animal.ageDays << '}';
        }
    }
    out << "]}";
}
}
