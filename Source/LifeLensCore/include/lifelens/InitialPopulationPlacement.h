#pragma once

#include <array>
#include <limits>
#include <vector>
#include "CoreNavigation.h"

namespace lifelens {

inline int initialPlacementDistanceSquared(GridPos a,GridPos b)
{
    const int x=a.x-b.x,y=a.y-b.y;
    return x*x+y*y;
}

// New Game only. These are initial conditions, never a persistent settlement,
// exclusion zone, camera offset, or relocation of residents loaded from saves.
inline std::array<GridPos,4> initialFounderPositions(const World& world)
{
    const GridPos center=world.initialStartRegionCenterGrid();
    const int spread=world.generationVersion>=4 ? 6 : 1;
    const std::array<GridPos,4> offsets={GridPos{-spread,-spread},
        GridPos{spread,-spread},GridPos{-spread,spread},GridPos{spread,spread}};
    std::array<GridPos,4> result{};
    for(std::size_t i=0;i<result.size();++i){
        const GridPos preferred{center.x+offsets[i].x,center.y+offsets[i].y};
        result[i]=preferred;
        if(world.generationVersion<4) continue;
        int best=std::numeric_limits<int>::max();
        for(int y=-WorldChunkSpanGridCells;y<=WorldChunkSpanGridCells;++y){
            for(int x=-WorldChunkSpanGridCells;x<=WorldChunkSpanGridCells;++x){
                const GridPos candidate{preferred.x+x,preferred.y+y};
                const int cost=x*x+y*y;
                if(cost>=best) continue;
                bool clear=true;
                for(std::size_t j=0;j<i;++j)
                    if(initialPlacementDistanceSquared(candidate,result[j])<64) clear=false;
                if(clear && coreGroundTraversable(world,candidate)){
                    result[i]=candidate;best=cost;
                }
            }
        }
    }
    return result;
}

inline void separateInitialLandWildlife(
    World& world,const std::array<GridPos,4>& founders)
{
    if(world.generationVersion<4 || !world.wildlifeEnabled) return;
    for(auto& population:world.wildlifePopulations){
        const GridPos origin=chunkOriginGrid(population.habitat.coord);
        const auto identity=world.genesisIdentity();
        if(deriveMacroSurfaceFacts(identity,population.habitat.coord).surfaceClass
            ==MacroSurfaceClass::Ocean) continue;
        const auto hydrology=deriveHydrologyFacts(identity,population.habitat.coord);
        std::vector<GridPos> dryCells,placed;
        for(int y=0;y<WorldChunkSpanGridCells;++y){
            for(int x=0;x<WorldChunkSpanGridCells;++x){
                const GridPos cell{origin.x+x,origin.y+y};
                if(!surfaceWaterGroundContainsGrid(hydrology,cell)) dryCells.push_back(cell);
            }
        }
        for(auto& animal:population.animals){
            if(!animal.alive || animal.species==WildlifeSpecies::FreshwaterFish) continue;
            const bool predator=animal.species==WildlifeSpecies::Wolf
                ||animal.species==WildlifeSpecies::Tiger;
            const int clearance=predator ? 12 : 8;
            int best=std::numeric_limits<int>::max();
            GridPos position=animal.position;
            for(const GridPos candidate:dryCells){
                const int cost=initialPlacementDistanceSquared(candidate,animal.position);
                if(cost>=best) continue;
                bool clear=true;
                for(const GridPos founder:founders)
                    if(initialPlacementDistanceSquared(candidate,founder)<clearance*clearance) clear=false;
                for(const GridPos other:placed)
                    if(initialPlacementDistanceSquared(candidate,other)<16) clear=false;
                if(clear){position=candidate;best=cost;}
            }
            // A constrained habitat may not fit every animal with clearance.
            // Keep its authoritative position rather than deleting population
            // members or inventing an unverified neighbouring habitat.
            animal.position=position;
            placed.push_back(position);
        }
    }
}

} // namespace lifelens
