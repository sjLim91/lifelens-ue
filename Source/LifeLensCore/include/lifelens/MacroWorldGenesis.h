#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "WorldGenesis.h"

namespace lifelens {

inline constexpr int MacroStartSearchRadiusChunks = 12;

enum class MacroBiome : std::uint8_t {
    TemperateForest = 0,
    Meadow,
    Plains,
    Hills,
    Wetland,
    DryScrub,
    ColdSteppe
};

struct MacroRegionFacts {
    ChunkCoord coord{};
    double elevation = 0.5;
    double moisture = 0.5;
    double temperature = 0.5;
    double waterPotential = 0.5;
    double fertilityPotential = 0.5;
    double woodPotential = 0.5;
    double stonePotential = 0.5;
    double foodPotential = 0.5;
    double traversalEase = 0.5;
    double hazardPotential = 0.5;
    MacroBiome biome = MacroBiome::Plains;
};

struct InitialStartRegionSelection {
    MacroRegionFacts region{};
    double viability = 0.0;
    int evaluatedCandidates = 0;
};

inline constexpr double MacroSeaLevel01 = 0.43;

enum class MacroSurfaceClass : std::uint8_t {
    Land = 0,
    Coast,
    Ocean
};

struct MacroSurfaceFacts {
    ChunkCoord coord{};
    MacroSurfaceClass surfaceClass = MacroSurfaceClass::Land;
    bool hasMarineNeighbour = false;
    ChunkCoord marineNeighbour{};
};

inline double clampMacro01(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

inline double macroUnitFromWord(std::uint64_t word)
{
    // Stable 53-bit conversion to [0,1), independent of C library RNGs.
    return static_cast<double>(word >> 11U) * (1.0 / 9007199254740992.0);
}

inline double macroSmoothStep(double value)
{
    const double t=clampMacro01(value);
    return t*t*(3.0-2.0*t);
}

inline double macroLerp(double a,double b,double t)
{
    return a+(b-a)*t;
}

inline double macroLatticeValue(
    const WorldGenesisIdentity& identity,
    int latticeX,
    int latticeY,
    std::uint64_t domain)
{
    std::uint64_t state=worldGenesisMix64(normalizeWorldSeed(identity.worldSeed)^domain);
    state=combineWorldGenesisWord(state,normalizeWorldGenerationVersion(identity.generationVersion));
    state=combineWorldGenesisWord(state,stableSignedCoordinateWord(latticeX));
    state=combineWorldGenesisWord(state,stableSignedCoordinateWord(latticeY));
    return macroUnitFromWord(state);
}

inline double macroValueNoise(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord,
    std::uint64_t domain,
    int latticeSpanChunks)
{
    const int span=std::max(1,latticeSpanChunks);
    const int cellX=floorDivWorldGrid(coord.x,span);
    const int cellY=floorDivWorldGrid(coord.y,span);
    const int originX=cellX*span;
    const int originY=cellY*span;
    const double fx=static_cast<double>(coord.x-originX)/static_cast<double>(span);
    const double fy=static_cast<double>(coord.y-originY)/static_cast<double>(span);
    const double sx=macroSmoothStep(fx);
    const double sy=macroSmoothStep(fy);

    const double v00=macroLatticeValue(identity,cellX,cellY,domain);
    const double v10=macroLatticeValue(identity,cellX+1,cellY,domain);
    const double v01=macroLatticeValue(identity,cellX,cellY+1,domain);
    const double v11=macroLatticeValue(identity,cellX+1,cellY+1,domain);
    return macroLerp(macroLerp(v00,v10,sx),macroLerp(v01,v11,sx),sy);
}

inline MacroBiome classifyMacroBiome(const MacroRegionFacts& facts)
{
    if(facts.elevation>0.72) return MacroBiome::Hills;
    if(facts.waterPotential>0.74 && facts.moisture>0.65) return MacroBiome::Wetland;
    if(facts.temperature<0.30) return MacroBiome::ColdSteppe;
    if(facts.moisture<0.30) return MacroBiome::DryScrub;
    if(facts.moisture>0.62 && facts.woodPotential>0.62) return MacroBiome::TemperateForest;
    if(facts.fertilityPotential>0.62) return MacroBiome::Meadow;
    return MacroBiome::Plains;
}

inline const char* macroBiomeName(MacroBiome biome)
{
    switch(biome){
        case MacroBiome::TemperateForest: return "TemperateForest";
        case MacroBiome::Meadow: return "Meadow";
        case MacroBiome::Plains: return "Plains";
        case MacroBiome::Hills: return "Hills";
        case MacroBiome::Wetland: return "Wetland";
        case MacroBiome::DryScrub: return "DryScrub";
        case MacroBiome::ColdSteppe: return "ColdSteppe";
    }
    return "Plains";
}

inline MacroRegionFacts deriveMacroRegionFacts(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord)
{
    // Broad + local coherent fields. PopulationSeed is intentionally absent.
    constexpr std::uint64_t ElevationBroad = 0x4d4143524f454c31ULL; // MACROEL1
    constexpr std::uint64_t ElevationLocal = 0x4d4143524f454c32ULL; // MACROEL2
    constexpr std::uint64_t MoistureBroad = 0x4d4143524f4d4f31ULL;  // MACROMO1
    constexpr std::uint64_t MoistureLocal = 0x4d4143524f4d4f32ULL;  // MACROMO2
    constexpr std::uint64_t TemperatureBroad = 0x4d4143524f544d31ULL; // MACROTM1
    constexpr std::uint64_t TemperatureLocal = 0x4d4143524f544d32ULL; // MACROTM2
    constexpr std::uint64_t WaterDomain = 0x4d4143524f574154ULL; // MACROWAT
    constexpr std::uint64_t ResourceDomain = 0x4d4143524f524553ULL; // MACRORES
    constexpr std::uint64_t HazardDomain = 0x4d4143524f48415aULL; // MACROHAZ

    MacroRegionFacts facts;
    facts.coord=coord;
    facts.elevation=clampMacro01(
        0.68*macroValueNoise(identity,coord,ElevationBroad,18)+
        0.32*macroValueNoise(identity,coord,ElevationLocal,6));
    facts.moisture=clampMacro01(
        0.72*macroValueNoise(identity,coord,MoistureBroad,16)+
        0.28*macroValueNoise(identity,coord,MoistureLocal,5));
    facts.temperature=clampMacro01(
        0.75*macroValueNoise(identity,coord,TemperatureBroad,20)+
        0.25*macroValueNoise(identity,coord,TemperatureLocal,7));

    const double drainage=macroValueNoise(identity,coord,WaterDomain,7);
    const double roughness=macroValueNoise(identity,coord,ResourceDomain,5);
    const double hazardNoise=macroValueNoise(identity,coord,HazardDomain,9);
    facts.waterPotential=clampMacro01(
        0.52*facts.moisture+0.23*(1.0-facts.elevation)+0.25*drainage);

    const double temperatureComfort=clampMacro01(1.0-std::abs(facts.temperature-0.56)*1.8);
    facts.fertilityPotential=clampMacro01(
        0.42*facts.moisture+0.23*facts.waterPotential+
        0.20*temperatureComfort+0.15*(1.0-std::abs(facts.elevation-0.42)));
    facts.woodPotential=clampMacro01(
        0.56*facts.moisture+0.18*facts.fertilityPotential+
        0.16*temperatureComfort+0.10*(1.0-facts.elevation));
    facts.stonePotential=clampMacro01(
        0.48*facts.elevation+0.34*roughness+0.18*(1.0-facts.moisture));
    facts.foodPotential=clampMacro01(
        0.48*facts.fertilityPotential+0.24*facts.moisture+
        0.18*temperatureComfort+0.10*facts.woodPotential);

    const double slopePenalty=clampMacro01(std::abs(facts.elevation-0.45)*1.65);
    facts.traversalEase=clampMacro01(
        0.82-0.48*slopePenalty-0.20*facts.waterPotential+0.18*(1.0-hazardNoise));
    const double thermalHazard=clampMacro01(std::abs(facts.temperature-0.55)*1.9);
    const double floodHazard=clampMacro01(std::max(0.0,facts.waterPotential-0.78)*2.7);
    const double terrainHazard=clampMacro01(std::max(0.0,facts.elevation-0.78)*2.5);
    facts.hazardPotential=clampMacro01(
        0.30*hazardNoise+0.28*thermalHazard+0.22*floodHazard+0.20*terrainHazard);
    facts.biome=classifyMacroBiome(facts);
    return facts;
}

inline MacroSurfaceFacts deriveMacroSurfaceFacts(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord)
{
    MacroSurfaceFacts surface;
    surface.coord = coord;

    // Generation v1 shipped as an all-land local-surface projection. Never
    // reinterpret an old v1 save through the v2 sea-level contract.
    if (identity.generationVersion < 2)
    {
        return surface;
    }

    const MacroRegionFacts center = deriveMacroRegionFacts(identity, coord);
    if (center.elevation < MacroSeaLevel01)
    {
        surface.surfaceClass = MacroSurfaceClass::Ocean;
        return surface;
    }

    const ChunkCoord neighbours[4] = {
        {coord.x + 1, coord.y},
        {coord.x - 1, coord.y},
        {coord.x, coord.y + 1},
        {coord.x, coord.y - 1}
    };

    bool foundMarine = false;
    double lowestMarineElevation = 2.0;
    ChunkCoord lowestMarine{};
    for (const ChunkCoord neighbour : neighbours)
    {
        const double elevation =
            deriveMacroRegionFacts(identity, neighbour).elevation;
        if (elevation >= MacroSeaLevel01)
        {
            continue;
        }
        if (!foundMarine
            || elevation < lowestMarineElevation
            || (elevation == lowestMarineElevation && neighbour < lowestMarine))
        {
            foundMarine = true;
            lowestMarineElevation = elevation;
            lowestMarine = neighbour;
        }
    }

    if (foundMarine)
    {
        surface.surfaceClass = MacroSurfaceClass::Coast;
        surface.hasMarineNeighbour = true;
        surface.marineNeighbour = lowestMarine;
    }
    return surface;
}

inline double scoreInitialStartRegion(const MacroRegionFacts& facts)
{
    // Water/food dominate early survival; material diversity and traversal make
    // experimentation possible. Extreme scarcity is heavily penalized, while a
    // small oversupply penalty avoids deliberately selecting a fully solved Eden.
    double score=
        0.25*facts.waterPotential+
        0.20*facts.foodPotential+
        0.13*facts.woodPotential+
        0.10*facts.stonePotential+
        0.12*facts.fertilityPotential+
        0.12*facts.traversalEase+
        0.08*(1.0-facts.hazardPotential);

    score-=0.30*std::max(0.0,0.35-facts.waterPotential);
    score-=0.26*std::max(0.0,0.30-facts.foodPotential);
    score-=0.12*std::max(0.0,0.22-facts.woodPotential);
    score-=0.08*std::max(0.0,0.18-facts.stonePotential);

    const double abundance=(
        facts.waterPotential+facts.foodPotential+facts.woodPotential+
        facts.stonePotential+facts.fertilityPotential)/5.0;
    score-=0.18*std::max(0.0,abundance-0.80);
    return clampMacro01(score);
}

inline InitialStartRegionSelection selectInitialStartRegion(
    const WorldGenesisIdentity& identity,
    int searchRadiusChunks=MacroStartSearchRadiusChunks)
{
    const int radius=std::max(1,searchRadiusChunks);
    InitialStartRegionSelection best;
    bool hasBest=false;

    for(int y=-radius;y<=radius;++y){
        for(int x=-radius;x<=radius;++x){
            const ChunkCoord coord{x,y};
            const MacroRegionFacts facts=deriveMacroRegionFacts(identity,coord);
            const MacroSurfaceFacts surface=deriveMacroSurfaceFacts(identity,coord);
            const double viability=surface.surfaceClass==MacroSurfaceClass::Land
                ? scoreInitialStartRegion(facts)
                : 0.0;
            ++best.evaluatedCandidates;
            if(surface.surfaceClass!=MacroSurfaceClass::Land) continue;
            if(!hasBest || viability>best.viability ||
               (viability==best.viability && facts.coord<best.region.coord)){
                best.region=facts;
                best.viability=viability;
                hasBest=true;
            }
        }
    }
    return best;
}

} // namespace lifelens
