#include "WorldPresentation/LLWorldPresentationActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Simulation/LLCivilizationReadTypes.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLWorldGenerationReadTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "World/LLWorldSpatialContract.h"

namespace
{
    uint32 MixHash(uint32 Seed, uint32 Value)
    {
        Seed ^= Value + 0x9E3779B9u + (Seed << 6) + (Seed >> 2);
        return Seed;
    }

    uint32 ChunkHash(int64 WorldSeed, int32 GenerationVersion, int32 ChunkX, int32 ChunkY)
    {
        uint32 Hash = static_cast<uint32>(WorldSeed & 0xFFFFFFFF);
        Hash = MixHash(Hash, static_cast<uint32>((WorldSeed >> 32) & 0xFFFFFFFF));
        Hash = MixHash(Hash, static_cast<uint32>(GenerationVersion));
        Hash = MixHash(Hash, static_cast<uint32>(ChunkX));
        Hash = MixHash(Hash, static_cast<uint32>(ChunkY));
        return Hash;
    }

    float HashUnit(uint32& State)
    {
        State = MixHash(State, 0x2545F491u);
        return static_cast<float>(State % 1000000u) / 1000000.0f;
    }

    int32 ScaledCount(float Factor, int32 MaxCount)
    {
        return FMath::Clamp(FMath::RoundToInt(FMath::Clamp(Factor, 0.0f, 1.0f) * MaxCount), 0, MaxCount);
    }
}

ALLWorldPresentationActor::ALLWorldPresentationActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.5f;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GrassMatFinder(TEXT("/Game/Environment/Materials/MI_Ground_Grass.MI_Ground_Grass"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> DryMatFinder(TEXT("/Game/Environment/Materials/MI_Ground_DryEarth.MI_Ground_DryEarth"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TransitionMatFinder(TEXT("/Game/Environment/Materials/MI_Ground_Transition.MI_Ground_Transition"));

    GroundMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
    GroundGrass = GrassMatFinder.Succeeded() ? GrassMatFinder.Object : nullptr;
    GroundDry = DryMatFinder.Succeeded() ? DryMatFinder.Object : nullptr;
    GroundTransition = TransitionMatFinder.Succeeded() ? TransitionMatFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> TreeA(TEXT("/Game/Environment/Quaternius/StylizedNature/CommonTree_1/StaticMeshes/CommonTree_1.CommonTree_1"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> TreeB(TEXT("/Game/Environment/Quaternius/StylizedNature/Pine_1/StaticMeshes/Pine_1.Pine_1"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> TreeC(TEXT("/Game/Environment/Quaternius/StylizedNature/TwistedTree_2/StaticMeshes/TwistedTree_2.TwistedTree_2"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ShrubA(TEXT("/Game/Environment/Quaternius/StylizedNature/Bush_Common/StaticMeshes/Bush_Common.Bush_Common"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ShrubB(TEXT("/Game/Environment/Quaternius/StylizedNature/Fern_1/StaticMeshes/Fern_1.Fern_1"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> GrassA(TEXT("/Game/Environment/Quaternius/StylizedNature/Grass_Common_Tall/StaticMeshes/Grass_Common_Tall.Grass_Common_Tall"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> GrassB(TEXT("/Game/Environment/Quaternius/StylizedNature/Grass_Wispy_Tall/StaticMeshes/Grass_Wispy_Tall.Grass_Wispy_Tall"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RockA(TEXT("/Game/Environment/Quaternius/StylizedNature/Rock_Medium_1/StaticMeshes/Rock_Medium_1.Rock_Medium_1"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RockB(TEXT("/Game/Environment/Quaternius/StylizedNature/Pebble_Round_2/StaticMeshes/Pebble_Round_2.Pebble_Round_2"));

    if (TreeA.Succeeded()) { TreeMeshes.Add(TreeA.Object); }
    if (TreeB.Succeeded()) { TreeMeshes.Add(TreeB.Object); }
    if (TreeC.Succeeded()) { TreeMeshes.Add(TreeC.Object); }
    if (ShrubA.Succeeded()) { ShrubMeshes.Add(ShrubA.Object); }
    if (ShrubB.Succeeded()) { ShrubMeshes.Add(ShrubB.Object); }
    if (GrassA.Succeeded()) { GrassMeshes.Add(GrassA.Object); }
    if (GrassB.Succeeded()) { GrassMeshes.Add(GrassB.Object); }
    if (RockA.Succeeded()) { RockMeshes.Add(RockA.Object); }
    if (RockB.Succeeded()) { RockMeshes.Add(RockB.Object); }

    Ground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GeneratedGround"));
    Ground->SetupAttachment(Root);
    Ground->SetMobility(EComponentMobility::Movable);
    Ground->SetStaticMesh(GroundMesh);
    Ground->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Ground->SetCanEverAffectNavigation(false);

    for (int32 Index = 0; Index < TreeMeshes.Num(); ++Index)
    {
        TreeInstances.Add(AddInstancedComponent(*FString::Printf(TEXT("Trees_%d"), Index), TreeMeshes[Index], TreeCullStartUU, TreeCullEndUU, true));
    }
    for (int32 Index = 0; Index < ShrubMeshes.Num(); ++Index)
    {
        ShrubInstances.Add(AddInstancedComponent(*FString::Printf(TEXT("Shrubs_%d"), Index), ShrubMeshes[Index], SmallCullStartUU, SmallCullEndUU, false));
    }
    for (int32 Index = 0; Index < GrassMeshes.Num(); ++Index)
    {
        GrassInstances.Add(AddInstancedComponent(*FString::Printf(TEXT("Grass_%d"), Index), GrassMeshes[Index], SmallCullStartUU, SmallCullEndUU, false));
    }
    for (int32 Index = 0; Index < RockMeshes.Num(); ++Index)
    {
        RockInstances.Add(AddInstancedComponent(*FString::Printf(TEXT("Rocks_%d"), Index), RockMeshes[Index], SmallCullStartUU, TreeCullEndUU, false));
    }

    // Temporary low-cost primitive-storage composition. Core owns whether the
    // facility exists and where it is; these cube instances only visualize it.
    FacilityFoundationInstances = AddInstancedComponent(TEXT("FacilityFoundations"), GroundMesh, FacilityCullStartUU, FacilityCullEndUU, true);
    FacilityPostInstances = AddInstancedComponent(TEXT("FacilityPosts"), GroundMesh, FacilityCullStartUU, FacilityCullEndUU, true);
    FacilityRoofInstances = AddInstancedComponent(TEXT("FacilityRoofs"), GroundMesh, FacilityCullStartUU, FacilityCullEndUU, true);
    FacilityCargoInstances = AddInstancedComponent(TEXT("FacilityCargo"), GroundMesh, FacilityCullStartUU, FacilityCullEndUU, true);
}

UHierarchicalInstancedStaticMeshComponent* ALLWorldPresentationActor::AddInstancedComponent(
    const TCHAR* Name, UStaticMesh* Mesh, float CullStartUU, float CullEndUU, bool bCastShadow)
{
    UHierarchicalInstancedStaticMeshComponent* Component =
        CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(Name);
    Component->SetupAttachment(GetRootComponent());
    Component->SetStaticMesh(Mesh);
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCanEverAffectNavigation(false);
    Component->SetCastShadow(bCastShadow);
    Component->InstanceStartCullDistance = static_cast<int32>(CullStartUU);
    Component->InstanceEndCullDistance = static_cast<int32>(CullEndUU);
    return Component;
}

void ALLWorldPresentationActor::BeginPlay()
{
    Super::BeginPlay();
    RefreshFromCore(true);
}

void ALLWorldPresentationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    RefreshAccumulator += DeltaSeconds;
    if (RefreshAccumulator < RefreshIntervalSeconds)
    {
        return;
    }
    RefreshAccumulator = 0.0f;
    RefreshFromCore(false);
}

void ALLWorldPresentationActor::ClearInstances()
{
    for (UHierarchicalInstancedStaticMeshComponent* Component : TreeInstances) { if (Component) { Component->ClearInstances(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : ShrubInstances) { if (Component) { Component->ClearInstances(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : GrassInstances) { if (Component) { Component->ClearInstances(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : RockInstances) { if (Component) { Component->ClearInstances(); } }
    PlacedTrees = 0;
    PlacedShrubs = 0;
    PlacedGrass = 0;
    PlacedRocks = 0;
    SuppressedDressing = 0;
}

void ALLWorldPresentationActor::ClearFacilityInstances()
{
    if (FacilityFoundationInstances) { FacilityFoundationInstances->ClearInstances(); }
    if (FacilityPostInstances) { FacilityPostInstances->ClearInstances(); }
    if (FacilityRoofInstances) { FacilityRoofInstances->ClearInstances(); }
    if (FacilityCargoInstances) { FacilityCargoInstances->ClearInstances(); }
}

float ALLWorldPresentationActor::AmbientDressingKeepFactor(const FVector2D& LocationUU) const
{
    if (StartRegionClearRadiusUU <= 0.0f)
    {
        return 1.0f;
    }

    const float Distance = LocationUU.Size();
    if (Distance <= StartRegionClearRadiusUU)
    {
        return 0.0f;
    }
    if (StartRegionClearFalloffUU <= KINDA_SMALL_NUMBER)
    {
        return 1.0f;
    }
    return FMath::Clamp((Distance - StartRegionClearRadiusUU) / StartRegionClearFalloffUU, 0.0f, 1.0f);
}

FVector ALLWorldPresentationActor::ChunkOriginUU(const FLLCoreWorldGenerationObservation& World, int32 ChunkX, int32 ChunkY) const
{
    const float OffsetX = static_cast<float>(ChunkX - World.InitialChunkX) * LLWorldSpatialContract::ChunkSpanUU;
    const float OffsetY = static_cast<float>(ChunkY - World.InitialChunkY) * LLWorldSpatialContract::ChunkSpanUU;
    return FVector(OffsetX, OffsetY, 0.0f);
}

UMaterialInterface* ALLWorldPresentationActor::GroundMaterialForChunk(const FLLCoreNaturalChunkObservation& Chunk) const
{
    const FString Surface = Chunk.Surface.ToString().ToLower();
    const FString Biome = Chunk.Biome.ToString().ToLower();
    const bool bDry = Chunk.Moisture < 0.33f
        || Surface.Contains(TEXT("sand")) || Surface.Contains(TEXT("rock")) || Surface.Contains(TEXT("dirt"))
        || Biome.Contains(TEXT("desert")) || Biome.Contains(TEXT("arid"));
    const bool bLush = Chunk.Moisture > 0.6f && Chunk.FertilityPotential > 0.45f;

    if (bDry && GroundDry)
    {
        return GroundDry;
    }
    if (bLush && GroundGrass)
    {
        return GroundGrass;
    }
    return GroundTransition ? GroundTransition.Get() : GroundGrass.Get();
}

void ALLWorldPresentationActor::BuildGround(const FLLCoreWorldGenerationObservation& World)
{
    if (!Ground || !GroundMesh)
    {
        return;
    }

    const int32 Rings = FMath::Clamp(
        FMath::CeilToInt(FMath::Sqrt(static_cast<float>(FMath::Max(1, World.MaterializedChunkCount)))), 1, 9);
    const float SpanUU = LLWorldSpatialContract::ChunkSpanUU * (2.0f * Rings + 1.0f);
    const float Thickness = 20.0f;

    Ground->SetRelativeLocation(FVector(0.0f, 0.0f, -Thickness * 0.5f));
    Ground->SetRelativeScale3D(FVector(
        SpanUU / LLWorldSpatialContract::EngineCubeSideUU,
        SpanUU / LLWorldSpatialContract::EngineCubeSideUU,
        Thickness / LLWorldSpatialContract::EngineCubeSideUU));

    if (UMaterialInterface* Material = GroundMaterialForChunk(World.InitialChunk))
    {
        Ground->SetMaterial(0, Material);
    }
}

void ALLWorldPresentationActor::BuildChunkDressing(const FLLCoreWorldGenerationObservation& World, const FLLCoreNaturalChunkObservation& Chunk)
{
    if (!Chunk.bMaterialized)
    {
        return;
    }

    const FVector ChunkOrigin = ChunkOriginUU(World, Chunk.ChunkX, Chunk.ChunkY);
    const float HalfSpan = LLWorldSpatialContract::ChunkSpanUU * 0.5f;
    uint32 State = ChunkHash(World.WorldSeed, World.GenerationVersion, Chunk.ChunkX, Chunk.ChunkY);

    const float Fertility = FMath::Clamp(Chunk.FertilityPotential, 0.0f, 1.0f);
    const float Moisture = FMath::Clamp(Chunk.Moisture, 0.0f, 1.0f);
    const float Traversal = FMath::Clamp(Chunk.TraversalEase, 0.0f, 1.0f);

    const int32 TreeCount = ScaledCount(Fertility * Moisture, MaxTreesPerChunk);
    const int32 ShrubCount = ScaledCount(Fertility * 0.8f + Moisture * 0.2f, MaxShrubsPerChunk);
    const int32 GrassCount = ScaledCount(Fertility * 0.6f + Moisture * 0.4f, MaxGrassPerChunk);
    const int32 RockCount = ScaledCount((1.0f - Fertility) * 0.7f + (1.0f - Traversal) * 0.3f, MaxRocksPerChunk);

    auto Place = [&](TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& Components,
                     int32 Count, int32& Placed, int32 MaxTotal,
                     float MinScale, float MaxScale, float TiltDegrees,
                     bool bObeyReadabilityRadius)
    {
        if (Components.Num() == 0)
        {
            return;
        }
        for (int32 Index = 0; Index < Count && Placed < MaxTotal; ++Index)
        {
            const float X = (HashUnit(State) * 2.0f - 1.0f) * HalfSpan;
            const float Y = (HashUnit(State) * 2.0f - 1.0f) * HalfSpan;
            const float Yaw = HashUnit(State) * 360.0f;
            const float Scale = FMath::Lerp(MinScale, MaxScale, HashUnit(State));
            const float TiltPitch = (HashUnit(State) * 2.0f - 1.0f) * TiltDegrees;
            const float TiltRoll = (HashUnit(State) * 2.0f - 1.0f) * TiltDegrees;
            const float HeightJitter = FMath::Lerp(0.92f, 1.12f, HashUnit(State));

            const int32 Slot = static_cast<int32>(HashUnit(State) * Components.Num()) % Components.Num();
            const float KeepRoll = HashUnit(State);

            const FVector Location = ChunkOrigin + FVector(X, Y, 0.0f);
            if (bObeyReadabilityRadius
                && KeepRoll >= AmbientDressingKeepFactor(FVector2D(Location.X, Location.Y)))
            {
                ++SuppressedDressing;
                continue;
            }

            if (UHierarchicalInstancedStaticMeshComponent* Component = Components[Slot])
            {
                Component->AddInstance(FTransform(
                    FRotator(TiltPitch, Yaw, TiltRoll),
                    Location,
                    FVector(Scale, Scale, Scale * HeightJitter)));
                ++Placed;
            }
        }
    };

    Place(TreeInstances, TreeCount, PlacedTrees, MaxTreeInstances, 0.85f, 1.6f, 3.0f, true);
    Place(ShrubInstances, ShrubCount, PlacedShrubs, MaxShrubInstances, 0.7f, 1.5f, 5.0f, true);
    Place(GrassInstances, GrassCount, PlacedGrass, MaxGrassInstances, 0.7f, 1.7f, 4.0f, true);
    Place(RockInstances, RockCount, PlacedRocks, MaxRockInstances, 0.7f, 1.8f, 8.0f, false);

    for (const FLLCoreNaturalResourcePatchObservation& Patch : Chunk.ResourcePatches)
    {
        const FString Material = Patch.Material.ToString().ToLower();
        TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>* Target = nullptr;
        int32* PlacedCounter = nullptr;
        int32 MaxTotal = 0;
        float MinScale = 1.0f;
        float MaxScale = 1.3f;

        if (Material.Contains(TEXT("wood")) || Material.Contains(TEXT("timber")) || Material.Contains(TEXT("tree")))
        {
            Target = &TreeInstances;
            PlacedCounter = &PlacedTrees;
            MaxTotal = MaxTreeInstances;
            MinScale = 1.1f;
            MaxScale = 1.7f;
        }
        else if (Material.Contains(TEXT("stone")) || Material.Contains(TEXT("rock")) || Material.Contains(TEXT("flint")))
        {
            Target = &RockInstances;
            PlacedCounter = &PlacedRocks;
            MaxTotal = MaxRockInstances;
            MinScale = 1.0f;
            MaxScale = 1.9f;
        }
        else if (Material.Contains(TEXT("berry")) || Material.Contains(TEXT("plant"))
            || Material.Contains(TEXT("fiber")) || Material.Contains(TEXT("food")))
        {
            Target = &ShrubInstances;
            PlacedCounter = &PlacedShrubs;
            MaxTotal = MaxShrubInstances;
            MinScale = 0.9f;
            MaxScale = 1.5f;
        }

        if (!Target || Target->Num() == 0 || !PlacedCounter || *PlacedCounter >= MaxTotal)
        {
            continue;
        }

        const float PatchX = static_cast<float>(Patch.GridX - World.InitialCenterGridX) * LLWorldSpatialContract::GridCellSizeUU;
        const float PatchY = static_cast<float>(Patch.GridY - World.InitialCenterGridY) * LLWorldSpatialContract::GridCellSizeUU;

        const int32 PatchInstances = FMath::Clamp(
            FMath::RoundToInt(FMath::Clamp(Patch.VisualDensity, 0.0f, 1.0f) * 6.0f) + 1, 1, 8);
        for (int32 Index = 0; Index < PatchInstances && *PlacedCounter < MaxTotal; ++Index)
        {
            const float SpreadX = (HashUnit(State) * 2.0f - 1.0f) * LLWorldSpatialContract::GridCellSizeUU;
            const float SpreadY = (HashUnit(State) * 2.0f - 1.0f) * LLWorldSpatialContract::GridCellSizeUU;
            float Scale = FMath::Lerp(MinScale, MaxScale, HashUnit(State));
            const float Yaw = HashUnit(State) * 360.0f;
            const int32 Slot = static_cast<int32>(HashUnit(State) * Target->Num()) % Target->Num();

            const FVector2D PatchLocation(PatchX + SpreadX, PatchY + SpreadY);
            if (AmbientDressingKeepFactor(PatchLocation) <= 0.0f)
            {
                Scale *= StartRegionResourceScale;
            }

            if (UHierarchicalInstancedStaticMeshComponent* Component = (*Target)[Slot])
            {
                Component->AddInstance(FTransform(
                    FRotator(0.0f, Yaw, 0.0f),
                    FVector(PatchLocation.X, PatchLocation.Y, 0.0f),
                    FVector(Scale, Scale, Scale)));
                ++(*PlacedCounter);
            }
        }
    }
}

uint32 ALLWorldPresentationActor::FacilitySignature(const FLLCoreCivilizationWorldObservation& Civilization) const
{
    uint32 Hash = 0x46414331u;
    Hash = MixHash(Hash, static_cast<uint32>(Civilization.FacilityCount));
    for (const FLLCoreCivilizationFacilityObservation& Facility : Civilization.Facilities)
    {
        const uint64 Id = static_cast<uint64>(Facility.FacilityId);
        Hash = MixHash(Hash, static_cast<uint32>(Id & 0xFFFFFFFFu));
        Hash = MixHash(Hash, static_cast<uint32>((Id >> 32) & 0xFFFFFFFFu));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.Kind));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.State));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.GridX));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.GridY));
        Hash = MixHash(Hash, static_cast<uint32>(FMath::RoundToInt(Facility.WorkProgress * 1000.0f)));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.RequiredMaterialUnits));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.DeliveredMaterialUnits));
        Hash = MixHash(Hash, Facility.bActive ? 1u : 0u);
    }
    return Hash;
}

void ALLWorldPresentationActor::BuildFacilities(
    const FLLCoreWorldGenerationObservation& World,
    const FLLCoreCivilizationWorldObservation& Civilization)
{
    for (const FLLCoreCivilizationFacilityObservation& Facility : Civilization.Facilities)
    {
        if (Facility.Kind != ELLCoreFacilityKind::PrimitiveStorage
            || Facility.State == ELLCoreFacilityState::Ruined)
        {
            continue;
        }

        const float X = static_cast<float>(Facility.GridX - World.InitialCenterGridX) * LLWorldSpatialContract::GridCellSizeUU;
        const float Y = static_cast<float>(Facility.GridY - World.InitialCenterGridY) * LLWorldSpatialContract::GridCellSizeUU;
        const FVector Base(X, Y, 0.0f);

        const float MaterialProgress = Facility.RequiredMaterialUnits > 0
            ? FMath::Clamp(static_cast<float>(Facility.DeliveredMaterialUnits) / static_cast<float>(Facility.RequiredMaterialUnits), 0.0f, 1.0f)
            : 0.0f;
        const float WorkProgress = FMath::Clamp(Facility.WorkProgress, 0.0f, 1.0f);
        const bool bOperational = Facility.State == ELLCoreFacilityState::Operational && Facility.bActive;
        const float BuildProgress = bOperational ? 1.0f : FMath::Max(MaterialProgress, WorkProgress);

        if (FacilityFoundationInstances)
        {
            const float PlannedScale = Facility.State == ELLCoreFacilityState::Planned ? 0.72f : 1.0f;
            FacilityFoundationInstances->AddInstance(FTransform(
                FRotator::ZeroRotator,
                Base + FVector(0.0f, 0.0f, 6.0f),
                FVector(2.2f * PlannedScale, 1.6f * PlannedScale, 0.12f)));
        }

        const FVector2D PostOffsets[4] = {
            FVector2D(-90.0f, -60.0f), FVector2D(90.0f, -60.0f),
            FVector2D(-90.0f, 60.0f), FVector2D(90.0f, 60.0f)};
        const int32 PostCount = bOperational ? 4 : FMath::Clamp(FMath::CeilToInt(BuildProgress * 4.0f), 0, 4);
        for (int32 Index = 0; Index < PostCount; ++Index)
        {
            if (!FacilityPostInstances) { break; }
            const float HeightFactor = bOperational ? 1.0f : FMath::Clamp(0.35f + 0.65f * BuildProgress, 0.35f, 1.0f);
            FacilityPostInstances->AddInstance(FTransform(
                FRotator::ZeroRotator,
                Base + FVector(PostOffsets[Index].X, PostOffsets[Index].Y, 55.0f * HeightFactor),
                FVector(0.14f, 0.14f, 1.1f * HeightFactor)));
        }

        const int32 CargoCount = bOperational ? 6 : FMath::Clamp(FMath::CeilToInt(MaterialProgress * 6.0f), 0, 6);
        for (int32 Index = 0; Index < CargoCount; ++Index)
        {
            if (!FacilityCargoInstances) { break; }
            const int32 Column = Index % 3;
            const int32 Row = Index / 3;
            FacilityCargoInstances->AddInstance(FTransform(
                FRotator(0.0f, (Index % 2 == 0) ? 0.0f : 90.0f, 0.0f),
                Base + FVector(-65.0f + Column * 65.0f, -22.0f + Row * 48.0f, 25.0f),
                FVector(0.55f, 0.32f, 0.28f)));
        }

        if ((bOperational || WorkProgress >= 0.65f) && FacilityRoofInstances)
        {
            const float RoofScale = bOperational ? 1.0f : FMath::Clamp((WorkProgress - 0.65f) / 0.35f, 0.25f, 1.0f);
            FacilityRoofInstances->AddInstance(FTransform(
                FRotator::ZeroRotator,
                Base + FVector(0.0f, 0.0f, 118.0f),
                FVector(2.15f * RoofScale, 1.55f, 0.12f)));
        }
    }
}

void ALLWorldPresentationActor::RefreshFromCore(bool bForce)
{
    const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    ULLCoreBridgeSubsystem* Bridge = GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    if (!Bridge)
    {
        return;
    }

    const FLLCoreWorldGenerationObservation World = Bridge->GetWorldGenerationObservation();
    if (!World.bAvailable || !World.bHasInitialStartRegion)
    {
        return;
    }
    const FLLCoreCivilizationWorldObservation Civilization = Bridge->GetCivilizationWorldObservation(0);

    const bool bNaturalChanged = bForce
        || World.WorldSeed != BuiltWorldSeed
        || World.GenerationVersion != BuiltGenerationVersion
        || World.MaterializedChunkCount != BuiltChunkCount;
    const uint32 CurrentFacilitySignature = FacilitySignature(Civilization);
    const bool bFacilitiesChanged = bForce
        || !bBuiltFacilityPresentation
        || CurrentFacilitySignature != BuiltFacilitySignature;

    if (!bNaturalChanged && !bFacilitiesChanged)
    {
        return;
    }

    if (bNaturalChanged)
    {
        BuiltWorldSeed = World.WorldSeed;
        BuiltGenerationVersion = World.GenerationVersion;
        BuiltChunkCount = World.MaterializedChunkCount;

        ClearInstances();
        BuildGround(World);
        BuildChunkDressing(World, World.InitialChunk);

        const int32 Rings = FMath::Clamp(
            FMath::CeilToInt(FMath::Sqrt(static_cast<float>(FMath::Max(1, World.MaterializedChunkCount)))), 1, 4);
        for (int32 OffsetX = -Rings; OffsetX <= Rings; ++OffsetX)
        {
            for (int32 OffsetY = -Rings; OffsetY <= Rings; ++OffsetY)
            {
                if (OffsetX == 0 && OffsetY == 0)
                {
                    continue;
                }
                FLLCoreNaturalChunkObservation Neighbour;
                if (Bridge->GetNaturalChunkObservation(World.InitialChunkX + OffsetX, World.InitialChunkY + OffsetY, Neighbour)
                    && Neighbour.bMaterialized)
                {
                    BuildChunkDressing(World, Neighbour);
                }
            }
        }
    }

    if (bFacilitiesChanged)
    {
        BuiltFacilitySignature = CurrentFacilitySignature;
        bBuiltFacilityPresentation = true;
        ClearFacilityInstances();
        BuildFacilities(World, Civilization);
    }

    int32 TreeInstanceCount = 0;
    int32 ShrubInstanceCount = 0;
    int32 GrassInstanceCount = 0;
    int32 RockInstanceCount = 0;
    for (UHierarchicalInstancedStaticMeshComponent* Component : TreeInstances) { if (Component) { TreeInstanceCount += Component->GetInstanceCount(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : ShrubInstances) { if (Component) { ShrubInstanceCount += Component->GetInstanceCount(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : GrassInstances) { if (Component) { GrassInstanceCount += Component->GetInstanceCount(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : RockInstances) { if (Component) { RockInstanceCount += Component->GetInstanceCount(); } }

    const int32 FacilityInstanceCount =
        (FacilityFoundationInstances ? FacilityFoundationInstances->GetInstanceCount() : 0)
        + (FacilityPostInstances ? FacilityPostInstances->GetInstanceCount() : 0)
        + (FacilityRoofInstances ? FacilityRoofInstances->GetInstanceCount() : 0)
        + (FacilityCargoInstances ? FacilityCargoInstances->GetInstanceCount() : 0);

    UE_LOG(LogTemp, Log,
        TEXT("LLWorldPresentation seed=%lld gen=%d chunks=%d natural=%d/%d/%d/%d facilities=%d facilityInstances=%d cleared=%d radius=%.0f ground=%s"),
        World.WorldSeed, World.GenerationVersion, World.MaterializedChunkCount,
        TreeInstanceCount, ShrubInstanceCount, GrassInstanceCount, RockInstanceCount,
        Civilization.FacilityCount, FacilityInstanceCount,
        SuppressedDressing, StartRegionClearRadiusUU,
        (Ground && Ground->GetStaticMesh()) ? TEXT("yes") : TEXT("no"));
}
