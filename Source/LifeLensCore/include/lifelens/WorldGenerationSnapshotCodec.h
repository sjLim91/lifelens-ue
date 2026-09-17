#pragma once

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
               || node->renewable != patch.renewable) return false;
            // The generated patch owns the immutable natural regeneration
            // baseline. E3 may cache a weather-adjusted per-day rate on the
            // live ResourceNode; that derived runtime value is intentionally
            // allowed to differ from patch.regenerationPerDay and is already
            // range-validated by the civilization snapshot validator.
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