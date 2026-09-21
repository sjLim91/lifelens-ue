#include "World/LLEnvironmentalResidueVisualizerComponent.h"

#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLCivilizationReadTypes.h"
#include "Simulation/LLEnvironmentReadTypes.h"
#include "Simulation/LLWorldGenerationReadTypes.h"
#include "World/LLWorldSpatialContract.h"
#include "WorldPresentation/LLTerrainPresentationContract.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float ResidueMarkerNativeDiameterUU = 100.0f;
constexpr int32 ResidueCustomDataFloats = 4;

uint32 MixVisualHash(uint32 Seed, uint32 Value)
{
    return HashCombine(Seed, Value);
}

int32 ChunkCoordForGrid(int32 GridCoordinate)
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
}

ULLEnvironmentalResidueVisualizerComponent::ULLEnvironmentalResidueVisualizerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetCastShadow(false);
    SetCanEverAffectNavigation(false);
    SetMobility(EComponentMobility::Movable);
    NumCustomDataFloats = ResidueCustomDataFloats;

    // Residue is a visible world consequence, not a collision/debug proxy.
    // A cylinder gives the patch a soft footprint and the authored dry-earth
    // material prevents the default Engine grey rectangle from leaking into
    // production when human-waste residue is present.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MarkerFinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ResidueMaterialFinder(
        TEXT("/Game/Environment/Materials/MI_Ground_DryEarth.MI_Ground_DryEarth"));
    if (MarkerFinder.Succeeded())
    {
        SetStaticMesh(MarkerFinder.Object);
    }
    if (ResidueMaterialFinder.Succeeded())
    {
        SetMaterial(0, ResidueMaterialFinder.Object);
    }
}

uint32 ULLEnvironmentalResidueVisualizerComponent::BuildVisualSignature(
    const FLLCoreEnvironmentObservation& Environment,
    const FLLCoreWorldGenerationObservation& World,
    const FLLCoreCivilizationWorldObservation& Civilization,
    float CoreGridCellSizeUU,
    int32 CoreOriginGridX,
    int32 CoreOriginGridY) const
{
    uint32 Hash = GetTypeHash(Environment.TotalResidues);
    Hash = MixVisualHash(Hash, GetTypeHash(Environment.Residues.Num()));

    // Terrain projection is deterministic from this world/start frame, while
    // facility positions alter the shared flattening envelope. Both therefore
    // belong in the visual transform signature, not only the residue records.
    const uint64 Seed = static_cast<uint64>(World.WorldSeed);
    Hash = MixVisualHash(Hash, static_cast<uint32>(Seed & 0xFFFFFFFFu));
    Hash = MixVisualHash(Hash, static_cast<uint32>((Seed >> 32) & 0xFFFFFFFFu));
    Hash = MixVisualHash(Hash, GetTypeHash(World.GenerationVersion));
    Hash = MixVisualHash(Hash, GetTypeHash(World.InitialChunkX));
    Hash = MixVisualHash(Hash, GetTypeHash(World.InitialChunkY));
    Hash = MixVisualHash(Hash, GetTypeHash(World.InitialCenterGridX));
    Hash = MixVisualHash(Hash, GetTypeHash(World.InitialCenterGridY));
    Hash = MixVisualHash(
        Hash,
        GetTypeHash(FMath::RoundToInt(World.InitialChunk.Elevation * 100000.0f)));

    Hash = MixVisualHash(Hash, GetTypeHash(Civilization.Facilities.Num()));
    for (const FLLCoreCivilizationFacilityObservation& Facility : Civilization.Facilities)
    {
        const uint64 FacilityId = static_cast<uint64>(Facility.FacilityId);
        Hash = MixVisualHash(Hash, static_cast<uint32>(FacilityId & 0xFFFFFFFFu));
        Hash = MixVisualHash(Hash, static_cast<uint32>((FacilityId >> 32) & 0xFFFFFFFFu));
        Hash = MixVisualHash(Hash, GetTypeHash(Facility.GridX));
        Hash = MixVisualHash(Hash, GetTypeHash(Facility.GridY));
    }

    // Instance transforms are presentation outputs too. If world scale/owner
    // placement/surface lift changes, stale HISM transforms must rebuild.
    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
    Hash = MixVisualHash(Hash, GetTypeHash(FMath::RoundToInt(CellSize * 1000.0f)));
    Hash = MixVisualHash(Hash, GetTypeHash(CoreOriginGridX));
    Hash = MixVisualHash(Hash, GetTypeHash(CoreOriginGridY));
    Hash = MixVisualHash(Hash, GetTypeHash(FMath::RoundToInt(SurfaceOffsetUU * 1000.0f)));

    if (const AActor* Owner = GetOwner())
    {
        const FVector OwnerLocation = Owner->GetActorLocation();
        Hash = MixVisualHash(Hash, GetTypeHash(FMath::RoundToInt(OwnerLocation.X * 10.0f)));
        Hash = MixVisualHash(Hash, GetTypeHash(FMath::RoundToInt(OwnerLocation.Y * 10.0f)));
        Hash = MixVisualHash(Hash, GetTypeHash(FMath::RoundToInt(OwnerLocation.Z * 10.0f)));
    }

    for (const FLLCoreEnvironmentalResidueObservation& Residue : Environment.Residues)
    {
        Hash = MixVisualHash(Hash, GetTypeHash(Residue.ResidueId));
        Hash = MixVisualHash(Hash, GetTypeHash(static_cast<uint8>(Residue.Kind)));
        Hash = MixVisualHash(Hash, GetTypeHash(Residue.GridX));
        Hash = MixVisualHash(Hash, GetTypeHash(Residue.GridY));
        Hash = MixVisualHash(Hash, GetTypeHash(Residue.RadiusTiles));
        Hash = MixVisualHash(Hash, GetTypeHash(Residue.AgeMinutes));
        Hash = MixVisualHash(Hash, GetTypeHash(FMath::RoundToInt(Residue.Amount * 1000.0f)));
        Hash = MixVisualHash(Hash, GetTypeHash(FMath::RoundToInt(Residue.Intensity * 10000.0f)));
    }
    return Hash;
}

FVector ULLEnvironmentalResidueVisualizerComponent::ResolveSurfaceLocation(
    const ULLCoreBridgeSubsystem& CoreBridge,
    const FLLCoreWorldGenerationObservation& World,
    const TArray<FVector2D>& FacilityCentersUU,
    int32 GridX,
    int32 GridY,
    float CoreGridCellSizeUU,
    int32 CoreOriginGridX,
    int32 CoreOriginGridY) const
{
    const AActor* Owner = GetOwner();
    const FVector OwnerLocation =
        Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);

    FVector Location = OwnerLocation;
    Location.X += static_cast<float>(GridX - CoreOriginGridX) * CellSize;
    Location.Y += static_cast<float>(GridY - CoreOriginGridY) * CellSize;

    if (!World.bAvailable || !World.bHasInitialStartRegion)
    {
        Location.Z += SurfaceOffsetUU;
        return Location;
    }

    const int32 ChunkX = ChunkCoordForGrid(GridX);
    const int32 ChunkY = ChunkCoordForGrid(GridY);
    FLLCoreTerrainPresentationObservation Terrain;
    bool bHasTerrain =
        CoreBridge.GetTerrainPresentationObservation(
            ChunkX,
            ChunkY,
            Terrain)
        && Terrain.bAvailable;
    if (!bHasTerrain)
    {
        bHasTerrain =
            CoreBridge.GetTerrainPreviewObservation(
                ChunkX,
                ChunkY,
                Terrain)
            && Terrain.bAvailable;
    }

    if (!bHasTerrain)
    {
        Location.Z += SurfaceOffsetUU;
        return Location;
    }

    const FVector2D SurfaceLocationUU(
        static_cast<float>(GridX - World.InitialCenterGridX)
            * LLWorldSpatialContract::GridCellSizeUU,
        static_cast<float>(GridY - World.InitialCenterGridY)
            * LLWorldSpatialContract::GridCellSizeUU);
    const float SurfaceZUU =
        LLTerrainPresentationContract::LocalSurfaceZUU(
            World,
            Terrain,
            SurfaceLocationUU,
            FVector2D::ZeroVector,
            FacilityCentersUU);

    Location.Z = OwnerLocation.Z + SurfaceZUU + SurfaceOffsetUU;
    return Location;
}

void ULLEnvironmentalResidueVisualizerComponent::RefreshFromCore(
    const ULLCoreBridgeSubsystem& CoreBridge,
    float CoreGridCellSizeUU,
    bool bForce,
    int32 CoreOriginGridX,
    int32 CoreOriginGridY)
{
    SetCullDistances(
        FMath::Max(0, StartCullDistanceUU),
        FMath::Max(StartCullDistanceUU, EndCullDistanceUU));

    const FLLCoreEnvironmentObservation Environment =
        CoreBridge.GetEnvironmentObservation(FMath::Max(1, MaxResidueInstances));
    const FLLCoreWorldGenerationObservation World =
        CoreBridge.GetWorldGenerationObservation();
    const FLLCoreCivilizationWorldObservation Civilization =
        CoreBridge.GetCivilizationWorldObservation(0);

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

    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
    const uint32 Signature = BuildVisualSignature(
        Environment,
        World,
        Civilization,
        CellSize,
        CoreOriginGridX,
        CoreOriginGridY);
    if (!bForce && bHasVisualSignature && Signature == LastVisualSignature)
    {
        return;
    }

    LastVisualSignature = Signature;
    bHasVisualSignature = true;
    ClearInstances();
    for (const FLLCoreEnvironmentalResidueObservation& Residue : Environment.Residues)
    {
        if (Residue.Kind != ELLCoreEnvironmentalResidueKind::HumanWaste)
        {
            continue;
        }

        const float Intensity = FMath::Clamp(Residue.Intensity, 0.0f, 1.0f);
        const float Amount = FMath::Max(0.0f, Residue.Amount);
        const float Radius = static_cast<float>(FMath::Clamp(Residue.RadiusTiles, 1, 16));
        const float AmountNorm = FMath::Clamp(
            FMath::Log2(1.0f + Amount) / FMath::Log2(101.0f), 0.0f, 1.0f);
        const float RadiusNorm = Radius / 16.0f;
        const float AgeNorm = FMath::Clamp(
            static_cast<float>(FMath::Max(0, Residue.AgeMinutes)) / 1440.0f,
            0.0f,
            1.0f);

        // Footprint is intentionally smaller than the exposure radius.  Radius,
        // intensity and accumulated amount still make open waste visually larger
        // than a contained DugPit record without pretending the waste disappeared.
        const float FootprintCells = FMath::Clamp(0.30f + Radius * 0.18f, 0.45f, 1.40f);
        const float AmountScale = 0.90f + AmountNorm * 0.35f;
        const float IntensityScale = 0.78f + Intensity * 0.22f;
        const float DiameterUU = CellSize * FootprintCells * AmountScale * IntensityScale;
        const float XYScale = FMath::Max(0.04f, DiameterUU / ResidueMarkerNativeDiameterUU);
        const float ZScale = FMath::Lerp(0.010f, 0.035f, Intensity);

        const FVector WorldLocation = ResolveSurfaceLocation(
            CoreBridge,
            World,
            FacilityCentersUU,
            Residue.GridX,
            Residue.GridY,
            CellSize,
            CoreOriginGridX,
            CoreOriginGridY);
        const FTransform InstanceTransform(
            FRotator::ZeroRotator,
            WorldLocation,
            FVector(XYScale, XYScale, ZScale));

        const int32 InstanceIndex = AddInstance(InstanceTransform, true);
        if (InstanceIndex == INDEX_NONE)
        {
            continue;
        }

        SetCustomDataValue(InstanceIndex, 0, Intensity, false);
        SetCustomDataValue(InstanceIndex, 1, AmountNorm, false);
        SetCustomDataValue(InstanceIndex, 2, RadiusNorm, false);
        SetCustomDataValue(InstanceIndex, 3, AgeNorm, false);
    }

    MarkRenderStateDirty();
}
