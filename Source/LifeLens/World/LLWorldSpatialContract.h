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

    // Observer framing is expressed relative to the authoritative chunk span,
    // not to the old 1,400 UU bootstrap floor. This keeps the initial camera
    // useful as WorldPresentation expands while preserving the selected start
    // chunk at Unreal presentation origin.
    inline constexpr float ObserverCameraDistanceChunks = 2.0f;
    inline constexpr float ObserverCameraHeightChunks = 1.25f;
    inline constexpr float ObserverCameraDistanceUU =
        ChunkSpanUU * ObserverCameraDistanceChunks;
    inline constexpr float ObserverCameraHeightUU =
        ChunkSpanUU * ObserverCameraHeightChunks;
    inline constexpr float ObserverCameraTargetHeightUU = GridCellSizeUU;
    inline constexpr float ObserverCameraFOVDegrees = 60.0f;

    // Map presentation-space XY back to the logical chunk grid. The initial
    // Core chunk remains the presentation anchor, but the observer may move to
    // any other logical chunk without changing simulation authority.
    inline int32 PresentationChunkOffsetForAxis(float AxisUU)
    {
        const float HalfChunkUU = ChunkSpanUU * 0.5f;
        return FMath::FloorToInt(
            (AxisUU + HalfChunkUU)
            / FMath::Max(1.0f, ChunkSpanUU));
    }

    inline FIntPoint LogicalChunkForPresentationLocation(
        int32 AnchorChunkX,
        int32 AnchorChunkY,
        const FVector2D& LocationUU)
    {
        return FIntPoint(
            AnchorChunkX + PresentationChunkOffsetForAxis(LocationUU.X),
            AnchorChunkY + PresentationChunkOffsetForAxis(LocationUU.Y));
    }

    inline FVector2D PresentationLocationForLogicalChunk(
        int32 AnchorChunkX,
        int32 AnchorChunkY,
        int32 ChunkX,
        int32 ChunkY)
    {
        return FVector2D(
            static_cast<float>(ChunkX - AnchorChunkX) * ChunkSpanUU,
            static_cast<float>(ChunkY - AnchorChunkY) * ChunkSpanUU);
    }

    static_assert(ChunkSpanGridCells > 0, "World chunk span must stay positive");
    static_assert(GridCellSizeUU > 0.0f, "Presentation grid scale must stay positive");
    static_assert(ObserverCameraDistanceUU > ChunkSpanUU, "Observer camera must frame beyond one start chunk");
    static_assert(ObserverCameraHeightUU > 0.0f, "Observer camera height must stay positive");
}
