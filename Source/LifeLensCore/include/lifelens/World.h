#pragma once
#include <cstdint>
#include <random>
#include <vector>
#include "Character.h"
#include "SmartObject.h"
#include "Civilization.h"
#include "Facility.h"
#include "EnvironmentalResidue.h"
#include "PrimitiveSanitation.h"
#include "WorldGenesis.h"
#include "MacroWorldGenesis.h"
#include "Hydrology.h"
#include "NaturalWorldChunk.h"
namespace lifelens {
struct World {
    int minute=7*60;
    // `seed` remains the compatibility spelling for authoritative WorldSeed.
    std::uint64_t seed=1;
    PopulationSeed populationSeed=1;
    WorldGenerationVersion generationVersion=CurrentWorldGenerationVersion;
    std::mt19937_64 rng{1};
    std::vector<Character> characters;
    std::vector<SmartObject> objects;
    std::vector<ResourceNode> resourceNodes;
    std::vector<StorageSite> storageSites;
    std::vector<ConstructedFacility> facilities;
    std::vector<PrimitiveSanitationSite> primitiveSanitationSites;
    EnvironmentalResidueField environmentalResidues;
    std::vector<GeneratedNaturalChunk> generatedNaturalChunks;
    bool hasInitialStartRegionSelection=false;
    ChunkCoord initialStartRegionCoord{};
    double initialStartRegionViability=0.0;

    // Runtime execution policy only. The binary snapshot codec deliberately
    // does not persist this flag; Unreal re-enables external execution after
    // starting/restoring Core while standalone Core tests remain autonomous.
    bool externalPhysicalExecution=false;

    explicit World(
        WorldSeed worldSeed=1,
        PopulationSeed initialPopulationSeed=0,
        WorldGenerationVersion initialGenerationVersion=CurrentWorldGenerationVersion)
    {
        const WorldGenesisIdentity identity=makeWorldGenesisIdentity(
            worldSeed,initialPopulationSeed,initialGenerationVersion);
        seed=identity.worldSeed;
        populationSeed=identity.populationSeed;
        generationVersion=identity.generationVersion;
        rng.seed(seed);
        resetCivilizationEnvironment();
    }

    WorldGenesisIdentity genesisIdentity() const
    {
        return {seed,populationSeed,generationVersion};
    }

    UntouchedChunkBaseline untouchedChunkBaseline(ChunkCoord coord) const
    {
        return deriveUntouchedChunkBaseline(genesisIdentity(),coord);
    }

    MacroRegionFacts macroRegionFacts(ChunkCoord coord) const
    {
        return deriveMacroRegionFacts(genesisIdentity(),coord);
    }

    InitialStartRegionSelection initialStartRegion() const
    {
        if(hasInitialStartRegionSelection){
            InitialStartRegionSelection stored;
            stored.region=macroRegionFacts(initialStartRegionCoord);
            stored.viability=initialStartRegionViability;
            stored.evaluatedCandidates=(MacroStartSearchRadiusChunks*2+1)*(MacroStartSearchRadiusChunks*2+1);
            return stored;
        }
        return generationVersion >= 2
            ? selectInitialFreshSurfaceWaterRegion(genesisIdentity())
            : selectInitialStartRegion(genesisIdentity());
    }

    InitialStartRegionSelection establishInitialStartRegion()
    {
        const InitialStartRegionSelection selected = generationVersion >= 2
            ? selectInitialFreshSurfaceWaterRegion(genesisIdentity())
            : selectInitialStartRegion(genesisIdentity());
        hasInitialStartRegionSelection=true;
        initialStartRegionCoord=selected.region.coord;
        initialStartRegionViability=selected.viability;
        return selected;
    }

    GridPos initialStartRegionCenterGrid() const
    {
        const InitialStartRegionSelection selected=initialStartRegion();
        const GridPos origin=chunkOriginGrid(selected.region.coord);
        return {origin.x+WorldChunkSpanGridCells/2,origin.y+WorldChunkSpanGridCells/2};
    }

    const GeneratedNaturalChunk* findGeneratedNaturalChunk(ChunkCoord coord) const
    {
        const auto it=std::lower_bound(
            generatedNaturalChunks.begin(),generatedNaturalChunks.end(),coord,
            [](const GeneratedNaturalChunk& chunk,ChunkCoord value){return chunk.coord<value;});
        return it!=generatedNaturalChunks.end() && it->coord==coord ? &*it : nullptr;
    }

    GeneratedNaturalChunk* findGeneratedNaturalChunk(ChunkCoord coord)
    {
        const auto it=std::lower_bound(
            generatedNaturalChunks.begin(),generatedNaturalChunks.end(),coord,
            [](const GeneratedNaturalChunk& chunk,ChunkCoord value){return chunk.coord<value;});
        return it!=generatedNaturalChunks.end() && it->coord==coord ? &*it : nullptr;
    }

    GeneratedNaturalChunk& materializeNaturalChunk(ChunkCoord coord)
    {
        auto it=std::lower_bound(
            generatedNaturalChunks.begin(),generatedNaturalChunks.end(),coord,
            [](const GeneratedNaturalChunk& chunk,ChunkCoord value){return chunk.coord<value;});
        if(it!=generatedNaturalChunks.end() && it->coord==coord) return *it;

        GeneratedNaturalChunk generated=deriveGeneratedNaturalChunk(genesisIdentity(),coord,minute);
        for(const auto& patch:generated.resourcePatches){
            const auto nodeIt=std::find_if(resourceNodes.begin(),resourceNodes.end(),[&](const ResourceNode& node){
                return node.id==patch.nodeId;
            });
            if(nodeIt==resourceNodes.end()){
                resourceNodes.push_back({
                    patch.nodeId,patch.material,patch.baselineQuantity,patch.maxQuantity,
                    patch.renewable,patch.regenerationPerDay,patch.pos});
            }
        }
        std::sort(resourceNodes.begin(),resourceNodes.end(),[](const ResourceNode& a,const ResourceNode& b){
            return a.id<b.id;
        });
        it=generatedNaturalChunks.insert(it,std::move(generated));
        return *it;
    }

    void clearGeneratedNaturalWorld()
    {
        generatedNaturalChunks.clear();
        hasInitialStartRegionSelection=false;
        initialStartRegionCoord={};
        initialStartRegionViability=0.0;
    }

    void resetCivilizationEnvironment()
    {
        // Natural starting environment. These are material opportunities, not
        // pre-unlocked techniques. Characters still begin with no recipe/tech
        // knowledge and must discover reproducible methods themselves. The
        // fixed positions belong only to this compatibility/test environment;
        // production NEW GAME replaces these nodes with generated patch positions.
        resourceNodes={
            {1,MaterialKind::Stone,160,160,false,0,{-4,-2}},
            {2,MaterialKind::Flint,90,90,false,0,{-2,-4}},
            {3,MaterialKind::Wood,140,180,true,8,{4,-3}},
            {4,MaterialKind::Fiber,100,140,true,7,{5,1}},
            {5,MaterialKind::Clay,120,120,false,0,{-5,3}},
            {6,MaterialKind::Water,240,300,true,30,{0,6}},
            {7,MaterialKind::PlantFood,80,120,true,10,{3,5}}
        };
        storageSites={{1,Inventory{},{0,0}}};
        // Compatibility worlds retain their legacy storage fixture, but the new
        // constructed-facility authority starts empty. Production NEW GAME also
        // clears storageSites, so no facility/storage is granted for free.
        facilities.clear();
        primitiveSanitationSites.clear();
        environmentalResidues.clear();
    }
};
}
