#pragma once

#include <algorithm>
#include <array>
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

// 긴급 생존 탐색은 일반 탐색보다 멀리 갈 수 있지만, 월드 크기에
// 비례해 모든 링을 전수 검사하지 않는다. 아래의 희소 프런티어 탐색은
// 일정한 수의 후보만 검사해 장기 시뮬레이션 비용을 제한한다.
inline constexpr std::array<int,11> ResourceExplorationCriticalRadiiChunks{
    7,8,10,12,16,24,32,48,64,96,128
};

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
    GridPos authoritativePosition)
{
    ResourceExplorationOpportunity best;
    if(actor == 0 || !validNaturalResourceMaterial(material)) return best;

    const ChunkCoord center = chunkCoordForGrid(authoritativePosition);

    // Search nearest frontier ring first. Within the same ring, use only
    // observable terrain quality plus a stable actor/material preference. Do
    // not call deriveGeneratedNaturalChunk here: resources stay unknown until
    // arrival materializes the chunk.
    for(int radius = 1; radius <= ResourceExplorationMaxRadiusChunks; ++radius){
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


inline ResourceExplorationOpportunity chooseCriticalResourceExplorationOpportunity(
    const World& world,
    CharacterId actor,
    MaterialKind material,
    GridPos authoritativePosition)
{
    // 가까운 일반 프런티어가 남아 있으면 기존 규칙을 그대로 사용한다.
    ResourceExplorationOpportunity local=
        chooseResourceExplorationOpportunity(
            world,actor,material,authoritativePosition);
    if(local.available) return local;
    if(actor==0 || !validNaturalResourceMaterial(material)){
        return ResourceExplorationOpportunity{};
    }

    const ChunkCoord center=chunkCoordForGrid(authoritativePosition);

    // 각 거리에서 16개 방향만 확인한다. 따라서 이미 생성된 월드가
    // 수천 청크여도 긴급 의사결정 비용은 상수에 가깝게 유지된다.
    for(const int radius:ResourceExplorationCriticalRadiiChunks){
        const int half=std::max(1,radius/2);
        const std::array<ChunkCoord,16> offsets{{
            { radius,0},{-radius,0},{0, radius},{0,-radius},
            { radius, radius},{ radius,-radius},
            {-radius, radius},{-radius,-radius},
            { radius, half},{ radius,-half},
            {-radius, half},{-radius,-half},
            { half, radius},{-half, radius},
            { half,-radius},{-half,-radius}
        }};

        ResourceExplorationOpportunity best;
        double bestScore=-std::numeric_limits<double>::infinity();

        for(const ChunkCoord offset:offsets){
            const ChunkCoord candidate{
                center.x+offset.x,
                center.y+offset.y
            };
            if(world.findGeneratedNaturalChunk(candidate)!=nullptr) continue;

            const MacroSurfaceFacts surface=
                deriveMacroSurfaceFacts(world.genesisIdentity(),candidate);
            if(surface.surfaceClass==MacroSurfaceClass::Ocean) continue;

            GridPos target{};
            if(!chooseDryExplorationEntryPoint(
                    world,candidate,authoritativePosition,target)){
                continue;
            }

            const MacroRegionFacts region=
                deriveMacroRegionFacts(world.genesisIdentity(),candidate);
            const double preference=
                resourceExplorationPreference(
                    world,actor,material,candidate);
            const double score=
                0.60*clampMacro01(region.traversalEase)
                +0.25*(1.0-clampMacro01(region.hazardPotential))
                +0.15*preference;

            if(!best.available || score>bestScore+1e-12){
                best.available=true;
                best.material=material;
                best.chunk=candidate;
                best.target=target;
                best.suitability=score;
                bestScore=score;
            }
        }

        if(best.available) return best;
    }

    return ResourceExplorationOpportunity{};
}

} // namespace lifelens
