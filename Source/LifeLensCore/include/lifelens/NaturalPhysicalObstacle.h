#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "NaturalWorldChunk.h"

namespace lifelens {

enum class NaturalPhysicalObstacleKind : std::uint8_t {
    Tree = 0,
    Rock
};

struct NaturalPhysicalObstacle {
    std::uint64_t id = 0;
    NaturalPhysicalObstacleKind kind = NaturalPhysicalObstacleKind::Tree;
    ResourceNodeId sourceNodeId = 0;
    GridPos grid{};
    double offsetXCells = 0.0;
    double offsetYCells = 0.0;
    double halfExtentXCells = 0.0;
    double halfExtentYCells = 0.0;
    double halfHeightCells = 0.0;
    std::uint64_t styleSeed = 0;
};

inline bool validNaturalPhysicalObstacleKind(NaturalPhysicalObstacleKind kind)
{
    return kind == NaturalPhysicalObstacleKind::Tree
        || kind == NaturalPhysicalObstacleKind::Rock;
}

inline bool materialCreatesNaturalPhysicalObstacle(MaterialKind material)
{
    return material == MaterialKind::Wood || material == MaterialKind::Stone;
}

inline int naturalPhysicalObstacleCount(const NaturalResourcePatch& patch)
{
    const double density = std::max(0.0,std::min(1.0,patch.visualDensity));
    if(patch.material == MaterialKind::Wood){
        return std::max(2,std::min(6,2 + static_cast<int>(std::round(density * 4.0))));
    }
    if(patch.material == MaterialKind::Stone){
        if(density < 0.25) return 0;
        return std::max(1,std::min(3,1 + static_cast<int>(std::round(density * 2.0))));
    }
    return 0;
}

inline std::uint64_t naturalPhysicalObstacleId(
    const NaturalResourcePatch& patch,
    NaturalPhysicalObstacleKind kind,
    int ordinal)
{
    constexpr std::uint64_t ObstacleDomain = 0x4e41544f42533131ULL; // NATOBS11
    std::uint64_t state = combineWorldGenesisWord(patch.detailSeed,ObstacleDomain);
    state = combineWorldGenesisWord(state,static_cast<std::uint64_t>(kind));
    state = combineWorldGenesisWord(state,static_cast<std::uint64_t>(std::max(0,ordinal)));
    std::uint64_t id = worldGenesisMix64(state) & 0x3fffffffffffffffULL;
    id |= 0x2000000000000000ULL;
    return id == 0 ? 1 : id;
}

inline double naturalObstacleUnit(std::uint64_t seed,std::uint64_t salt)
{
    return macroUnitFromWord(worldGenesisMix64(seed ^ salt));
}

inline std::vector<NaturalPhysicalObstacle> deriveNaturalPhysicalObstacles(
    const GeneratedNaturalChunk& chunk)
{
    std::vector<NaturalPhysicalObstacle> result;
    constexpr std::array<GridPos,8> offsets={
        GridPos{-1,-1},GridPos{0,-1},GridPos{1,-1},GridPos{1,0},
        GridPos{1,1},GridPos{0,1},GridPos{-1,1},GridPos{-1,0}
    };

    for(const NaturalResourcePatch& patch:chunk.resourcePatches){
        if(!materialCreatesNaturalPhysicalObstacle(patch.material)) continue;
        const int count=naturalPhysicalObstacleCount(patch);
        if(count<=0) continue;

        const NaturalPhysicalObstacleKind kind = patch.material == MaterialKind::Wood
            ? NaturalPhysicalObstacleKind::Tree
            : NaturalPhysicalObstacleKind::Rock;
        const std::size_t start=static_cast<std::size_t>(
            worldGenesisMix64(patch.detailSeed ^ 0x4f46465345543131ULL) % offsets.size());

        for(int ordinal=0;ordinal<count;++ordinal){
            const GridPos offset=offsets[(start+static_cast<std::size_t>(ordinal))%offsets.size()];
            const std::uint64_t styleSeed=worldGenesisMix64(
                patch.detailSeed ^ (0x5354594c45303031ULL + static_cast<std::uint64_t>(ordinal)*0x9e3779b97f4a7c15ULL));

            NaturalPhysicalObstacle obstacle;
            obstacle.id=naturalPhysicalObstacleId(patch,kind,ordinal);
            obstacle.kind=kind;
            obstacle.sourceNodeId=patch.nodeId;
            obstacle.grid={patch.pos.x+offset.x,patch.pos.y+offset.y};
            obstacle.offsetXCells=(naturalObstacleUnit(styleSeed,0x584f464631ULL)-0.5)*0.28;
            obstacle.offsetYCells=(naturalObstacleUnit(styleSeed,0x594f464631ULL)-0.5)*0.28;
            obstacle.styleSeed=styleSeed;

            if(kind==NaturalPhysicalObstacleKind::Tree){
                const double radius=0.16+0.08*naturalObstacleUnit(styleSeed,0x5452414431ULL);
                obstacle.halfExtentXCells=radius;
                obstacle.halfExtentYCells=radius;
                obstacle.halfHeightCells=0.62+0.28*naturalObstacleUnit(styleSeed,0x5448454931ULL);
            }else{
                obstacle.halfExtentXCells=0.24+0.16*naturalObstacleUnit(styleSeed,0x524f434b31ULL);
                obstacle.halfExtentYCells=0.22+0.16*naturalObstacleUnit(styleSeed,0x524f434b32ULL);
                obstacle.halfHeightCells=0.20+0.22*naturalObstacleUnit(styleSeed,0x524f434b33ULL);
            }
            result.push_back(obstacle);
        }
    }

    std::sort(result.begin(),result.end(),[](const NaturalPhysicalObstacle& a,const NaturalPhysicalObstacle& b){
        return a.id<b.id;
    });
    return result;
}

inline bool validNaturalPhysicalObstacle(
    const NaturalPhysicalObstacle& obstacle,
    ChunkCoord ownerCoord)
{
    return obstacle.id!=0
        && validNaturalPhysicalObstacleKind(obstacle.kind)
        && obstacle.sourceNodeId!=0
        && chunkCoordForGrid(obstacle.grid)==ownerCoord
        && obstacle.offsetXCells>=-0.5 && obstacle.offsetXCells<=0.5
        && obstacle.offsetYCells>=-0.5 && obstacle.offsetYCells<=0.5
        && obstacle.halfExtentXCells>0.0 && obstacle.halfExtentXCells<=0.5
        && obstacle.halfExtentYCells>0.0 && obstacle.halfExtentYCells<=0.5
        && obstacle.halfHeightCells>0.0 && obstacle.halfHeightCells<=1.5
        && obstacle.styleSeed!=0;
}

} // namespace lifelens
