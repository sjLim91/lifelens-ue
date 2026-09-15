#pragma once

#include "CoreMinimal.h"
#include "lifelens/WorldGenesis.h"

// Shared read-only spatial contract between the authoritative Core grid and
// Unreal presentation. World/WorldPresentation code may consume these values,
// but presentation must never change Core coordinates to fit a temporary map.
namespace LLWorldSpatialContract
{
    // Presentation scale for one authoritative Core GridPos cell.
    inline constexpr float GridCellSizeUU = 100.0f;

    // Authoritative logical chunk width comes from LifeLensCore WG-1.
    inline constexpr int32 ChunkSpanGridCells = lifelens::WorldChunkSpanGridCells;
    inline constexpr float ChunkSpanUU =
        static_cast<float>(ChunkSpanGridCells) * GridCellSizeUU;

    // Bootstrap-only visible ground should cover at least the whole selected
    // start chunk plus a one-cell visual/collision margin on every edge.
    inline constexpr float BootstrapFloorMarginUU = GridCellSizeUU;
    inline constexpr float BootstrapFloorSpanUU =
        ChunkSpanUU + 2.0f * BootstrapFloorMarginUU;

    // /Engine/BasicShapes/Cube has a 100 UU side length at scale 1.
    inline constexpr float EngineCubeSideUU = 100.0f;
    inline constexpr float BootstrapFloorCubeScale =
        BootstrapFloorSpanUU / EngineCubeSideUU;

    static_assert(ChunkSpanGridCells > 0, "World chunk span must stay positive");
    static_assert(GridCellSizeUU > 0.0f, "Presentation grid scale must stay positive");
}
