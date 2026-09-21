#pragma once

#include "CoreMinimal.h"
#include "Simulation/LLWorldGenerationReadTypes.h"
#include "World/LLWorldSpatialContract.h"

/**
 * Shared local-surface presentation contract.
 *
 * World terrain, desktop smooth terrain and water must resolve the same visual
 * surface from the same authoritative Core terrain samples. Keeping the math in
 * one place prevents a presentation-only tuning change from making trees,
 * facilities or water float above/bury into another renderer.
 *
 * The resident/navigation authority remains the flat Core-local physical plane.
 * This contract is visual-only.
 */
namespace LLTerrainPresentationContract
{
    // Keep local materialized relief within the resident visual-grounding lift
    // budget while using the whole available range. The previous 180 UU ceiling
    // made coherent neighbouring elevation differences read almost perfectly flat.
    inline constexpr float LocalReliefAmplitudeUU = 220.0f;
    inline constexpr float FacilityFlattenRadiusUU = 340.0f;
    inline constexpr float FacilityBlendEndRadiusUU = 900.0f;
    inline constexpr float DesktopSurfaceLiftUU = 1.0f;
    inline constexpr int32 RegionalPreviewRadiusChunks = 8;
    // Only the materialized start surface needs the local flat compatibility
    // envelope. Regional preview is presentation-only and can immediately begin
    // building macro relief outside it.
    inline constexpr int32 RegionalInnerFlatRingChunks = 0;
    // Macro terrain needs metre-scale relief to read as hills/valleys at the
    // Observer camera distance. 8000 UU is an 80 m full-range presentation
    // envelope; coherent neighbour deltas use only a fraction of it.
    inline constexpr float RegionalReliefAmplitudeUU = 8000.0f;

    inline float SmoothBand(float Distance, float Start, float End)
    {
        const float SafeStart = FMath::Max(0.0f, Start);
        const float SafeEnd = FMath::Max(SafeStart + 1.0f, End);
        const float Alpha = FMath::Clamp(
            (Distance - SafeStart) / (SafeEnd - SafeStart),
            0.0f,
            1.0f);
        return Alpha * Alpha * (3.0f - 2.0f * Alpha);
    }

    inline float FacilityReliefBlend(
        const FVector2D& LocationUU,
        const TArray<FVector2D>& FacilityCentersUU)
    {
        // World v2 natural terrain exists before residents. The opening spawn
        // point is only a coordinate anchor and must never flatten the baseline.
        // Only actual constructed facilities may request a local visual pad.
        float Blend = 1.0f;
        for (const FVector2D& FacilityCenter : FacilityCentersUU)
        {
            Blend = FMath::Min(
                Blend,
                SmoothBand(
                    FVector2D::Distance(LocationUU, FacilityCenter),
                    FacilityFlattenRadiusUU,
                    FacilityBlendEndRadiusUU));
        }
        return FMath::Clamp(Blend, 0.0f, 1.0f);
    }

    inline float LocalSurfaceZUU(
        const FLLCoreWorldGenerationObservation& World,
        const FLLCoreTerrainPresentationObservation& Terrain,
        const FVector2D& LocationUU,
        const TArray<FVector2D>& FacilityCentersUU,
        float SurfaceLiftUU = 0.0f)
    {
        if (!Terrain.bAvailable)
        {
            return SurfaceLiftUU;
        }

        const FVector2D ChunkCenter(
            static_cast<float>(Terrain.ChunkX - World.InitialChunkX)
                * LLWorldSpatialContract::ChunkSpanUU,
            static_cast<float>(Terrain.ChunkY - World.InitialChunkY)
                * LLWorldSpatialContract::ChunkSpanUU);
        const float Half = LLWorldSpatialContract::ChunkSpanUU * 0.5f;
        const float U = FMath::Clamp(
            (LocationUU.X - (ChunkCenter.X - Half))
                / FMath::Max(LLWorldSpatialContract::ChunkSpanUU, 1.0f),
            0.0f,
            1.0f);
        const float V = FMath::Clamp(
            (LocationUU.Y - (ChunkCenter.Y - Half))
                / FMath::Max(LLWorldSpatialContract::ChunkSpanUU, 1.0f),
            0.0f,
            1.0f);

        auto RelativeHeight = [&](float Elevation01)
        {
            // Local physical gameplay still uses the compatibility plane, so
            // this milestone keeps local relief non-negative. Unlike the old
            // contract, the spawn point no longer suppresses natural relief.
            return FMath::Max(
                0.0f,
                Elevation01 - World.InitialChunk.Elevation)
                * LocalReliefAmplitudeUU;
        };

        const float South = FMath::Lerp(
            RelativeHeight(Terrain.SouthWestElevation01),
            RelativeHeight(Terrain.SouthEastElevation01),
            U);
        const float North = FMath::Lerp(
            RelativeHeight(Terrain.NorthWestElevation01),
            RelativeHeight(Terrain.NorthEastElevation01),
            U);
        const float CornerSurface = FMath::Lerp(South, North, V);
        const float CenterSurface = RelativeHeight(Terrain.CenterElevation01);
        const float SharedSurface =
            FMath::Lerp(CenterSurface, CornerSurface, 0.72f)
            * FacilityReliefBlend(
                LocationUU,
                FacilityCentersUU);

        return SurfaceLiftUU + SharedSurface;
    }

    inline float RegionalSurfaceZUU(
        const FLLCoreWorldGenerationObservation& World,
        const FLLCoreTerrainPresentationObservation& Terrain,
        const FVector2D& LocationUU)
    {
        if (!Terrain.bAvailable)
        {
            return 0.0f;
        }

        const int32 Ring = FMath::Max(
            FMath::Abs(Terrain.ChunkX - World.InitialChunkX),
            FMath::Abs(Terrain.ChunkY - World.InitialChunkY));
        const int32 InnerRing = FMath::Clamp(
            RegionalInnerFlatRingChunks,
            0,
            FMath::Max(0, RegionalPreviewRadiusChunks - 1));
        const float Denominator = static_cast<float>(
            FMath::Max(1, RegionalPreviewRadiusChunks - InnerRing));
        const float RawAlpha = FMath::Clamp(
            static_cast<float>(Ring - InnerRing) / Denominator,
            0.0f,
            1.0f);
        const float ReliefAlpha =
            RawAlpha * RawAlpha * (3.0f - 2.0f * RawAlpha);
        const float Amplitude = FMath::Lerp(
            LocalReliefAmplitudeUU,
            FMath::Max(LocalReliefAmplitudeUU, RegionalReliefAmplitudeUU),
            ReliefAlpha);

        auto RegionalHeight = [&](float Elevation01)
        {
            const float Delta =
                Elevation01 - World.InitialChunk.Elevation;

            // Ring 1 is rendered now, but it must meet the materialized local
            // surface without suddenly exposing a signed valley/cliff at the
            // bootstrap boundary. Keep the local non-negative baseline policy
            // through the compatibility ring, then allow signed macro valleys
            // once the observer is farther from local gameplay authority.
            if (Ring <= InnerRing)
            {
                return FMath::Max(0.0f, Delta)
                    * LocalReliefAmplitudeUU;
            }
            return Delta * Amplitude;
        };

        const FVector2D ChunkCenter(
            static_cast<float>(Terrain.ChunkX - World.InitialChunkX)
                * LLWorldSpatialContract::ChunkSpanUU,
            static_cast<float>(Terrain.ChunkY - World.InitialChunkY)
                * LLWorldSpatialContract::ChunkSpanUU);
        const float Half = LLWorldSpatialContract::ChunkSpanUU * 0.5f;
        const float U = FMath::Clamp(
            (LocationUU.X - (ChunkCenter.X - Half))
                / FMath::Max(LLWorldSpatialContract::ChunkSpanUU, 1.0f),
            0.0f,
            1.0f);
        const float V = FMath::Clamp(
            (LocationUU.Y - (ChunkCenter.Y - Half))
                / FMath::Max(LLWorldSpatialContract::ChunkSpanUU, 1.0f),
            0.0f,
            1.0f);

        const float South = FMath::Lerp(
            RegionalHeight(Terrain.SouthWestElevation01),
            RegionalHeight(Terrain.SouthEastElevation01),
            U);
        const float North = FMath::Lerp(
            RegionalHeight(Terrain.NorthWestElevation01),
            RegionalHeight(Terrain.NorthEastElevation01),
            U);
        const float CornerSurface = FMath::Lerp(South, North, V);
        const float CenterSurface = RegionalHeight(Terrain.CenterElevation01);
        return FMath::Lerp(CenterSurface, CornerSurface, 0.72f);
    }
}
