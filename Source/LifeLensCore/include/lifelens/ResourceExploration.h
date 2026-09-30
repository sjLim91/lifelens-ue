#pragma once

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <limits>

#include "World.h"

namespace lifelens {

// Exploration is a Core-authored frontier action. It may use immutable terrain
// facts to pick a physically plausible destination, but it must never inspect an
// unmaterialized chunk's resource catalogue before a resident actually reaches
// that frontier.
inline constexpr int ResourceExplorationMaxRadiusChunks = 6;
inline constexpr int ResourceExplorationDrySearchRadiusCells = 8;

// Ordinary exploration remains deliberately local. Critical survival may need
// to escape a fully explored/depleted settlement envelope after many simulated
// months, so derive a wider radius from the amount of world already materialized.
inline constexpr int ResourceExplorationCriticalMinimumRadiusChunks = 12;

inline int criticalResourceExplorationRadiusChunks(const World& world)
{
    const double generated=static_cast<double>(
        std::max<std::size_t>(1,world.generatedNaturalChunks.size()));
    // If a dense square around the resident were fully materialized, roughly
    // (2r+1)^2 chunks would exist. Search a few rings beyond that theoretical
    // dense radius so a finite explored world keeps an escape frontier.
    const int denseRadius=static_cast<int>(std::ceil(
        (std::sqrt(generated+1.0)-1.0)*0.5));
    return std::max(
        ResourceExplorationCriticalMinimumRadiusChunks,
        denseRadius+6);
}

struct ResourceExplorationOpportunity {
    bool available = false;
    MaterialKind material = MaterialKind::Unknown;
    ChunkCoord chunk{};
    GridPos target{};
    double suitability = 0.0;
};

inline double resourceExplorationPreference(
    const World& world,
    CharacterId actor,
    MaterialKind material,
    ChunkCoord chunk)
{
    std::uint64_t value = world.seed ? world.seed : 1;
    value = civilizationMix(value ^ actor);
    value = civilizationMix(
        value ^ (static_cast<std::uint64_t>(static_cast<int>(material)) + 1ULL)
            * 0x9e3779b97f4a7c15ULL);
    value = civilizationMix(
        value ^ (static_cast<std::uint64_t>(
            static_cast<std::uint32_t>(chunk.x)) << 32U)
            ^ static_cast<std::uint32_t>(chunk.y));
    const std::uint64_t mantissa = value >> 11;
    return static_cast<double>(mantissa) * (1.0 / 9007199254740992.0);
}

inline bool chooseDryExplorationEntryPoint(
    const World& world,
    ChunkCoord chunk,
    GridPos from,
    GridPos& outTarget)
{
    const MacroSurfaceFacts surface =
        deriveMacroSurfaceFacts(world.genesisIdentity(), chunk);
    if(surface.surfaceClass == MacroSurfaceClass::Ocean) return false;

    const HydrologyFacts hydrology =
        deriveHydrologyFacts(world.genesisIdentity(), chunk);
    const GridPos origin = chunkOriginGrid(chunk);
    const int minimumX = origin.x + 1;
    const int maximumX = origin.x + WorldChunkSpanGridCells - 2;
    const int minimumY = origin.y + 1;
    const int maximumY = origin.y + WorldChunkSpanGridCells - 2;

    const GridPos nearest{
        std::max(minimumX, std::min(maximumX, from.x)),
        std::max(minimumY, std::min(maximumY, from.y))
    };

    const auto valid = [&](GridPos candidate)
    {
        return candidate.x >= minimumX && candidate.x <= maximumX
            && candidate.y >= minimumY && candidate.y <= maximumY
            && !surfaceWaterGroundContainsGrid(hydrology, candidate);
    };

    if(valid(nearest)){
        outTarget = nearest;
        return true;
    }

    for(int radius = 1; radius <= ResourceExplorationDrySearchRadiusCells; ++radius){
        for(int dx = -radius; dx <= radius; ++dx){
            const GridPos top{nearest.x + dx, nearest.y - radius};
            if(valid(top)){
                outTarget = top;
                return true;
            }
            const GridPos bottom{nearest.x + dx, nearest.y + radius};
            if(valid(bottom)){
                outTarget = bottom;
                return true;
            }
        }
        for(int dy = -radius + 1; dy <= radius - 1; ++dy){
            const GridPos left{nearest.x - radius, nearest.y + dy};
            if(valid(left)){
                outTarget = left;
                return true;
            }
            const GridPos right{nearest.x + radius, nearest.y + dy};
            if(valid(right)){
                outTarget = right;
                return true;
            }
        }
    }

    return false;
}

inline ResourceExplorationOpportunity chooseResourceExplorationOpportunity(
    const World& world,
    CharacterId actor,
    MaterialKind material,
    GridPos authoritativePosition,
    int maxRadiusChunks=ResourceExplorationMaxRadiusChunks)
{
    ResourceExplorationOpportunity best;
    if(actor == 0 || !validNaturalResourceMaterial(material)) return best;

    const ChunkCoord center = chunkCoordForGrid(authoritativePosition);

    // Search nearest frontier ring first. Within the same ring, use only
    // observable terrain quality plus a stable actor/material preference. Do
    // not call deriveGeneratedNaturalChunk here: resources stay unknown until
    // arrival materializes the chunk.
    const int radiusLimit=std::max(1,maxRadiusChunks);
    for(int radius = 1; radius <= radiusLimit; ++radius){
        bool foundAtRadius = false;
        double bestScore = -std::numeric_limits<double>::infinity();

        for(int dx = -radius; dx <= radius; ++dx){
            for(int dy = -radius; dy <= radius; ++dy){
                if(std::max(std::abs(dx), std::abs(dy)) != radius) continue;

                const ChunkCoord candidate{center.x + dx, center.y + dy};
                if(world.findGeneratedNaturalChunk(candidate) != nullptr) continue;

                const MacroSurfaceFacts surface =
                    deriveMacroSurfaceFacts(world.genesisIdentity(), candidate);
                if(surface.surfaceClass == MacroSurfaceClass::Ocean) continue;

                GridPos target{};
                if(!chooseDryExplorationEntryPoint(
                        world, candidate, authoritativePosition, target)){
                    continue;
                }

                const MacroRegionFacts region =
                    deriveMacroRegionFacts(world.genesisIdentity(), candidate);
                const double preference =
                    resourceExplorationPreference(world, actor, material, candidate);
                const double score =
                    0.60 * clampMacro01(region.traversalEase)
                    + 0.25 * (1.0 - clampMacro01(region.hazardPotential))
                    + 0.15 * preference;

                if(!foundAtRadius || score > bestScore + 1e-12){
                    foundAtRadius = true;
                    bestScore = score;
                    best.available = true;
                    best.material = material;
                    best.chunk = candidate;
                    best.target = target;
                    best.suitability = score;
                }
            }
        }

        if(foundAtRadius) return best;
    }

    return ResourceExplorationOpportunity{};
}

} // namespace lifelens
