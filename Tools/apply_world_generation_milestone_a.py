#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def write(path: str, content: str):
    p = ROOT / path
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(content, encoding='utf-8')


def replace_once(path: str, old: str, new: str):
    p = ROOT / path
    text = p.read_text(encoding='utf-8')
    if old not in text:
        raise RuntimeError(f'missing replacement anchor in {path}: {old[:120]!r}')
    p.write_text(text.replace(old, new, 1), encoding='utf-8')


write('Source/LifeLensCore/include/lifelens/NaturalWorldChunk.h', r'''#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "Civilization.h"
#include "MacroWorldGenesis.h"

namespace lifelens {

enum class NaturalSurfaceKind : std::uint8_t {
    Meadow = 0,
    ForestFloor,
    Plains,
    RockyGround,
    WetGround,
    DryGround,
    ColdGround
};

inline const char* naturalSurfaceKindName(NaturalSurfaceKind kind)
{
    switch(kind){
        case NaturalSurfaceKind::Meadow: return "Meadow";
        case NaturalSurfaceKind::ForestFloor: return "ForestFloor";
        case NaturalSurfaceKind::Plains: return "Plains";
        case NaturalSurfaceKind::RockyGround: return "RockyGround";
        case NaturalSurfaceKind::WetGround: return "WetGround";
        case NaturalSurfaceKind::DryGround: return "DryGround";
        case NaturalSurfaceKind::ColdGround: return "ColdGround";
    }
    return "Plains";
}

inline bool validNaturalSurfaceKind(NaturalSurfaceKind kind)
{
    switch(kind){
        case NaturalSurfaceKind::Meadow:
        case NaturalSurfaceKind::ForestFloor:
        case NaturalSurfaceKind::Plains:
        case NaturalSurfaceKind::RockyGround:
        case NaturalSurfaceKind::WetGround:
        case NaturalSurfaceKind::DryGround:
        case NaturalSurfaceKind::ColdGround:
            return true;
    }
    return false;
}

struct NaturalResourcePatch {
    ResourceNodeId nodeId = 0;
    MaterialKind material = MaterialKind::Unknown;
    GridPos pos{};
    int baselineQuantity = 0;
    int maxQuantity = 0;
    bool renewable = false;
    int regenerationPerDay = 0;
    double visualDensity = 0.0;
    std::uint64_t detailSeed = 0;
};

struct GeneratedNaturalChunk {
    ChunkCoord coord{};
    std::uint64_t chunkSeed = 0;
    WorldGenerationVersion generationVersion = CurrentWorldGenerationVersion;
    MacroBiome biome = MacroBiome::Plains;
    NaturalSurfaceKind surface = NaturalSurfaceKind::Plains;
    double elevation = 0.5;
    double moisture = 0.5;
    double temperature = 0.5;
    double waterPotential = 0.5;
    double fertilityPotential = 0.5;
    double traversalEase = 0.5;
    double hazardPotential = 0.5;
    int materializedMinute = 0;
    std::vector<NaturalResourcePatch> resourcePatches;
};

inline NaturalSurfaceKind naturalSurfaceForBiome(MacroBiome biome)
{
    switch(biome){
        case MacroBiome::TemperateForest: return NaturalSurfaceKind::ForestFloor;
        case MacroBiome::Meadow: return NaturalSurfaceKind::Meadow;
        case MacroBiome::Plains: return NaturalSurfaceKind::Plains;
        case MacroBiome::Hills: return NaturalSurfaceKind::RockyGround;
        case MacroBiome::Wetland: return NaturalSurfaceKind::WetGround;
        case MacroBiome::DryScrub: return NaturalSurfaceKind::DryGround;
        case MacroBiome::ColdSteppe: return NaturalSurfaceKind::ColdGround;
    }
    return NaturalSurfaceKind::Plains;
}

inline std::uint64_t deriveNaturalResourceNodeId(
    std::uint64_t chunkSeed,
    MaterialKind material,
    int ordinal)
{
    constexpr std::uint64_t ResourceNodeDomain = 0x4e41545245534e31ULL; // NATRESN1
    std::uint64_t state = combineWorldGenesisWord(chunkSeed, ResourceNodeDomain);
    state = combineWorldGenesisWord(state, static_cast<std::uint64_t>(static_cast<int>(material)));
    state = combineWorldGenesisWord(state, static_cast<std::uint64_t>(std::max(0, ordinal)));
    // Keep generated ids positive when projected through Unreal int64 while
    // staying far away from the small compatibility ids used by legacy tests.
    std::uint64_t id = worldGenesisMix64(state) & 0x3fffffffffffffffULL;
    id |= 0x4000000000000000ULL;
    return id;
}

inline double naturalPotentialForMaterial(const MacroRegionFacts& facts, MaterialKind material)
{
    switch(material){
        case MaterialKind::Water: return facts.waterPotential;
        case MaterialKind::Wood: return facts.woodPotential;
        case MaterialKind::Stone: return facts.stonePotential;
        case MaterialKind::Flint: return clampMacro01(facts.stonePotential * 0.82 + facts.hazardPotential * 0.08);
        case MaterialKind::Fiber: return clampMacro01(facts.fertilityPotential * 0.58 + facts.moisture * 0.32);
        case MaterialKind::Clay: return clampMacro01(facts.waterPotential * 0.48 + facts.fertilityPotential * 0.32 + (1.0 - facts.elevation) * 0.20);
        case MaterialKind::PlantFood: return facts.foodPotential;
        default: return 0.0;
    }
}

inline int naturalPatchCount(double potential, int maxPatches)
{
    if(potential <= 0.12 || maxPatches <= 0) return 0;
    return std::max(1, std::min(maxPatches, 1 + static_cast<int>(std::floor(potential * static_cast<double>(maxPatches - 1)))));
}

inline int naturalResourceBaseMinimum(MaterialKind material)
{
    switch(material){
        case MaterialKind::Water: return 120;
        case MaterialKind::Wood: return 65;
        case MaterialKind::Stone: return 50;
        case MaterialKind::Flint: return 22;
        case MaterialKind::Fiber: return 45;
        case MaterialKind::Clay: return 48;
        case MaterialKind::PlantFood: return 38;
        default: return 20;
    }
}

inline int naturalResourceBaseMaximum(MaterialKind material)
{
    switch(material){
        case MaterialKind::Water: return 320;
        case MaterialKind::Wood: return 190;
        case MaterialKind::Stone: return 150;
        case MaterialKind::Flint: return 82;
        case MaterialKind::Fiber: return 135;
        case MaterialKind::Clay: return 145;
        case MaterialKind::PlantFood: return 120;
        default: return 80;
    }
}

inline bool naturalResourceRenewable(MaterialKind material)
{
    return material == MaterialKind::Water
        || material == MaterialKind::Wood
        || material == MaterialKind::Fiber
        || material == MaterialKind::PlantFood;
}

inline int naturalResourceRegenerationPerDay(MaterialKind material)
{
    switch(material){
        case MaterialKind::Water: return 28;
        case MaterialKind::Wood: return 4;
        case MaterialKind::Fiber: return 7;
        case MaterialKind::PlantFood: return 9;
        default: return 0;
    }
}

inline GeneratedNaturalChunk deriveGeneratedNaturalChunk(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord,
    int materializedMinute)
{
    const UntouchedChunkBaseline untouched = deriveUntouchedChunkBaseline(identity, coord);
    const MacroRegionFacts macro = deriveMacroRegionFacts(identity, coord);

    GeneratedNaturalChunk chunk;
    chunk.coord = coord;
    chunk.chunkSeed = untouched.chunkSeed;
    chunk.generationVersion = identity.generationVersion;
    chunk.biome = macro.biome;
    chunk.surface = naturalSurfaceForBiome(macro.biome);
    chunk.elevation = macro.elevation;
    chunk.moisture = macro.moisture;
    chunk.temperature = macro.temperature;
    chunk.waterPotential = macro.waterPotential;
    chunk.fertilityPotential = macro.fertilityPotential;
    chunk.traversalEase = macro.traversalEase;
    chunk.hazardPotential = macro.hazardPotential;
    chunk.materializedMinute = std::max(0, materializedMinute);

    const std::array<MaterialKind, 7> materials = {
        MaterialKind::Water,
        MaterialKind::Wood,
        MaterialKind::Stone,
        MaterialKind::Flint,
        MaterialKind::Fiber,
        MaterialKind::Clay,
        MaterialKind::PlantFood
    };

    const GridPos origin = chunkOriginGrid(coord);
    for(MaterialKind material : materials){
        const double potential = naturalPotentialForMaterial(macro, material);
        const int maxPatches = material == MaterialKind::Water ? 3 : 4;
        const int patchCount = naturalPatchCount(potential, maxPatches);
        for(int ordinal = 0; ordinal < patchCount; ++ordinal){
            const ResourceNodeId nodeId = deriveNaturalResourceNodeId(untouched.chunkSeed, material, ordinal);
            std::uint64_t detailSeed = combineWorldGenesisWord(
                untouched.detailStreamSeed,
                static_cast<std::uint64_t>(static_cast<int>(material) * 17 + ordinal));
            const double amountNoise = macroUnitFromWord(worldGenesisMix64(detailSeed ^ 0x414d4f554e543031ULL));
            const double densityNoise = macroUnitFromWord(worldGenesisMix64(detailSeed ^ 0x44454e5349545931ULL));
            const int localSpan = std::max(1, WorldChunkSpanGridCells - 4);
            const int localX = 2 + static_cast<int>(worldGenesisMix64(detailSeed ^ 0x504f535f585f3031ULL) % static_cast<std::uint64_t>(localSpan));
            const int localY = 2 + static_cast<int>(worldGenesisMix64(detailSeed ^ 0x504f535f595f3031ULL) % static_cast<std::uint64_t>(localSpan));

            const int minimum = naturalResourceBaseMinimum(material);
            const int maximum = naturalResourceBaseMaximum(material);
            const double abundance = clampMacro01(potential * 0.72 + amountNoise * 0.28);
            const int quantity = minimum + static_cast<int>(std::round(
                static_cast<double>(maximum - minimum) * abundance));

            NaturalResourcePatch patch;
            patch.nodeId = nodeId;
            patch.material = material;
            patch.pos = {origin.x + localX, origin.y + localY};
            patch.baselineQuantity = quantity;
            patch.maxQuantity = quantity;
            patch.renewable = naturalResourceRenewable(material);
            patch.regenerationPerDay = naturalResourceRegenerationPerDay(material);
            patch.visualDensity = clampMacro01(potential * 0.78 + densityNoise * 0.22);
            patch.detailSeed = detailSeed;
            chunk.resourcePatches.push_back(patch);
        }
    }

    std::sort(chunk.resourcePatches.begin(), chunk.resourcePatches.end(), [](const NaturalResourcePatch& a, const NaturalResourcePatch& b){
        return a.nodeId < b.nodeId;
    });
    return chunk;
}

inline bool validNaturalResourcePatch(const NaturalResourcePatch& patch, ChunkCoord ownerCoord)
{
    return patch.nodeId != 0
        && validMaterialKind(patch.material)
        && patch.material != MaterialKind::Unknown
        && chunkCoordForGrid(patch.pos) == ownerCoord
        && patch.baselineQuantity > 0
        && patch.maxQuantity == patch.baselineQuantity
        && patch.regenerationPerDay >= 0
        && patch.visualDensity >= 0.0
        && patch.visualDensity <= 1.0;
}

inline bool validGeneratedNaturalChunk(const GeneratedNaturalChunk& chunk)
{
    if(chunk.chunkSeed == 0
       || chunk.generationVersion == 0
       || !validNaturalSurfaceKind(chunk.surface)
       || chunk.elevation < 0.0 || chunk.elevation > 1.0
       || chunk.moisture < 0.0 || chunk.moisture > 1.0
       || chunk.temperature < 0.0 || chunk.temperature > 1.0
       || chunk.waterPotential < 0.0 || chunk.waterPotential > 1.0
       || chunk.fertilityPotential < 0.0 || chunk.fertilityPotential > 1.0
       || chunk.traversalEase < 0.0 || chunk.traversalEase > 1.0
       || chunk.hazardPotential < 0.0 || chunk.hazardPotential > 1.0
       || chunk.materializedMinute < 0) return false;

    ResourceNodeId previous = 0;
    for(const auto& patch : chunk.resourcePatches){
        if(!validNaturalResourcePatch(patch, chunk.coord)) return false;
        if(previous != 0 && patch.nodeId <= previous) return false;
        previous = patch.nodeId;
    }
    return true;
}

inline bool sameGeneratedNaturalChunkBaseline(
    const GeneratedNaturalChunk& a,
    const GeneratedNaturalChunk& b)
{
    if(a.coord != b.coord
       || a.chunkSeed != b.chunkSeed
       || a.generationVersion != b.generationVersion
       || a.biome != b.biome
       || a.surface != b.surface
       || a.elevation != b.elevation
       || a.moisture != b.moisture
       || a.temperature != b.temperature
       || a.waterPotential != b.waterPotential
       || a.fertilityPotential != b.fertilityPotential
       || a.traversalEase != b.traversalEase
       || a.hazardPotential != b.hazardPotential
       || a.materializedMinute != b.materializedMinute
       || a.resourcePatches.size() != b.resourcePatches.size()) return false;

    for(std::size_t i = 0; i < a.resourcePatches.size(); ++i){
        const auto& x = a.resourcePatches[i];
        const auto& y = b.resourcePatches[i];
        if(x.nodeId != y.nodeId
           || x.material != y.material
           || x.pos.x != y.pos.x || x.pos.y != y.pos.y
           || x.baselineQuantity != y.baselineQuantity
           || x.maxQuantity != y.maxQuantity
           || x.renewable != y.renewable
           || x.regenerationPerDay != y.regenerationPerDay
           || x.visualDensity != y.visualDensity
           || x.detailSeed != y.detailSeed) return false;
    }
    return true;
}

} // namespace lifelens
''')

write('Source/LifeLensCore/include/lifelens/WorldGenerationSnapshotCodec.h', r'''#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <set>
#include <utility>
#include <vector>

#include "World.h"

namespace lifelens {

constexpr char WorldGenerationSnapshotExtensionMagic[]={'L','L','W','G','E','N','0','1'};
constexpr std::uint32_t WorldGenerationSnapshotExtensionVersion=1;

inline const ResourceNode* findWorldGenerationResourceNode(const World& world, ResourceNodeId id)
{
    for(const auto& node : world.resourceNodes) if(node.id == id) return &node;
    return nullptr;
}

inline bool validateWorldGenerationSnapshotState(const World& world)
{
    if(world.populationSeed == 0 || world.generationVersion == 0) return false;
    if(world.hasInitialStartRegionSelection
       && (world.initialStartRegionViability < 0.0 || world.initialStartRegionViability > 1.0)) return false;

    std::set<ChunkCoord> coords;
    bool foundInitial = !world.hasInitialStartRegionSelection;
    for(const auto& chunk : world.generatedNaturalChunks){
        if(!coords.insert(chunk.coord).second || !validGeneratedNaturalChunk(chunk)) return false;
        if(chunk.generationVersion != world.generationVersion) return false;
        const GeneratedNaturalChunk expected = deriveGeneratedNaturalChunk(
            world.genesisIdentity(), chunk.coord, chunk.materializedMinute);
        if(!sameGeneratedNaturalChunkBaseline(chunk, expected)) return false;
        if(world.hasInitialStartRegionSelection && chunk.coord == world.initialStartRegionCoord) foundInitial = true;

        for(const auto& patch : chunk.resourcePatches){
            const ResourceNode* node = findWorldGenerationResourceNode(world, patch.nodeId);
            if(node == nullptr
               || node->material != patch.material
               || node->maxQuantity != patch.maxQuantity
               || node->quantity < 0 || node->quantity > node->maxQuantity
               || node->renewable != patch.renewable
               || node->regenerationPerDay != patch.regenerationPerDay) return false;
        }
    }
    return foundInitial;
}

template<typename WriterT>
void writeNaturalResourcePatch(WriterT& w, const NaturalResourcePatch& patch)
{
    w.u64(patch.nodeId);
    w.enumeration(patch.material);
    w.i32(patch.pos.x);
    w.i32(patch.pos.y);
    w.i32(patch.baselineQuantity);
    w.i32(patch.maxQuantity);
    w.boolean(patch.renewable);
    w.i32(patch.regenerationPerDay);
    w.real(patch.visualDensity);
    w.u64(patch.detailSeed);
}

template<typename ReaderT>
bool readNaturalResourcePatch(ReaderT& r, NaturalResourcePatch& patch)
{
    return r.u64(patch.nodeId)
        && r.enumeration(patch.material)
        && r.i32(patch.pos.x)
        && r.i32(patch.pos.y)
        && r.i32(patch.baselineQuantity)
        && r.i32(patch.maxQuantity)
        && r.boolean(patch.renewable)
        && r.i32(patch.regenerationPerDay)
        && r.real(patch.visualDensity)
        && r.u64(patch.detailSeed);
}

template<typename WriterT>
void writeGeneratedNaturalChunk(WriterT& w, const GeneratedNaturalChunk& chunk)
{
    w.i32(chunk.coord.x);
    w.i32(chunk.coord.y);
    w.u64(chunk.chunkSeed);
    w.u32(chunk.generationVersion);
    w.enumeration(chunk.biome);
    w.enumeration(chunk.surface);
    w.real(chunk.elevation);
    w.real(chunk.moisture);
    w.real(chunk.temperature);
    w.real(chunk.waterPotential);
    w.real(chunk.fertilityPotential);
    w.real(chunk.traversalEase);
    w.real(chunk.hazardPotential);
    w.i32(chunk.materializedMinute);
    w.u32(static_cast<std::uint32_t>(chunk.resourcePatches.size()));
    for(const auto& patch : chunk.resourcePatches) writeNaturalResourcePatch(w, patch);
}

template<typename ReaderT>
bool readGeneratedNaturalChunk(ReaderT& r, GeneratedNaturalChunk& chunk)
{
    if(!r.i32(chunk.coord.x)
       || !r.i32(chunk.coord.y)
       || !r.u64(chunk.chunkSeed)
       || !r.u32(chunk.generationVersion)
       || !r.enumeration(chunk.biome)
       || !r.enumeration(chunk.surface)
       || !r.real(chunk.elevation)
       || !r.real(chunk.moisture)
       || !r.real(chunk.temperature)
       || !r.real(chunk.waterPotential)
       || !r.real(chunk.fertilityPotential)
       || !r.real(chunk.traversalEase)
       || !r.real(chunk.hazardPotential)
       || !r.i32(chunk.materializedMinute)) return false;

    std::uint32_t count = 0;
    if(!r.count(count)) return false;
    chunk.resourcePatches.clear();
    chunk.resourcePatches.reserve(count);
    for(std::uint32_t i = 0; i < count; ++i){
        NaturalResourcePatch patch;
        if(!readNaturalResourcePatch(r, patch)) return false;
        chunk.resourcePatches.push_back(std::move(patch));
    }
    return validGeneratedNaturalChunk(chunk);
}

template<typename WriterT>
void writeWorldGenerationSnapshotExtension(WriterT& w, const World& world)
{
    w.raw(WorldGenerationSnapshotExtensionMagic, sizeof(WorldGenerationSnapshotExtensionMagic));
    w.u32(WorldGenerationSnapshotExtensionVersion);
    w.u64(world.populationSeed);
    w.u32(world.generationVersion);
    w.boolean(world.hasInitialStartRegionSelection);
    if(world.hasInitialStartRegionSelection){
        w.i32(world.initialStartRegionCoord.x);
        w.i32(world.initialStartRegionCoord.y);
        w.real(world.initialStartRegionViability);
    }
    w.u32(static_cast<std::uint32_t>(world.generatedNaturalChunks.size()));
    for(const auto& chunk : world.generatedNaturalChunks) writeGeneratedNaturalChunk(w, chunk);
}

template<typename ReaderT>
bool readWorldGenerationSnapshotExtension(ReaderT& r, World& world)
{
    char magic[sizeof(WorldGenerationSnapshotExtensionMagic)]{};
    std::uint32_t version = 0;
    if(!r.raw(magic, sizeof(magic))
       || std::memcmp(magic, WorldGenerationSnapshotExtensionMagic, sizeof(magic)) != 0
       || !r.u32(version)
       || version != WorldGenerationSnapshotExtensionVersion) return false;

    PopulationSeed populationSeed = 0;
    WorldGenerationVersion generationVersion = 0;
    bool hasStart = false;
    ChunkCoord startCoord{};
    double startViability = 0.0;
    if(!r.u64(populationSeed)
       || populationSeed == 0
       || !r.u32(generationVersion)
       || generationVersion == 0
       || !r.boolean(hasStart)) return false;
    if(hasStart){
        if(!r.i32(startCoord.x) || !r.i32(startCoord.y) || !r.real(startViability)) return false;
        if(startViability < 0.0 || startViability > 1.0) return false;
    }

    std::uint32_t count = 0;
    if(!r.count(count)) return false;
    std::vector<GeneratedNaturalChunk> chunks;
    chunks.reserve(count);
    for(std::uint32_t i = 0; i < count; ++i){
        GeneratedNaturalChunk chunk;
        if(!readGeneratedNaturalChunk(r, chunk)) return false;
        chunks.push_back(std::move(chunk));
    }
    std::sort(chunks.begin(), chunks.end(), [](const GeneratedNaturalChunk& a, const GeneratedNaturalChunk& b){
        return a.coord < b.coord;
    });

    world.populationSeed = populationSeed;
    world.generationVersion = generationVersion;
    world.hasInitialStartRegionSelection = hasStart;
    world.initialStartRegionCoord = startCoord;
    world.initialStartRegionViability = hasStart ? startViability : 0.0;
    world.generatedNaturalChunks = std::move(chunks);
    return validateWorldGenerationSnapshotState(world);
}

} // namespace lifelens
''')

# World.h: add detailed natural chunk state and materialization helpers.
replace_once(
    'Source/LifeLensCore/include/lifelens/World.h',
    '#include "MacroWorldGenesis.h"\n',
    '#include "MacroWorldGenesis.h"\n#include "NaturalWorldChunk.h"\n')
replace_once(
    'Source/LifeLensCore/include/lifelens/World.h',
    '    EnvironmentalResidueField environmentalResidues;\n',
    '    EnvironmentalResidueField environmentalResidues;\n    std::vector<GeneratedNaturalChunk> generatedNaturalChunks;\n    bool hasInitialStartRegionSelection=false;\n    ChunkCoord initialStartRegionCoord{};\n    double initialStartRegionViability=0.0;\n')
replace_once(
    'Source/LifeLensCore/include/lifelens/World.h',
    '''    InitialStartRegionSelection initialStartRegion() const
    {
        return selectInitialStartRegion(genesisIdentity());
    }

    void resetCivilizationEnvironment()
''',
    '''    InitialStartRegionSelection initialStartRegion() const
    {
        if(hasInitialStartRegionSelection){
            InitialStartRegionSelection stored;
            stored.region=macroRegionFacts(initialStartRegionCoord);
            stored.viability=initialStartRegionViability;
            stored.evaluatedCandidates=(MacroStartSearchRadiusChunks*2+1)*(MacroStartSearchRadiusChunks*2+1);
            return stored;
        }
        return selectInitialStartRegion(genesisIdentity());
    }

    InitialStartRegionSelection establishInitialStartRegion()
    {
        const InitialStartRegionSelection selected=selectInitialStartRegion(genesisIdentity());
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
                    patch.renewable,patch.regenerationPerDay});
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
''')

# Production NEW GAME now materializes selected start chunk, removes legacy free storage,
# and places founders in authoritative Core coordinates inside that chunk.
replace_once(
    'Source/LifeLensCore/src/Simulation.cpp',
    '''    world_.minute=8*60;
    world_.resetCivilizationEnvironment();

    // World randomness and initial-population randomness are separate.
''',
    '''    world_.minute=8*60;
    world_.resetCivilizationEnvironment();
    // Production NEW GAME begins with nature only. The compatibility World
    // constructor still seeds a utility-test storage/resource baseline, but the
    // production path replaces it with the selected natural chunk and no storage.
    world_.resourceNodes.clear();
    world_.storageSites.clear();
    world_.clearGeneratedNaturalWorld();
    const InitialStartRegionSelection startRegion=world_.establishInitialStartRegion();
    world_.materializeNaturalChunk(startRegion.region.coord);

    // World randomness and initial-population randomness are separate.
''')
replace_once(
    'Source/LifeLensCore/src/Simulation.cpp',
    '''    std::uniform_real_distribution<double> familiarity(0.0,0.04);
    for(const Character& from:world_.characters){
        runtime_[from.id]=Runtime{};
        for(const Character& to:world_.characters){
''',
    '''    const GridPos startCenter=world_.initialStartRegionCenterGrid();
    const std::array<GridPos,4> founderOffsets={GridPos{-1,-1},GridPos{1,-1},GridPos{-1,1},GridPos{1,1}};
    std::uniform_real_distribution<double> familiarity(0.0,0.04);
    std::size_t founderIndex=0;
    for(const Character& from:world_.characters){
        Runtime initialRuntime;
        const GridPos offset=founderOffsets[std::min(founderIndex,founderOffsets.size()-1)];
        initialRuntime.pos={startCenter.x+offset.x,startCenter.y+offset.y};
        runtime_[from.id]=initialRuntime;
        ++founderIndex;
        for(const Character& to:world_.characters){
''')
replace_once(
    'Source/LifeLensCore/src/Simulation.cpp',
    '    emit("new game start seed="+std::to_string(world_.seed)+" populationSeed="+std::to_string(world_.populationSeed)+" founders=4");\n',
    '    emit("new game start seed="+std::to_string(world_.seed)+" populationSeed="+std::to_string(world_.populationSeed)+" founders=4 startChunk=("+std::to_string(startRegion.region.coord.x)+","+std::to_string(startRegion.region.coord.y)+")");\n')

# Snapshot format v6 persists WG identity, selected region and generated chunk registry.
replace_once(
    'Source/LifeLensCore/include/lifelens/SimulationSnapshotCodec.h',
    'constexpr std::uint32_t SimulationSnapshotBinaryFormatVersion=5;',
    'constexpr std::uint32_t SimulationSnapshotBinaryFormatVersion=6;')
replace_once(
    'Source/LifeLensCore/src/SimulationSnapshotCodec.cpp',
    '#include "lifelens/PrimitiveSanitationSnapshotCodec.h"\n',
    '#include "lifelens/PrimitiveSanitationSnapshotCodec.h"\n#include "lifelens/WorldGenerationSnapshotCodec.h"\n')
replace_once(
    'Source/LifeLensCore/src/SimulationSnapshotCodec.cpp',
    '    if(!validatePrimitiveSanitationSitesForCodec(snapshot.world,error)) return false;\n',
    '    if(!validatePrimitiveSanitationSitesForCodec(snapshot.world,error)) return false;\n    if(!validateWorldGenerationSnapshotState(snapshot.world)){ setError(error,"invalid world generation state"); return false; }\n')
replace_once(
    'Source/LifeLensCore/src/SimulationSnapshotCodec.cpp',
    '''    Writer sanitationExtension;
    writePrimitiveSanitationSnapshotExtension(
        sanitationExtension,snapshot.world.primitiveSanitationSites);
    body.insert(body.end(),sanitationExtension.bytes.begin(),sanitationExtension.bytes.end());

    outBytes=std::move(body);
''',
    '''    Writer sanitationExtension;
    writePrimitiveSanitationSnapshotExtension(
        sanitationExtension,snapshot.world.primitiveSanitationSites);
    body.insert(body.end(),sanitationExtension.bytes.begin(),sanitationExtension.bytes.end());

    Writer worldGenerationExtension;
    writeWorldGenerationSnapshotExtension(worldGenerationExtension,snapshot.world);
    body.insert(body.end(),worldGenerationExtension.bytes.begin(),worldGenerationExtension.bytes.end());

    outBytes=std::move(body);
''')
replace_once(
    'Source/LifeLensCore/src/SimulationSnapshotCodec.cpp',
    '''    auto sanitationMarker=bytes.end();
    if(binaryVersion>=5){
        sanitationMarker=std::find_end(
            environmentMarker,bytes.end(),
            PrimitiveSanitationSnapshotExtensionMagic,
            PrimitiveSanitationSnapshotExtensionMagic+sizeof(PrimitiveSanitationSnapshotExtensionMagic));
        if(sanitationMarker==bytes.end() || sanitationMarker<=environmentMarker){
            setError(error,"missing primitive sanitation snapshot extension");
            return false;
        }
    }

    const auto civilizationEnd=binaryVersion>=3 ? socialMarker : bytes.end();
    const auto socialEnd=binaryVersion>=4 ? environmentMarker : bytes.end();
    const auto environmentEnd=binaryVersion>=5 ? sanitationMarker : bytes.end();
''',
    '''    auto sanitationMarker=bytes.end();
    if(binaryVersion>=5){
        sanitationMarker=std::find_end(
            environmentMarker,bytes.end(),
            PrimitiveSanitationSnapshotExtensionMagic,
            PrimitiveSanitationSnapshotExtensionMagic+sizeof(PrimitiveSanitationSnapshotExtensionMagic));
        if(sanitationMarker==bytes.end() || sanitationMarker<=environmentMarker){
            setError(error,"missing primitive sanitation snapshot extension");
            return false;
        }
    }

    auto worldGenerationMarker=bytes.end();
    if(binaryVersion>=6){
        worldGenerationMarker=std::find_end(
            sanitationMarker,bytes.end(),
            WorldGenerationSnapshotExtensionMagic,
            WorldGenerationSnapshotExtensionMagic+sizeof(WorldGenerationSnapshotExtensionMagic));
        if(worldGenerationMarker==bytes.end() || worldGenerationMarker<=sanitationMarker){
            setError(error,"missing world generation snapshot extension");
            return false;
        }
    }

    const auto civilizationEnd=binaryVersion>=3 ? socialMarker : bytes.end();
    const auto socialEnd=binaryVersion>=4 ? environmentMarker : bytes.end();
    const auto environmentEnd=binaryVersion>=5 ? sanitationMarker : bytes.end();
    const auto sanitationEnd=binaryVersion>=6 ? worldGenerationMarker : bytes.end();
''')
replace_once(
    'Source/LifeLensCore/src/SimulationSnapshotCodec.cpp',
    '        std::vector<std::uint8_t> sanitationBytes(sanitationMarker,bytes.end());\n',
    '        std::vector<std::uint8_t> sanitationBytes(sanitationMarker,sanitationEnd);\n')
replace_once(
    'Source/LifeLensCore/src/SimulationSnapshotCodec.cpp',
    '''    }else{
        decoded.world.primitiveSanitationSites.clear();
    }

    outSnapshot=std::move(decoded);
''',
    '''    }else{
        decoded.world.primitiveSanitationSites.clear();
    }

    if(binaryVersion>=6){
        std::vector<std::uint8_t> worldGenerationBytes(worldGenerationMarker,bytes.end());
        Reader worldGenerationReader(worldGenerationBytes);
        if(!readWorldGenerationSnapshotExtension(worldGenerationReader,decoded.world)
           || !worldGenerationReader.done()){
            setError(error,"invalid world generation snapshot extension");
            return false;
        }
    }else{
        // Legacy snapshots predate materialized chunks. Preserve their existing
        // resource/history state and do not silently generate a new landscape.
        decoded.world.generatedNaturalChunks.clear();
        decoded.world.hasInitialStartRegionSelection=false;
        decoded.world.initialStartRegionCoord={};
        decoded.world.initialStartRegionViability=0.0;
    }

    outSnapshot=std::move(decoded);
''')

replace_once(
    'Source/LifeLensCore/src/SimulationSnapshot.cpp',
    '#include "lifelens/CivilizationSnapshotCodec.h"\n',
    '#include "lifelens/CivilizationSnapshotCodec.h"\n#include "lifelens/WorldGenerationSnapshotCodec.h"\n')
replace_once(
    'Source/LifeLensCore/src/SimulationSnapshot.cpp',
    '''    for(const auto& storage:snapshot.world.storageSites){
        if(storage.id==0) return fail("snapshot contains zero storage id");
        if(!storageIds.insert(storage.id).second) return fail("snapshot contains duplicate storage id");
        if(!validateInventoryState(storage.inventory)) return fail("snapshot contains invalid storage inventory");
    }

    EnvironmentalResidueField validatedResidues;
''',
    '''    for(const auto& storage:snapshot.world.storageSites){
        if(storage.id==0) return fail("snapshot contains zero storage id");
        if(!storageIds.insert(storage.id).second) return fail("snapshot contains duplicate storage id");
        if(!validateInventoryState(storage.inventory)) return fail("snapshot contains invalid storage inventory");
    }
    if(!validateWorldGenerationSnapshotState(snapshot.world))
        return fail("snapshot contains invalid world generation state");

    EnvironmentalResidueField validatedResidues;
''')

# Unreal read DTOs and bridge.
write('Source/LifeLens/Simulation/LLWorldGenerationReadTypes.h', r'''#pragma once

#include "CoreMinimal.h"
#include "LLWorldGenerationReadTypes.generated.h"

USTRUCT(BlueprintType)
struct FLLCoreNaturalResourcePatchObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int64 ResourceNodeId = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") FName Material;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 GridX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 GridY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 CurrentQuantity = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 MaxQuantity = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") bool bRenewable = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 RegenerationPerDay = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float VisualDensity = 0.0f;
};

USTRUCT(BlueprintType)
struct FLLCoreNaturalChunkObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") bool bMaterialized = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 ChunkX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 ChunkY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") FName Biome;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") FName Surface;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float Elevation = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float Moisture = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float Temperature = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float WaterPotential = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float FertilityPotential = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float TraversalEase = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float HazardPotential = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 MaterializedMinute = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") TArray<FLLCoreNaturalResourcePatchObservation> ResourcePatches;
};

USTRUCT(BlueprintType)
struct FLLCoreWorldGenerationObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") bool bAvailable = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int64 WorldSeed = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 GenerationVersion = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") bool bHasInitialStartRegion = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 InitialChunkX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 InitialChunkY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 InitialCenterGridX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 InitialCenterGridY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float InitialViability = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 MaterializedChunkCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") FLLCoreNaturalChunkObservation InitialChunk;
};
''')

replace_once(
    'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h',
    '#include "Simulation/LLEnvironmentReadTypes.h"\n',
    '#include "Simulation/LLEnvironmentReadTypes.h"\n#include "Simulation/LLWorldGenerationReadTypes.h"\n')
replace_once(
    'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h',
    '''    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Environment")
    FLLCoreEnvironmentObservation GetEnvironmentObservation(int32 MaxResidues = 64) const;

    // Native persistence bridge.
''',
    '''    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Environment")
    FLLCoreEnvironmentObservation GetEnvironmentObservation(int32 MaxResidues = 64) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|WorldGeneration")
    FLLCoreWorldGenerationObservation GetWorldGenerationObservation() const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|WorldGeneration")
    bool GetNaturalChunkObservation(
        int32 ChunkX,
        int32 ChunkY,
        FLLCoreNaturalChunkObservation& OutObservation) const;

    // Native persistence bridge.
''')

write('Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp', r'''#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/Civilization.h"
#include "lifelens/NaturalWorldChunk.h"
#include "lifelens/Simulation.h"

namespace
{
const lifelens::ResourceNode* FindResourceNode(
    const lifelens::World& World,
    lifelens::ResourceNodeId Id)
{
    for (const lifelens::ResourceNode& Node : World.resourceNodes)
    {
        if (Node.id == Id)
        {
            return &Node;
        }
    }
    return nullptr;
}

void FillNaturalChunkObservation(
    const lifelens::World& World,
    const lifelens::GeneratedNaturalChunk& Chunk,
    FLLCoreNaturalChunkObservation& Out)
{
    Out = FLLCoreNaturalChunkObservation{};
    Out.bMaterialized = true;
    Out.ChunkX = Chunk.coord.x;
    Out.ChunkY = Chunk.coord.y;
    Out.Biome = FName(UTF8_TO_TCHAR(lifelens::macroBiomeName(Chunk.biome)));
    Out.Surface = FName(UTF8_TO_TCHAR(lifelens::naturalSurfaceKindName(Chunk.surface)));
    Out.Elevation = static_cast<float>(Chunk.elevation);
    Out.Moisture = static_cast<float>(Chunk.moisture);
    Out.Temperature = static_cast<float>(Chunk.temperature);
    Out.WaterPotential = static_cast<float>(Chunk.waterPotential);
    Out.FertilityPotential = static_cast<float>(Chunk.fertilityPotential);
    Out.TraversalEase = static_cast<float>(Chunk.traversalEase);
    Out.HazardPotential = static_cast<float>(Chunk.hazardPotential);
    Out.MaterializedMinute = Chunk.materializedMinute;
    Out.ResourcePatches.Reserve(static_cast<int32>(Chunk.resourcePatches.size()));

    for (const lifelens::NaturalResourcePatch& Patch : Chunk.resourcePatches)
    {
        FLLCoreNaturalResourcePatchObservation Item;
        Item.ResourceNodeId = static_cast<int64>(Patch.nodeId);
        Item.Material = FName(UTF8_TO_TCHAR(lifelens::materialName(Patch.material)));
        Item.GridX = Patch.pos.x;
        Item.GridY = Patch.pos.y;
        Item.MaxQuantity = Patch.maxQuantity;
        Item.bRenewable = Patch.renewable;
        Item.RegenerationPerDay = Patch.regenerationPerDay;
        Item.VisualDensity = static_cast<float>(Patch.visualDensity);
        if (const lifelens::ResourceNode* Node = FindResourceNode(World, Patch.nodeId))
        {
            Item.CurrentQuantity = Node->quantity;
        }
        Out.ResourcePatches.Add(Item);
    }
}
}

FLLCoreWorldGenerationObservation ULLCoreBridgeSubsystem::GetWorldGenerationObservation() const
{
    FLLCoreWorldGenerationObservation Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const lifelens::World& World = CoreSimulation->world();
    Result.bAvailable = true;
    Result.WorldSeed = static_cast<int64>(World.seed);
    Result.GenerationVersion = static_cast<int32>(World.generationVersion);
    Result.MaterializedChunkCount = static_cast<int32>(FMath::Min<std::size_t>(
        World.generatedNaturalChunks.size(), static_cast<std::size_t>(MAX_int32)));
    Result.bHasInitialStartRegion = World.hasInitialStartRegionSelection;

    if (!World.hasInitialStartRegionSelection)
    {
        return Result;
    }

    Result.InitialChunkX = World.initialStartRegionCoord.x;
    Result.InitialChunkY = World.initialStartRegionCoord.y;
    Result.InitialViability = static_cast<float>(World.initialStartRegionViability);
    const lifelens::GridPos Center = World.initialStartRegionCenterGrid();
    Result.InitialCenterGridX = Center.x;
    Result.InitialCenterGridY = Center.y;

    if (const lifelens::GeneratedNaturalChunk* Chunk = World.findGeneratedNaturalChunk(World.initialStartRegionCoord))
    {
        FillNaturalChunkObservation(World, *Chunk, Result.InitialChunk);
    }
    return Result;
}

bool ULLCoreBridgeSubsystem::GetNaturalChunkObservation(
    int32 ChunkX,
    int32 ChunkY,
    FLLCoreNaturalChunkObservation& OutObservation) const
{
    OutObservation = FLLCoreNaturalChunkObservation{};
    if (!CoreSimulation)
    {
        return false;
    }

    const lifelens::World& World = CoreSimulation->world();
    const lifelens::ChunkCoord Coord{ChunkX, ChunkY};
    const lifelens::GeneratedNaturalChunk* Chunk = World.findGeneratedNaturalChunk(Coord);
    if (!Chunk)
    {
        return false;
    }

    FillNaturalChunkObservation(World, *Chunk, OutObservation);
    return true;
}
''')

# Presentation origin keeps global Core chunk coordinates authoritative while
# mapping the selected start chunk center onto the current Unreal bootstrap origin.
replace_once(
    'Source/LifeLens/World/LLWorldDirector.h',
    '    void CollectActivityAnchors();\n',
    '    void RefreshCorePresentationOrigin();\n    void CollectActivityAnchors();\n')
replace_once(
    'Source/LifeLens/World/LLWorldDirector.h',
    '    float EnvironmentalVisualRefreshAccumulator = 0.0f;\n',
    '    float EnvironmentalVisualRefreshAccumulator = 0.0f;\n    FIntPoint CorePresentationOriginGrid = FIntPoint::ZeroValue;\n')
replace_once(
    'Source/LifeLens/World/LLWorldDirector.cpp',
    '''    CoreBridge->SetExternalPhysicalExecutionEnabled(true);
    CollectActivityAnchors();
    SpawnResidents();
    if (EnvironmentalResidueVisualizer)
    {
        EnvironmentalResidueVisualizer->RefreshFromCore(*CoreBridge, CoreGridCellSizeUU, true);
    }
''',
    '''    CoreBridge->SetExternalPhysicalExecutionEnabled(true);
    RefreshCorePresentationOrigin();
    CollectActivityAnchors();
    SpawnResidents();
    if (EnvironmentalResidueVisualizer)
    {
        EnvironmentalResidueVisualizer->RefreshFromCore(
            *CoreBridge, CoreGridCellSizeUU, true,
            CorePresentationOriginGrid.X, CorePresentationOriginGrid.Y);
    }
''')
replace_once(
    'Source/LifeLens/World/LLWorldDirector.cpp',
    '''        EnvironmentalResidueVisualizer->RefreshFromCore(*CoreBridge, CoreGridCellSizeUU, false);
''',
    '''        EnvironmentalResidueVisualizer->RefreshFromCore(
            *CoreBridge, CoreGridCellSizeUU, false,
            CorePresentationOriginGrid.X, CorePresentationOriginGrid.Y);
''')
replace_once(
    'Source/LifeLens/World/LLWorldDirector.cpp',
    'void ALLWorldDirector::CollectActivityAnchors()\n',
    '''void ALLWorldDirector::RefreshCorePresentationOrigin()
{
    CorePresentationOriginGrid = FIntPoint::ZeroValue;
    if (!CoreBridge)
    {
        return;
    }

    const FLLCoreWorldGenerationObservation Genesis = CoreBridge->GetWorldGenerationObservation();
    if (Genesis.bAvailable && Genesis.bHasInitialStartRegion)
    {
        CorePresentationOriginGrid = FIntPoint(
            Genesis.InitialCenterGridX,
            Genesis.InitialCenterGridY);
    }
}

void ALLWorldDirector::CollectActivityAnchors()
''')
replace_once(
    'Source/LifeLens/World/LLWorldDirector.cpp',
    '''            FVector RecommendedLocation = GetActorLocation()
                + FVector(
                    static_cast<float>(RecommendedGridX) * CellSize,
                    static_cast<float>(RecommendedGridY) * CellSize,
                    0.0f);
''',
    '''            FVector RecommendedLocation = GetActorLocation()
                + FVector(
                    static_cast<float>(RecommendedGridX - CorePresentationOriginGrid.X) * CellSize,
                    static_cast<float>(RecommendedGridY - CorePresentationOriginGrid.Y) * CellSize,
                    0.0f);
''')
replace_once(
    'Source/LifeLens/World/LLWorldDirector.cpp',
    '''    return FIntPoint(
        FMath::RoundToInt(Relative.X / CellSize),
        FMath::RoundToInt(Relative.Y / CellSize));
''',
    '''    return FIntPoint(
        CorePresentationOriginGrid.X + FMath::RoundToInt(Relative.X / CellSize),
        CorePresentationOriginGrid.Y + FMath::RoundToInt(Relative.Y / CellSize));
''')
replace_once(
    'Source/LifeLens/World/LLWorldDirector.cpp',
    '''            static_cast<float>(GridX) * CellSize,
            static_cast<float>(GridY) * CellSize,
''',
    '''            static_cast<float>(GridX - CorePresentationOriginGrid.X) * CellSize,
            static_cast<float>(GridY - CorePresentationOriginGrid.Y) * CellSize,
''')

replace_once(
    'Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.h',
    '''    void RefreshFromCore(
        const ULLCoreBridgeSubsystem& CoreBridge,
        float CoreGridCellSizeUU,
        bool bForce = false);
''',
    '''    void RefreshFromCore(
        const ULLCoreBridgeSubsystem& CoreBridge,
        float CoreGridCellSizeUU,
        bool bForce = false,
        int32 CoreOriginGridX = 0,
        int32 CoreOriginGridY = 0);
''')
replace_once(
    'Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.h',
    '    FVector ResolveSurfaceLocation(int32 GridX, int32 GridY, float CoreGridCellSizeUU) const;\n',
    '    FVector ResolveSurfaceLocation(int32 GridX, int32 GridY, float CoreGridCellSizeUU, int32 CoreOriginGridX, int32 CoreOriginGridY) const;\n')
replace_once(
    'Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.cpp',
    '''    int32 GridX,
    int32 GridY,
    float CoreGridCellSizeUU) const
''',
    '''    int32 GridX,
    int32 GridY,
    float CoreGridCellSizeUU,
    int32 CoreOriginGridX,
    int32 CoreOriginGridY) const
''')
replace_once(
    'Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.cpp',
    '''    Location.X += static_cast<float>(GridX) * CellSize;
    Location.Y += static_cast<float>(GridY) * CellSize;
''',
    '''    Location.X += static_cast<float>(GridX - CoreOriginGridX) * CellSize;
    Location.Y += static_cast<float>(GridY - CoreOriginGridY) * CellSize;
''')
replace_once(
    'Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.cpp',
    '''void ULLEnvironmentalResidueVisualizerComponent::RefreshFromCore(
    const ULLCoreBridgeSubsystem& CoreBridge,
    float CoreGridCellSizeUU,
    bool bForce)
''',
    '''void ULLEnvironmentalResidueVisualizerComponent::RefreshFromCore(
    const ULLCoreBridgeSubsystem& CoreBridge,
    float CoreGridCellSizeUU,
    bool bForce,
    int32 CoreOriginGridX,
    int32 CoreOriginGridY)
''')
replace_once(
    'Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.cpp',
    '''        const FVector WorldLocation = ResolveSurfaceLocation(
            Residue.GridX, Residue.GridY, CellSize);
''',
    '''        const FVector WorldLocation = ResolveSurfaceLocation(
            Residue.GridX, Residue.GridY, CellSize,
            CoreOriginGridX, CoreOriginGridY);
''')

# New milestone test.
write('Source/LifeLensCore/tests/test_world_generation_milestone_a.cpp', r'''#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

#include <cassert>
#include <string>
#include <vector>

using namespace lifelens;

namespace {

bool sameResourceNodes(const World& a, const World& b)
{
    if(a.resourceNodes.size()!=b.resourceNodes.size()) return false;
    for(std::size_t i=0;i<a.resourceNodes.size();++i){
        const auto& x=a.resourceNodes[i];
        const auto& y=b.resourceNodes[i];
        if(x.id!=y.id || x.material!=y.material || x.quantity!=y.quantity
           || x.maxQuantity!=y.maxQuantity || x.renewable!=y.renewable
           || x.regenerationPerDay!=y.regenerationPerDay) return false;
    }
    return true;
}

} // namespace

int main()
{
    const WorldSeed seed=874213954ULL;
    const PopulationSeed peopleA=111ULL;
    const PopulationSeed peopleB=222ULL;

    // Natural detail is independent of population randomness.
    const WorldGenesisIdentity identityA=makeWorldGenesisIdentity(seed,peopleA,1);
    const WorldGenesisIdentity identityB=makeWorldGenesisIdentity(seed,peopleB,1);
    const std::vector<ChunkCoord> probes={{0,0},{-1,2},{6,-7},{12,12}};
    for(const ChunkCoord coord:probes){
        const GeneratedNaturalChunk a=deriveGeneratedNaturalChunk(identityA,coord,480);
        const GeneratedNaturalChunk b=deriveGeneratedNaturalChunk(identityB,coord,480);
        assert(sameGeneratedNaturalChunkBaseline(a,b));
        assert(validGeneratedNaturalChunk(a));
    }

    Simulation simulation(seed,peopleA,1);
    simulation.setupNewGame();
    World& world=simulation.world();
    const InitialStartRegionSelection selected=selectInitialStartRegion(identityA);

    assert(world.hasInitialStartRegionSelection);
    assert(world.initialStartRegionCoord==selected.region.coord);
    assert(world.initialStartRegionViability==selected.viability);
    assert(world.generatedNaturalChunks.size()==1);
    assert(world.storageSites.empty());
    assert(world.objects.empty());
    assert(world.primitiveSanitationSites.empty());

    const GeneratedNaturalChunk* startChunk=world.findGeneratedNaturalChunk(selected.region.coord);
    assert(startChunk!=nullptr);
    assert(!startChunk->resourcePatches.empty());
    assert(world.resourceNodes.size()==startChunk->resourcePatches.size());
    assert(validateWorldGenerationSnapshotState(world));

    // Founders now occupy authoritative positions inside the selected global
    // start chunk instead of all defaulting to {0,0}.
    for(const Character& founder:world.characters){
        GridPos pos{};
        assert(simulation.runtimePosition(founder.id,pos));
        assert(chunkCoordForGrid(pos)==selected.region.coord);
    }

    // Materialization order cannot change untouched detail or global resource ordering.
    World orderA(seed,peopleA,1);
    World orderB(seed,peopleB,1);
    orderA.resourceNodes.clear(); orderA.generatedNaturalChunks.clear();
    orderB.resourceNodes.clear(); orderB.generatedNaturalChunks.clear();
    orderA.materializeNaturalChunk({3,-2});
    orderA.materializeNaturalChunk({-4,5});
    orderB.materializeNaturalChunk({-4,5});
    orderB.materializeNaturalChunk({3,-2});
    assert(sameGeneratedNaturalChunkBaseline(
        *orderA.findGeneratedNaturalChunk({3,-2}),
        *orderB.findGeneratedNaturalChunk({3,-2})));
    assert(sameGeneratedNaturalChunkBaseline(
        *orderA.findGeneratedNaturalChunk({-4,5}),
        *orderB.findGeneratedNaturalChunk({-4,5})));
    assert(sameResourceNodes(orderA,orderB));

    // Depletion is history, not generator input. Save/load must restore the exact
    // generated registry and current resource quantity without rerolling patches.
    assert(!world.resourceNodes.empty());
    const ResourceNodeId depletedId=world.resourceNodes.front().id;
    world.resourceNodes.front().quantity=std::max(0,world.resourceNodes.front().quantity-7);
    const int depletedQuantity=world.resourceNodes.front().quantity;
    const GeneratedNaturalChunk baselineBefore=*startChunk;

    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error));
    assert(error.empty());
    assert(bytes.size()>64);

    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(error.empty());
    assert(decoded.world.populationSeed==peopleA);
    assert(decoded.world.generationVersion==1);
    assert(decoded.world.hasInitialStartRegionSelection);
    assert(decoded.world.initialStartRegionCoord==selected.region.coord);
    const GeneratedNaturalChunk* decodedChunk=decoded.world.findGeneratedNaturalChunk(selected.region.coord);
    assert(decodedChunk!=nullptr);
    assert(sameGeneratedNaturalChunkBaseline(baselineBefore,*decodedChunk));
    const ResourceNode* decodedResource=findWorldGenerationResourceNode(decoded.world,depletedId);
    assert(decodedResource!=nullptr);
    assert(decodedResource->quantity==depletedQuantity);

    Simulation restored(1);
    assert(restored.restoreSnapshot(decoded,&error));
    GridPos restoredPos{};
    assert(restored.runtimePosition(restored.world().characters.front().id,restoredPos));
    assert(chunkCoordForGrid(restoredPos)==selected.region.coord);

    std::vector<std::uint8_t> reencoded;
    assert(encodeSimulationSnapshot(restored.captureSnapshot(),reencoded,&error));
    assert(bytes==reencoded);

    // Same world, different people: terrain/resources stay exact while founder
    // randomization remains free to differ.
    Simulation otherPeople(seed,peopleB,1);
    otherPeople.setupNewGame();
    const GeneratedNaturalChunk* otherChunk=otherPeople.world().findGeneratedNaturalChunk(selected.region.coord);
    assert(otherChunk!=nullptr);
    assert(sameGeneratedNaturalChunkBaseline(baselineBefore,*otherChunk));

    return 0;
}
''')

replace_once(
    'Source/LifeLensCore/CMakeLists.txt',
    'lifelens_add_test(test_world_genesis_wg2)\n',
    'lifelens_add_test(test_world_genesis_wg2)\n\nlifelens_add_test(test_world_generation_milestone_a)\n')

# Update WG-2 test to assert the newly materialized start boundary rather than the old wait state.
replace_once(
    'Source/LifeLensCore/tests/test_world_genesis_wg2.cpp',
    '''    // Production NEW GAME still starts with nature and founders only. WG-2 does
    // not manufacture a house/toilet/farm/road/tool or make Unreal presentation
    // objects authoritative. Physical relocation waits for detailed chunk/world
    // materialization so the current bootstrap test map remains valid.
''',
    '''    // Production NEW GAME still starts with nature and founders only. Milestone A
    // materializes the WG-2-selected natural chunk and places founder Core positions
    // inside it without manufacturing civilization infrastructure.
''')
replace_once(
    'Source/LifeLensCore/tests/test_world_genesis_wg2.cpp',
    '''    assert(simulation.world().primitiveSanitationSites.empty());
    assert(simulation.world().initialStartRegion().region.coord==startA.region.coord);

    return 0;
''',
    '''    assert(simulation.world().primitiveSanitationSites.empty());
    assert(simulation.world().storageSites.empty());
    assert(simulation.world().initialStartRegion().region.coord==startA.region.coord);
    assert(simulation.world().generatedNaturalChunks.size()==1);
    for(const Character& founder:simulation.world().characters){
        GridPos pos{};
        assert(simulation.runtimePosition(founder.id,pos));
        assert(chunkCoordForGrid(pos)==startA.region.coord);
    }

    return 0;
''')

# Production New Game regression: replace hard-coded legacy resource/storage reset expectations.
replace_once(
    'Source/LifeLensCore/tests/test_autonomous_civilization_loop.cpp',
    '''    // NEW GAME is a true reset: depleted resources and stored items cannot leak.
    first.world().resourceNodes[1].quantity=1;
    first.world().storageSites[0].inventory.add({ItemKind::RawMaterial,MaterialKind::Stone,99,0.5,1.0});
    first.setupNewGame();
    CHECK(first.world().resourceNodes.size()==7);
    CHECK(first.world().resourceNodes[1].material==MaterialKind::Flint);
    CHECK(first.world().resourceNodes[1].quantity==90);
    CHECK(first.world().storageSites.size()==1);
    CHECK(first.world().storageSites[0].inventory.stacks().empty());
''',
    '''    // NEW GAME is a true reset: depleted generated resources and ad-hoc storage
    // cannot leak. Production now starts with no civilization storage facility.
    CHECK(!first.world().resourceNodes.empty());
    first.world().resourceNodes.front().quantity=1;
    StorageSite leakedStorage;
    leakedStorage.id=9999;
    leakedStorage.inventory.add({ItemKind::RawMaterial,MaterialKind::Stone,99,0.5,1.0});
    first.world().storageSites.push_back(leakedStorage);
    first.setupNewGame();
    CHECK(!first.world().resourceNodes.empty());
    for(const auto& node:first.world().resourceNodes) CHECK(node.quantity==node.maxQuantity);
    CHECK(first.world().storageSites.empty());
    CHECK(first.world().generatedNaturalChunks.size()==1);
''')

# Structural validator for the whole milestone.
write('Tools/validate_world_generation_milestone_a.py', r'''#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

def read(path):
    return (root / path).read_text(encoding='utf-8')

natural = read('Source/LifeLensCore/include/lifelens/NaturalWorldChunk.h')
world = read('Source/LifeLensCore/include/lifelens/World.h')
sim = read('Source/LifeLensCore/src/Simulation.cpp')
codec_h = read('Source/LifeLensCore/include/lifelens/SimulationSnapshotCodec.h')
codec = read('Source/LifeLensCore/src/SimulationSnapshotCodec.cpp')
wg_codec = read('Source/LifeLensCore/include/lifelens/WorldGenerationSnapshotCodec.h')
bridge_h = read('Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h')
bridge = read('Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp')
director = read('Source/LifeLens/World/LLWorldDirector.cpp')
visual = read('Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.cpp')
test = read('Source/LifeLensCore/tests/test_world_generation_milestone_a.cpp')
cmake = read('Source/LifeLensCore/CMakeLists.txt')

for token in [
    'struct GeneratedNaturalChunk', 'struct NaturalResourcePatch',
    'deriveGeneratedNaturalChunk', 'deriveNaturalResourceNodeId',
    'deriveUntouchedChunkBaseline', 'deriveMacroRegionFacts',
    'resourcePatches', 'NaturalSurfaceKind']:
    assert token in natural, f'missing natural chunk contract: {token}'

assert 'populationSeed' not in natural, 'natural chunk detail must not depend on PopulationSeed'
assert 'std::hash<' not in natural, 'natural chunk detail must use stable world-genesis mixer'
for token in ['generatedNaturalChunks', 'materializeNaturalChunk', 'establishInitialStartRegion', 'initialStartRegionCenterGrid']:
    assert token in world, f'missing World materialization contract: {token}'
for token in ['world_.storageSites.clear()', 'world_.materializeNaturalChunk', 'initialRuntime.pos']:
    assert token in sim, f'missing production New Game integration: {token}'

assert 'SimulationSnapshotBinaryFormatVersion=6' in codec_h
assert 'WorldGenerationSnapshotExtensionMagic' in wg_codec
assert 'writeWorldGenerationSnapshotExtension' in codec
assert 'readWorldGenerationSnapshotExtension' in codec
assert 'validateWorldGenerationSnapshotState' in codec

assert 'GetWorldGenerationObservation' in bridge_h
assert 'GetNaturalChunkObservation' in bridge_h
assert 'FillNaturalChunkObservation' in bridge
assert 'InitialCenterGridX' in bridge

assert 'RefreshCorePresentationOrigin' in director
assert 'CorePresentationOriginGrid' in director
assert 'GridX - CorePresentationOriginGrid.X' in director
assert 'GridX - CoreOriginGridX' in visual

assert 'lifelens_add_test(test_world_generation_milestone_a)' in cmake
for token in ['sameGeneratedNaturalChunkBaseline', 'storageSites.empty()', 'bytes==reencoded', 'chunkCoordForGrid(pos)==selected.region.coord']:
    assert token in test, f'missing milestone regression coverage: {token}'

print('World Generation Milestone A structural validation: PASS')
''')

print('World Generation Milestone A patch applied')
