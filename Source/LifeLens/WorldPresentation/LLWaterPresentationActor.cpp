#include "WorldPresentation/LLWaterPresentationActor.h"

#include "Components/SplineComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLCivilizationReadTypes.h"
#include "Simulation/LLWorldGenerationReadTypes.h"
#include "UObject/UnrealType.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"
#include "WaterBodyLakeActor.h"
#include "WaterBodyRiverActor.h"
#include "WaterSplineComponent.h"
#include "WaterSplineMetadata.h"
#include "WaterZoneActor.h"
#include "World/LLWorldSpatialContract.h"
#include "WorldPresentation/LLTerrainPresentationContract.h"

namespace
{
uint32 MixWaterHash(uint32 Seed, uint32 Value)
{
    Seed ^= Value + 0x9E3779B9u + (Seed << 6) + (Seed >> 2);
    return Seed;
}

uint32 SurfaceWaterSignature(
    const FLLCoreWorldGenerationObservation& World,
    const TArray<FLLCoreSurfaceWaterPresentationObservation>& Observations,
    const FLLCoreCivilizationWorldObservation& Civilization)
{
    uint32 Hash = 0x57415452u; // WATR

    // GridToWorld and terrain-relative water height depend on the selected
    // world/start-region frame as well as the hydrology DTOs. Any of these
    // changing must invalidate the already-spawned WaterBody actors.
    const uint64 WorldSeed = static_cast<uint64>(World.WorldSeed);
    Hash = MixWaterHash(Hash, static_cast<uint32>(WorldSeed & 0xFFFFFFFFu));
    Hash = MixWaterHash(Hash, static_cast<uint32>((WorldSeed >> 32) & 0xFFFFFFFFu));
    Hash = MixWaterHash(Hash, static_cast<uint32>(World.GenerationVersion));
    Hash = MixWaterHash(Hash, static_cast<uint32>(World.InitialChunkX));
    Hash = MixWaterHash(Hash, static_cast<uint32>(World.InitialChunkY));
    Hash = MixWaterHash(Hash, static_cast<uint32>(World.InitialCenterGridX));
    Hash = MixWaterHash(Hash, static_cast<uint32>(World.InitialCenterGridY));
    Hash = MixWaterHash(
        Hash,
        static_cast<uint32>(FMath::RoundToInt(World.InitialChunk.Elevation * 1000.0f)));

    // Facility positions alter the shared local terrain flattening envelope.
    // Water must rebuild with the same surface when construction or migration
    // moves that envelope, even if hydrology itself did not change.
    Hash = MixWaterHash(Hash, static_cast<uint32>(Civilization.Facilities.Num()));
    for (const FLLCoreCivilizationFacilityObservation& Facility : Civilization.Facilities)
    {
        const uint64 FacilityId = static_cast<uint64>(Facility.FacilityId);
        Hash = MixWaterHash(Hash, static_cast<uint32>(FacilityId & 0xFFFFFFFFu));
        Hash = MixWaterHash(Hash, static_cast<uint32>((FacilityId >> 32) & 0xFFFFFFFFu));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Facility.GridX));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Facility.GridY));
    }

    for (const FLLCoreSurfaceWaterPresentationObservation& Water : Observations)
    {
        const uint64 Id = static_cast<uint64>(Water.SurfaceWaterId);
        Hash = MixWaterHash(Hash, static_cast<uint32>(Id & 0xFFFFFFFFu));
        Hash = MixWaterHash(Hash, static_cast<uint32>((Id >> 32) & 0xFFFFFFFFu));
        Hash = MixWaterHash(Hash, Water.bAvailable ? 1u : 0u);
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.SurfaceKind));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.ChunkX));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.ChunkY));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.CenterGridX));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.CenterGridY));
        Hash = MixWaterHash(Hash, Water.bLinearChannel ? 1u : 0u);
        Hash = MixWaterHash(Hash, Water.bHasDownstreamTarget ? 1u : 0u);
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.DownstreamCenterGridX));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.DownstreamCenterGridY));
        Hash = MixWaterHash(Hash, Water.bHasMarineNeighbour ? 1u : 0u);
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.MarineCenterGridX));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.MarineCenterGridY));
        Hash = MixWaterHash(
            Hash,
            static_cast<uint32>(FMath::RoundToInt(
                FMath::Max(0.0f, Water.SuggestedChannelWidthCells) * 1000.0f)));
        Hash = MixWaterHash(
            Hash,
            static_cast<uint32>(FMath::RoundToInt(
                FMath::Max(0.0f, Water.SuggestedAreaRadiusCells) * 1000.0f)));
        Hash = MixWaterHash(
            Hash,
            static_cast<uint32>(FMath::RoundToInt(
                FMath::Clamp(Water.FlowPotential, 0.0f, 1.0f) * 1000.0f)));
    }
    return Hash;
}

void ConfigurePresentationOnlyWater(AWaterBody* WaterBody)
{
    if (!WaterBody)
    {
        return;
    }

    WaterBody->SetActorEnableCollision(false);
    if (UWaterBodyComponent* Component = WaterBody->GetWaterBodyComponent())
    {
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCanEverAffectNavigation(false);
        Component->SetGenerateOverlapEvents(false);

        // Water's landscape brush is presentation/editor machinery, not LifeLens
        // terrain authority. Use reflection so this stays tolerant of Water
        // plugin API churn while still disabling the reflected property when
        // present in UE 5.6.
        if (FBoolProperty* AffectsLandscape =
            FindFProperty<FBoolProperty>(
                Component->GetClass(),
                TEXT("bAffectsLandscape")))
        {
            AffectsLandscape->SetPropertyValue_InContainer(Component, false);
        }
    }
}

void ConfigureRiverMetadata(
    AWaterBodyRiver* River,
    float SuggestedWidthUU,
    float FlowPotential01)
{
    if (!River)
    {
        return;
    }

    UWaterSplineComponent* Spline = River->GetWaterSpline();
    UWaterSplineMetadata* Metadata = River->GetWaterSplineMetadata();
    if (!Spline || !Metadata)
    {
        return;
    }

    const int32 PointCount = Spline->GetNumberOfSplinePoints();
    const float HalfWidthUU = FMath::Max(35.0f, SuggestedWidthUU * 0.5f);
    const float Velocity = FMath::Lerp(
        20.0f,
        180.0f,
        FMath::Clamp(FlowPotential01, 0.0f, 1.0f));

    Metadata->RiverWidth.Points.Reset();
    Metadata->WaterVelocityScalar.Points.Reset();
    for (int32 Index = 0; Index < PointCount; ++Index)
    {
        Metadata->RiverWidth.AddPoint(static_cast<float>(Index), HalfWidthUU);
        Metadata->WaterVelocityScalar.AddPoint(
            static_cast<float>(Index),
            Velocity);
    }
}

void NotifyWaterShapeChanged(AWaterBody* WaterBody)
{
    if (!WaterBody)
    {
        return;
    }

    if (UWaterBodyComponent* Component = WaterBody->GetWaterBodyComponent())
    {
        // UE 5.6 exposes the Blueprint-compatible overload used here. This
        // requests mesh/zone refresh after runtime spline mutation.
        Component->OnWaterBodyChanged(true, false, false);
        Component->UpdateWaterZones(true);
    }
}
}

ALLWaterPresentationActor::ALLWaterPresentationActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.5f;
}

void ALLWaterPresentationActor::BeginPlay()
{
    Super::BeginPlay();
    EnsureWaterZone();
    RefreshFromCore(true);
}

void ALLWaterPresentationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    RefreshAccumulator += FMath::Max(0.0f, DeltaSeconds);
    if (RefreshAccumulator < RefreshIntervalSeconds)
    {
        return;
    }

    RefreshAccumulator = 0.0f;
    RefreshFromCore(false);
}

void ALLWaterPresentationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearProjectedWater();
    if (SpawnedWaterZone)
    {
        SpawnedWaterZone->Destroy();
        SpawnedWaterZone = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

int32 ALLWaterPresentationActor::ChunkCoordForGrid(int32 GridCoordinate) const
{
    const int32 Span = FMath::Max(1, LLWorldSpatialContract::ChunkSpanGridCells);
    int32 Quotient = GridCoordinate / Span;
    const int32 Remainder = GridCoordinate % Span;
    if (Remainder < 0)
    {
        --Quotient;
    }
    return Quotient;
}

float ALLWaterPresentationActor::WaterSurfaceZForGrid(
    ULLCoreBridgeSubsystem* Bridge,
    const FLLCoreWorldGenerationObservation& World,
    int32 GridX,
    int32 GridY,
    const TArray<FVector2D>& FacilityCentersUU) const
{
    if (!Bridge)
    {
        return WaterSurfaceZUU;
    }

    const int32 ChunkX = ChunkCoordForGrid(GridX);
    const int32 ChunkY = ChunkCoordForGrid(GridY);
    FLLCoreTerrainPresentationObservation Terrain;
    const bool bMaterializedTerrain =
        Bridge->GetTerrainPresentationObservation(
            ChunkX,
            ChunkY,
            Terrain)
        && Terrain.bAvailable;

    if (!bMaterializedTerrain
        && (!Bridge->GetTerrainPreviewObservation(
                ChunkX,
                ChunkY,
                Terrain)
            || !Terrain.bAvailable))
    {
        return WaterSurfaceZUU;
    }

    const FVector2D LocationUU(
        static_cast<float>(GridX - World.InitialCenterGridX)
            * LLWorldSpatialContract::GridCellSizeUU,
        static_cast<float>(GridY - World.InitialCenterGridY)
            * LLWorldSpatialContract::GridCellSizeUU);

    const float TerrainZUU = bMaterializedTerrain
        ? LLTerrainPresentationContract::LocalSurfaceZUU(
            World,
            Terrain,
            LocationUU,
            FVector2D::ZeroVector,
            FacilityCentersUU)
        : LLTerrainPresentationContract::RegionalSurfaceZUU(
            World,
            Terrain,
            LocationUU);

    return WaterSurfaceZUU + TerrainZUU;
}

FVector ALLWaterPresentationActor::GridToWorld(
    int32 GridX,
    int32 GridY,
    int32 InitialCenterGridX,
    int32 InitialCenterGridY,
    float SurfaceZUU) const
{
    return FVector(
        static_cast<float>(GridX - InitialCenterGridX)
            * LLWorldSpatialContract::GridCellSizeUU,
        static_cast<float>(GridY - InitialCenterGridY)
            * LLWorldSpatialContract::GridCellSizeUU,
        SurfaceZUU);
}

void ALLWaterPresentationActor::EnsureWaterZone()
{
    if (SpawnedWaterZone || !GetWorld())
    {
        return;
    }

    if (AWaterZone* Zone = GetWorld()->SpawnActor<AWaterZone>(
        AWaterZone::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator))
    {
        Zone->SetZoneExtent(FVector2D(
            FMath::Max(8000.0f, WaterZoneExtentUU),
            FMath::Max(8000.0f, WaterZoneExtentUU)));
        SpawnedWaterZone = Zone;
    }
}

void ALLWaterPresentationActor::ClearProjectedWater()
{
    for (AActor* Actor : SpawnedWaterActors)
    {
        if (IsValid(Actor))
        {
            Actor->Destroy();
        }
    }
    SpawnedWaterActors.Reset();
}

void ALLWaterPresentationActor::RefreshFromCore(bool bForce)
{
    UGameInstance* GameInstance = GetGameInstance();
    ULLCoreBridgeSubsystem* Bridge =
        GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;

    auto ClearStaleWaterProjection = [this]()
    {
        // Hydrology is a projection of Core authority. Once that authority is
        // absent, old water bodies must not survive as if the previous world
        // were still current.
        if (bBuiltOnce || SpawnedWaterActors.Num() > 0)
        {
            ClearProjectedWater();
        }
        bBuiltOnce = false;
        BuiltSignature = 0;
    };

    if (!Bridge || !Bridge->IsCoreRunning() || !GetWorld())
    {
        ClearStaleWaterProjection();
        return;
    }

    const FLLCoreWorldGenerationObservation World =
        Bridge->GetWorldGenerationObservation();
    if (!World.bAvailable || !World.bHasInitialStartRegion)
    {
        ClearStaleWaterProjection();
        return;
    }

    const TArray<FLLCoreSurfaceWaterPresentationObservation> Waters =
        Bridge->GetMaterializedSurfaceWaterPresentationObservations();
    const FLLCoreCivilizationWorldObservation Civilization =
        Bridge->GetCivilizationWorldObservation(0);
    TArray<FVector2D> FacilityCentersUU;
    FacilityCentersUU.Reserve(Civilization.Facilities.Num());
    for (const FLLCoreCivilizationFacilityObservation& Facility : Civilization.Facilities)
    {
        FacilityCentersUU.Add(FVector2D(
            static_cast<float>(Facility.GridX - World.InitialCenterGridX)
                * LLWorldSpatialContract::GridCellSizeUU,
            static_cast<float>(Facility.GridY - World.InitialCenterGridY)
                * LLWorldSpatialContract::GridCellSizeUU));
    }

    const uint32 Signature = SurfaceWaterSignature(
        World,
        Waters,
        Civilization);
    if (!bForce && bBuiltOnce && Signature == BuiltSignature)
    {
        return;
    }

    bBuiltOnce = true;
    BuiltSignature = Signature;
    ClearProjectedWater();
    EnsureWaterZone();

    int32 LinearCount = 0;
    int32 AreaCount = 0;
    int32 MarineCount = 0;

    for (const FLLCoreSurfaceWaterPresentationObservation& Water : Waters)
    {
        if (!Water.bAvailable)
        {
            continue;
        }

        const bool bMarine =
            Water.SurfaceKind == ELLCoreSurfaceWaterKind::Coast
            || Water.SurfaceKind == ELLCoreSurfaceWaterKind::Ocean;
        const float CenterWaterZUU = bMarine
            ? WaterSurfaceZUU
            : WaterSurfaceZForGrid(
                Bridge,
                World,
                Water.CenterGridX,
                Water.CenterGridY,
                FacilityCentersUU);
        const FVector Center = GridToWorld(
            Water.CenterGridX,
            Water.CenterGridY,
            World.InitialCenterGridX,
            World.InitialCenterGridY,
            CenterWaterZUU);

        if (Water.bLinearChannel && Water.bHasDownstreamTarget)
        {
            AWaterBodyRiver* River = GetWorld()->SpawnActor<AWaterBodyRiver>(
                AWaterBodyRiver::StaticClass(),
                FVector::ZeroVector,
                FRotator::ZeroRotator);
            if (!River)
            {
                continue;
            }

            ConfigurePresentationOnlyWater(River);
            if (UWaterSplineComponent* Spline = River->GetWaterSpline())
            {
                const float DownstreamWaterZUU = WaterSurfaceZForGrid(
                    Bridge,
                    World,
                    Water.DownstreamCenterGridX,
                    Water.DownstreamCenterGridY,
                    FacilityCentersUU);
                const FVector Downstream = GridToWorld(
                    Water.DownstreamCenterGridX,
                    Water.DownstreamCenterGridY,
                    World.InitialCenterGridX,
                    World.InitialCenterGridY,
                    DownstreamWaterZUU);
                Spline->ClearSplinePoints(false);
                Spline->AddSplinePoint(
                    Center,
                    ESplineCoordinateSpace::World,
                    false);
                Spline->AddSplinePoint(
                    Downstream,
                    ESplineCoordinateSpace::World,
                    false);
                Spline->SetClosedLoop(false, false);
                Spline->UpdateSpline();

                ConfigureRiverMetadata(
                    River,
                    Water.SuggestedChannelWidthCells
                        * LLWorldSpatialContract::GridCellSizeUU,
                    Water.FlowPotential);
                NotifyWaterShapeChanged(River);
                SpawnedWaterActors.Add(River);
                ++LinearCount;
            }
            else
            {
                River->Destroy();
            }
            continue;
        }

        if (bMarine)
        {
            // Local Surface presentation deliberately uses bounded lake-style
            // polygons for marine chunks. AWaterBodyOcean is reserved for the
            // later Planetary representation because its exclusion/shoreline
            // semantics can flood an unbounded local map when spawned per chunk.
            AWaterBodyLake* Marine = GetWorld()->SpawnActor<AWaterBodyLake>(
                AWaterBodyLake::StaticClass(),
                FVector::ZeroVector,
                FRotator::ZeroRotator);
            if (!Marine)
            {
                continue;
            }

            ConfigurePresentationOnlyWater(Marine);
            UWaterSplineComponent* Spline = Marine->GetWaterSpline();
            if (!Spline)
            {
                Marine->Destroy();
                continue;
            }

            const float HalfChunkUU =
                LLWorldSpatialContract::ChunkSpanUU * 0.515f;
            Spline->ClearSplinePoints(false);

            if (Water.SurfaceKind == ELLCoreSurfaceWaterKind::Ocean)
            {
                const FVector Corners[4] = {
                    Center + FVector(-HalfChunkUU, -HalfChunkUU, 0.0f),
                    Center + FVector( HalfChunkUU, -HalfChunkUU, 0.0f),
                    Center + FVector( HalfChunkUU,  HalfChunkUU, 0.0f),
                    Center + FVector(-HalfChunkUU,  HalfChunkUU, 0.0f)
                };
                for (const FVector& Corner : Corners)
                {
                    Spline->AddSplinePoint(
                        Corner,
                        ESplineCoordinateSpace::World,
                        false);
                }
            }
            else
            {
                if (!Water.bHasMarineNeighbour)
                {
                    Marine->Destroy();
                    continue;
                }

                const FVector MarineCenter = GridToWorld(
                    Water.MarineCenterGridX,
                    Water.MarineCenterGridY,
                    World.InitialCenterGridX,
                    World.InitialCenterGridY,
                    WaterSurfaceZUU);
                FVector2D Direction(
                    MarineCenter.X - Center.X,
                    MarineCenter.Y - Center.Y);
                if (!Direction.Normalize())
                {
                    Marine->Destroy();
                    continue;
                }

                const FVector2D Perpendicular(-Direction.Y, Direction.X);
                const FVector2D Center2D(Center.X, Center.Y);
                const FVector2D ShoreLine =
                    Center2D + Direction * (HalfChunkUU * 0.04f);
                const FVector2D MarineEdge =
                    Center2D + Direction * HalfChunkUU;

                const FVector2D Points[4] = {
                    ShoreLine + Perpendicular * HalfChunkUU,
                    MarineEdge + Perpendicular * HalfChunkUU,
                    MarineEdge - Perpendicular * HalfChunkUU,
                    ShoreLine - Perpendicular * HalfChunkUU
                };
                for (const FVector2D& Point : Points)
                {
                    Spline->AddSplinePoint(
                        FVector(Point.X, Point.Y, WaterSurfaceZUU),
                        ESplineCoordinateSpace::World,
                        false);
                }
            }

            Spline->SetClosedLoop(true, false);
            Spline->UpdateSpline();
            NotifyWaterShapeChanged(Marine);
            SpawnedWaterActors.Add(Marine);
            ++MarineCount;
            continue;
        }

        float RadiusCells = Water.SuggestedAreaRadiusCells;
        if (Water.SurfaceKind == ELLCoreSurfaceWaterKind::Spring)
        {
            RadiusCells = FMath::Max(
                0.65f,
                Water.SuggestedChannelWidthCells * 0.75f);
        }
        if (RadiusCells <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        AWaterBodyLake* Lake = GetWorld()->SpawnActor<AWaterBodyLake>(
            AWaterBodyLake::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator);
        if (!Lake)
        {
            continue;
        }

        ConfigurePresentationOnlyWater(Lake);
        if (UWaterSplineComponent* Spline = Lake->GetWaterSpline())
        {
            const float RadiusUU =
                RadiusCells * LLWorldSpatialContract::GridCellSizeUU;
            constexpr int32 PointCount = 8;
            Spline->ClearSplinePoints(false);
            for (int32 Index = 0; Index < PointCount; ++Index)
            {
                const float Angle =
                    2.0f * PI * static_cast<float>(Index)
                    / static_cast<float>(PointCount);
                Spline->AddSplinePoint(
                    Center + FVector(
                        FMath::Cos(Angle) * RadiusUU,
                        FMath::Sin(Angle) * RadiusUU,
                        0.0f),
                    ESplineCoordinateSpace::World,
                    false);
            }
            Spline->SetClosedLoop(true, false);
            Spline->UpdateSpline();
            NotifyWaterShapeChanged(Lake);
            SpawnedWaterActors.Add(Lake);
            ++AreaCount;
        }
        else
        {
            Lake->Destroy();
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("LifeLens Unreal Water projection: observations=%d linear=%d area=%d marineLocalSurface=%d"),
        Waters.Num(),
        LinearCount,
        AreaCount,
        MarineCount);
}
