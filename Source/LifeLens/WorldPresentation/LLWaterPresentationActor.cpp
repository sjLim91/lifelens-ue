#include "WorldPresentation/LLWaterPresentationActor.h"

#include "Components/SplineComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
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

namespace
{
uint32 MixWaterHash(uint32 Seed, uint32 Value)
{
    Seed ^= Value + 0x9E3779B9u + (Seed << 6) + (Seed >> 2);
    return Seed;
}

uint32 SurfaceWaterSignature(
    const TArray<FLLCoreSurfaceWaterPresentationObservation>& Observations)
{
    uint32 Hash = 0x57415452u; // WATR
    for (const FLLCoreSurfaceWaterPresentationObservation& Water : Observations)
    {
        const uint64 Id = static_cast<uint64>(Water.SurfaceWaterId);
        Hash = MixWaterHash(Hash, static_cast<uint32>(Id & 0xFFFFFFFFu));
        Hash = MixWaterHash(Hash, static_cast<uint32>((Id >> 32) & 0xFFFFFFFFu));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.SurfaceKind));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.ChunkX));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.ChunkY));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.DownstreamCenterGridX));
        Hash = MixWaterHash(Hash, static_cast<uint32>(Water.DownstreamCenterGridY));
        Hash = MixWaterHash(
            Hash,
            static_cast<uint32>(FMath::RoundToInt(
                FMath::Max(0.0f, Water.SuggestedChannelWidthCells) * 1000.0f)));
        Hash = MixWaterHash(
            Hash,
            static_cast<uint32>(FMath::RoundToInt(
                FMath::Max(0.0f, Water.SuggestedAreaRadiusCells) * 1000.0f)));
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

FVector ALLWaterPresentationActor::GridToWorld(
    int32 GridX,
    int32 GridY,
    int32 InitialCenterGridX,
    int32 InitialCenterGridY) const
{
    return FVector(
        static_cast<float>(GridX - InitialCenterGridX)
            * LLWorldSpatialContract::GridCellSizeUU,
        static_cast<float>(GridY - InitialCenterGridY)
            * LLWorldSpatialContract::GridCellSizeUU,
        WaterSurfaceZUU);
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
    if (!Bridge || !Bridge->IsCoreRunning() || !GetWorld())
    {
        return;
    }

    const FLLCoreWorldGenerationObservation World =
        Bridge->GetWorldGenerationObservation();
    if (!World.bAvailable || !World.bHasInitialStartRegion)
    {
        return;
    }

    const TArray<FLLCoreSurfaceWaterPresentationObservation> Waters =
        Bridge->GetMaterializedSurfaceWaterPresentationObservations();
    const uint32 Signature = SurfaceWaterSignature(Waters);
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
    int32 DeferredMarineCount = 0;

    for (const FLLCoreSurfaceWaterPresentationObservation& Water : Waters)
    {
        if (!Water.bAvailable)
        {
            continue;
        }

        const FVector Center = GridToWorld(
            Water.CenterGridX,
            Water.CenterGridY,
            World.InitialCenterGridX,
            World.InitialCenterGridY);

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
                const FVector Downstream = GridToWorld(
                    Water.DownstreamCenterGridX,
                    Water.DownstreamCenterGridY,
                    World.InitialCenterGridX,
                    World.InitialCenterGridY);
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

        if (Water.SurfaceKind == ELLCoreSurfaceWaterKind::Coast
            || Water.SurfaceKind == ELLCoreSurfaceWaterKind::Ocean)
        {
            // A runtime Ocean spline describes shoreline/exclusion semantics,
            // not merely a local water disc. Defer until the planetary coastline
            // contract can place it without accidentally flooding the local map.
            ++DeferredMarineCount;
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
        TEXT("LifeLens Unreal Water projection: observations=%d linear=%d area=%d marineDeferred=%d"),
        Waters.Num(),
        LinearCount,
        AreaCount,
        DeferredMarineCount);
}
