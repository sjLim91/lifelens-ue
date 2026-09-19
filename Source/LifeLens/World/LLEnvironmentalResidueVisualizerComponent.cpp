#include "World/LLEnvironmentalResidueVisualizerComponent.h"

#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLEnvironmentReadTypes.h"
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
    const FLLCoreEnvironmentObservation& Environment) const
{
    uint32 Hash = GetTypeHash(Environment.TotalResidues);
    Hash = MixVisualHash(Hash, GetTypeHash(Environment.Residues.Num()));

    for (const FLLCoreEnvironmentalResidueObservation& Residue : Environment.Residues)
    {
        Hash = MixVisualHash(Hash, GetTypeHash(Residue.ResidueId));
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
    int32 GridX,
    int32 GridY,
    float CoreGridCellSizeUU,
    int32 CoreOriginGridX,
    int32 CoreOriginGridY) const
{
    const AActor* Owner = GetOwner();
    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
    FVector Location = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
    Location.X += static_cast<float>(GridX - CoreOriginGridX) * CellSize;
    Location.Y += static_cast<float>(GridY - CoreOriginGridY) * CellSize;

    UWorld* World = GetWorld();
    if (!World)
    {
        Location.Z += SurfaceOffsetUU;
        return Location;
    }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(LLEnvironmentResidueSurface), false, Owner);
    FHitResult Hit;
    const FVector TraceStart(Location.X, Location.Y, Location.Z + 2500.0f);
    const FVector TraceEnd(Location.X, Location.Y, Location.Z - 2500.0f);
    if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
    {
        Location.Z = Hit.ImpactPoint.Z + SurfaceOffsetUU;
    }
    else
    {
        Location.Z += SurfaceOffsetUU;
    }
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
    const uint32 Signature = BuildVisualSignature(Environment);
    if (!bForce && bHasVisualSignature && Signature == LastVisualSignature)
    {
        return;
    }

    LastVisualSignature = Signature;
    bHasVisualSignature = true;
    ClearInstances();

    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
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
            Residue.GridX, Residue.GridY, CellSize,
            CoreOriginGridX, CoreOriginGridY);
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
