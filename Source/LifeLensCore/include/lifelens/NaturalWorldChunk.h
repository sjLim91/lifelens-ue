#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "Civilization.h"
#include "Hydrology.h"
#include "MacroWorldGenesis.h"

namespace lifelens {

enum class NaturalSurfaceKind : std::uint8_t {
    Meadow = 0,
    ForestFloor,
    Plains,
    RockyGround,
    WetGround,
    DryGround,
    ColdGround,
    Coast,
    Ocean
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
        case NaturalSurfaceKind::Coast: return "Coast";
        case NaturalSurfaceKind::Ocean: return "Ocean";
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
        case NaturalSurfaceKind::Coast:
        case NaturalSurfaceKind::Ocean:
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
        case MaterialKind::CopperOre:
            return clampMacro01(
                facts.stonePotential*0.25
                +facts.elevation*0.10
                +facts.hazardPotential*0.07
                -0.04);
        case MaterialKind::TinOre:
            return clampMacro01(
                facts.stonePotential*0.18
                +facts.elevation*0.09
                +facts.hazardPotential*0.06
                -0.06);
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
        case MaterialKind::CopperOre: return 12;
        case MaterialKind::TinOre: return 8;
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
        case MaterialKind::CopperOre: return 54;
        case MaterialKind::TinOre: return 36;
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
    const MacroSurfaceFacts macroSurface =
        deriveMacroSurfaceFacts(identity, coord);
    const HydrologyFacts hydrology =
        deriveHydrologyFacts(identity, coord);
    chunk.surface = macroSurface.surfaceClass == MacroSurfaceClass::Ocean
        ? NaturalSurfaceKind::Ocean
        : (macroSurface.surfaceClass == MacroSurfaceClass::Coast
            ? NaturalSurfaceKind::Coast
            : naturalSurfaceForBiome(macro.biome));
    chunk.elevation = macro.elevation;
    chunk.moisture = macro.moisture;
    chunk.temperature = macro.temperature;
    chunk.waterPotential = macro.waterPotential;
    chunk.fertilityPotential = macro.fertilityPotential;
    chunk.traversalEase = macro.traversalEase;
    chunk.hazardPotential = macro.hazardPotential;
    chunk.materializedMinute = std::max(0, materializedMinute);

    const std::array<MaterialKind, 9> materials = {
        MaterialKind::Water,
        MaterialKind::Wood,
        MaterialKind::Stone,
        MaterialKind::Flint,
        MaterialKind::Fiber,
        MaterialKind::Clay,
        MaterialKind::PlantFood,
        MaterialKind::CopperOre,
        MaterialKind::TinOre
    };

    // Ocean is represented by hydrology, not a drinkable Water ResourceNode
    // or terrestrial resource catalogue. Coastal land may keep terrestrial
    // materials, but brackish/salt surface water is not synthesized as fresh
    // inventory water.
    if(macroSurface.surfaceClass == MacroSurfaceClass::Ocean){
        return chunk;
    }

    const GridPos origin = chunkOriginGrid(coord);
    for(MaterialKind material : materials){
        const bool waterMaterial = material == MaterialKind::Water;
        if(waterMaterial && !isFreshSurfaceWater(hydrology)){
            // Do not manufacture invisible "Water" inventory from a generic
            // potential field. Drinkable natural Water must correspond to an
            // actual authoritative fresh spring/stream/river/lake/wetland.
            continue;
        }

        const double potential = waterMaterial
            ? hydrology.surfaceAvailability
            : naturalPotentialForMaterial(macro, material);
        const bool metalOre=
            material==MaterialKind::CopperOre
            || material==MaterialKind::TinOre;
        const int maxPatches = waterMaterial
            ? 1
            : (metalOre ? 2 : 4);
        const int patchCount = naturalPatchCount(potential, maxPatches);
        for(int ordinal = 0; ordinal < patchCount; ++ordinal){
            const ResourceNodeId nodeId = deriveNaturalResourceNodeId(untouched.chunkSeed, material, ordinal);
            std::uint64_t detailSeed = combineWorldGenesisWord(
                untouched.detailStreamSeed,
                static_cast<std::uint64_t>(static_cast<int>(material) * 17 + ordinal));
            const double amountNoise = macroUnitFromWord(worldGenesisMix64(detailSeed ^ 0x414d4f554e543031ULL));
            const double densityNoise = macroUnitFromWord(worldGenesisMix64(detailSeed ^ 0x44454e5349545931ULL));
            const int localSpan = std::max(1, WorldChunkSpanGridCells - 4);
            const int localX = waterMaterial
                ? WorldChunkSpanGridCells / 2
                : 2 + static_cast<int>(
                    worldGenesisMix64(detailSeed ^ 0x504f535f585f3031ULL)
                    % static_cast<std::uint64_t>(localSpan));
            const int localY = waterMaterial
                ? WorldChunkSpanGridCells / 2
                : 2 + static_cast<int>(
                    worldGenesisMix64(detailSeed ^ 0x504f535f595f3031ULL)
                    % static_cast<std::uint64_t>(localSpan));

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

inline bool validNaturalResourceMaterial(MaterialKind material)
{
    switch(material){
        case MaterialKind::Water:
        case MaterialKind::Wood:
        case MaterialKind::Stone:
        case MaterialKind::Flint:
        case MaterialKind::Fiber:
        case MaterialKind::Clay:
        case MaterialKind::PlantFood:
        case MaterialKind::CopperOre:
        case MaterialKind::TinOre:
            return true;
        default:
            return false;
    }
}

inline bool validNaturalResourcePatch(const NaturalResourcePatch& patch, ChunkCoord ownerCoord)
{
    return patch.nodeId != 0
        && validNaturalResourceMaterial(patch.material)
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
