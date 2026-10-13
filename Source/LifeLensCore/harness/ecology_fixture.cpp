#include "lifelens/WebClientBridge.h"
#include "lifelens/World.h"
#include <chrono>
#include <fstream>
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2) return 2;
    lifelens::World world{77,0,4};
    const auto center=world.initialStartRegion().region.coord;
    lifelens::WebClientBridge bridge;
    if(!bridge.newGame("77","",4)) return 3;
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
    std::cout << "ECOLOGY_FIXTURE {\"firstTerrainMs\":" << std::chrono::duration<double,std::milli>(first-start).count()
        << ",\"cachedTerrainMs\":" << std::chrono::duration<double,std::milli>(end-first).count()
        << ",\"terrainJsonBytes\":" << terrain.size() << "}\n";
}
