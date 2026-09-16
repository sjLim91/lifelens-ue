#include "WorldPresentation/LLWorldPresentationActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLWorldGenerationReadTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "World/LLWorldSpatialContract.h"

namespace
{
    // Deterministic hashing: the presentation must reproduce the same dressing
    // for the same world, so nothing here may use FMath::Rand or world time.
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

    // [0,1) from a hash stream.
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

    // Catalogue paths follow Content/Environment/PROVENANCE.md.
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
    // Collision stays with the authoritative bootstrap ground; this is visual.
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
    // Android budget: distant natural dressing is culled rather than drawn.
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
    SightlineCleared = 0;
}

FVector2D ALLWorldPresentationActor::SettlementReferenceUU(const FLLCoreWorldGenerationObservation& World) const
{
    // Resource patches are converted with
    // `(Patch.GridX - World.InitialCenterGridX) * GridCellSizeUU`, so the Core
    // start-region centre maps to the presentation origin by construction.
    // Deriving it here keeps the readability envelope tied to
    // `InitialCenterGrid` rather than to a hard-coded world origin: if the
    // mapping ever moves, this moves with it.
    const FVector ChunkOffset = ChunkOriginUU(World, World.InitialChunkX, World.InitialChunkY);
    return FVector2D(ChunkOffset.X, ChunkOffset.Y);
}

float ALLWorldPresentationActor::AmbientDressingKeepFactor(const FVector2D& LocationUU, ELLDressingLayer Layer) const
{
    // Ground detail is low enough that it never hides a resident.
    if (Layer == ELLDressingLayer::GroundDetail)
    {
        return 1.0f;
    }

    const float CoreRadius = FMath::Max(0.0f, CoreClearRadiusUU);
    const float ActivityRadius = FMath::Max(CoreRadius, ActivityRadiusUU);
    const float Distance = (LocationUU - CachedSettlementReferenceUU).Size();

    if (Distance >= ActivityRadius || ActivityRadius <= KINDA_SMALL_NUMBER)
    {
        return 1.0f;   // untouched natural density outside the envelope
    }

    const bool bCanopy = Layer == ELLDressingLayer::Canopy;
    const float CoreKeep = FMath::Clamp(bCanopy ? CoreZoneCanopyKeep : CoreZoneUndergrowthKeep, 0.0f, 1.0f);
    if (Distance <= CoreRadius)
    {
        return CoreKeep;   // living core stays open
    }

    // Activity zone: restore density with distance. Canopy uses the steeper
    // exponent so tall trees come back last.
    const float Band = FMath::Max(ActivityRadius - CoreRadius, KINDA_SMALL_NUMBER);
    const float Progress = FMath::Clamp((Distance - CoreRadius) / Band, 0.0f, 1.0f);
    const float Exponent = FMath::Max(1.0f, bCanopy ? CanopyRecoveryExponent : UndergrowthRecoveryExponent);
    return FMath::Lerp(CoreKeep, 1.0f, FMath::Pow(Progress, Exponent));
}

bool ALLWorldPresentationActor::CaptureInitialViewOrigin()
{
    if (bInitialViewCaptured)
    {
        return true;
    }
    const UWorld* World = GetWorld();
    const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
    const APlayerCameraManager* CameraManager = Controller ? Controller->PlayerCameraManager : nullptr;
    if (!CameraManager)
    {
        return false;   // the game mode has not placed the observer camera yet
    }

    // Read only. The observer camera pose and its Config tuning belong to
    // another lane; this never writes to either.
    const FVector CameraLocation = CameraManager->GetCameraLocation();
    if (CameraLocation.ContainsNaN())
    {
        return false;
    }

    InitialViewOriginUU = FVector2D(CameraLocation.X, CameraLocation.Y);
    bInitialViewCaptured = true;
    return true;
}

float ALLWorldPresentationActor::InitialSightlineKeepFactor(const FVector2D& LocationUU) const
{
    if (!bClearInitialSightlineCanopy || !bInitialViewCaptured)
    {
        return 1.0f;
    }

    const FVector2D Axis = CachedSettlementReferenceUU - InitialViewOriginUU;
    const float AxisLength = Axis.Size();
    if (AxisLength <= KINDA_SMALL_NUMBER)
    {
        return 1.0f;
    }
    const FVector2D AxisDirection = Axis / AxisLength;

    const FVector2D ToPoint = LocationUU - InitialViewOriginUU;
    const float Along = FVector2D::DotProduct(ToPoint, AxisDirection);
    // Only what stands between the camera and the settlement can occlude it.
    if (Along <= 0.0f || Along >= AxisLength)
    {
        return 1.0f;
    }

    const float Lateral = FMath::Abs(FVector2D::CrossProduct(ToPoint, AxisDirection));
    const float InnerHalfAngle = FMath::DegreesToRadians(FMath::Max(0.0f, InitialSightlineHalfAngleDegrees));
    const float OuterHalfAngle = InnerHalfAngle
        + FMath::DegreesToRadians(FMath::Max(0.0f, InitialSightlineEdgeFalloffDegrees));

    // Cone widens with distance from the camera, so the cleared wedge stays a
    // constant angular slice of the opening view.
    const float InnerWidth = Along * FMath::Tan(InnerHalfAngle);
    const float OuterWidth = Along * FMath::Tan(OuterHalfAngle);
    const float CentreKeep = FMath::Clamp(InitialSightlineCanopyKeep, 0.0f, 1.0f);

    if (Lateral <= InnerWidth)
    {
        return CentreKeep;
    }
    if (Lateral >= OuterWidth || OuterWidth - InnerWidth <= KINDA_SMALL_NUMBER)
    {
        return 1.0f;
    }
    const float EdgeProgress = (Lateral - InnerWidth) / (OuterWidth - InnerWidth);
    return FMath::Lerp(CentreKeep, 1.0f, EdgeProgress);
}

float ALLWorldPresentationActor::ResourcePatchScaleFactor(const FVector2D& LocationUU) const
{
    // An authoritative resource is never removed for readability. Inside the
    // settlement it is only drawn smaller.
    const float Distance = (LocationUU - CachedSettlementReferenceUU).Size();
    if (Distance <= FMath::Max(0.0f, CoreClearRadiusUU))
    {
        return FMath::Clamp(CoreZoneResourceScale, 0.1f, 1.0f);
    }
    if (Distance <= FMath::Max(CoreClearRadiusUU, ActivityRadiusUU))
    {
        return FMath::Clamp(ActivityZoneResourceScale, 0.1f, 1.0f);
    }
    return 1.0f;
}

FVector ALLWorldPresentationActor::ChunkOriginUU(const FLLCoreWorldGenerationObservation& World, int32 ChunkX, int32 ChunkY) const
{
    // The selected start chunk sits at the Unreal presentation origin, so every
    // other chunk is offset by whole chunk spans from it.
    const float OffsetX = static_cast<float>(ChunkX - World.InitialChunkX) * LLWorldSpatialContract::ChunkSpanUU;
    const float OffsetY = static_cast<float>(ChunkY - World.InitialChunkY) * LLWorldSpatialContract::ChunkSpanUU;
    return FVector(OffsetX, OffsetY, 0.0f);
}

UMaterialInterface* ALLWorldPresentationActor::GroundMaterialForChunk(const FLLCoreNaturalChunkObservation& Chunk) const
{
    // Surface/biome are authoritative facts; the material only reflects them.
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

    // Cover the materialized region: the start chunk plus a ring for every
    // additional materialized chunk, so the visible ground never ends inside
    // the area residents can reach.
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

    // Density follows the chunk's own authoritative facts.
    const float Fertility = FMath::Clamp(Chunk.FertilityPotential, 0.0f, 1.0f);
    const float Moisture = FMath::Clamp(Chunk.Moisture, 0.0f, 1.0f);
    const float Traversal = FMath::Clamp(Chunk.TraversalEase, 0.0f, 1.0f);

    const int32 TreeCount = ScaledCount(Fertility * Moisture, MaxTreesPerChunk);
    const int32 ShrubCount = ScaledCount(Fertility * 0.8f + Moisture * 0.2f, MaxShrubsPerChunk);
    const int32 GrassCount = ScaledCount(Fertility * 0.6f + Moisture * 0.4f, MaxGrassPerChunk);
    // Rocky ground is what is left when fertility is low or traversal is hard.
    const int32 RockCount = ScaledCount((1.0f - Fertility) * 0.7f + (1.0f - Traversal) * 0.3f, MaxRocksPerChunk);

    auto Place = [&](TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& Components,
                     int32 Count, int32& Placed, int32 MaxTotal,
                     float MinScale, float MaxScale, float TiltDegrees,
                     ELLDressingLayer Layer)
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
            // Start-region readability: ambient dressing thins out towards the
            // founders. The roll comes from the same deterministic stream, so
            // the same world always drops the same instances.
            const FVector2D Location2D(Location.X, Location.Y);
            float KeepFactor = AmbientDressingKeepFactor(Location2D, Layer);
            if (Layer == ELLDressingLayer::Canopy)
            {
                // Only the canopy blocks the opening view; shrubs, grass and
                // rocks keep their normal density inside the cone.
                const float SightlineKeep = InitialSightlineKeepFactor(Location2D);
                if (SightlineKeep < KeepFactor)
                {
                    ++SightlineCleared;
                    KeepFactor = SightlineKeep;
                }
            }
            if (KeepRoll >= KeepFactor)
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

    // Canopy blocks the observer the most, undergrowth blocks at resident
    // height, ground detail does not block at all.
    Place(TreeInstances, TreeCount, PlacedTrees, MaxTreeInstances, 0.85f, 1.6f, 3.0f, ELLDressingLayer::Canopy);
    Place(ShrubInstances, ShrubCount, PlacedShrubs, MaxShrubInstances, 0.7f, 1.5f, 5.0f, ELLDressingLayer::Undergrowth);
    Place(GrassInstances, GrassCount, PlacedGrass, MaxGrassInstances, 0.7f, 1.7f, 4.0f, ELLDressingLayer::Undergrowth);
    Place(RockInstances, RockCount, PlacedRocks, MaxRockInstances, 0.7f, 1.8f, 8.0f, ELLDressingLayer::GroundDetail);

    // Resource patches are authoritative world facts with their own grid
    // position, so they are marked where Core says they are.
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

        // Patch grid coordinates are Core-global; the start chunk centre is the
        // Unreal presentation origin.
        const float PatchX = static_cast<float>(Patch.GridX - World.InitialCenterGridX) * LLWorldSpatialContract::GridCellSizeUU;
        const float PatchY = static_cast<float>(Patch.GridY - World.InitialCenterGridY) * LLWorldSpatialContract::GridCellSizeUU;

        // VisualDensity is a Core fact; more of it means a denser visible patch.
        const int32 PatchInstances = FMath::Clamp(
            FMath::RoundToInt(FMath::Clamp(Patch.VisualDensity, 0.0f, 1.0f) * 6.0f) + 1, 1, 8);
        for (int32 Index = 0; Index < PatchInstances && *PlacedCounter < MaxTotal; ++Index)
        {
            const float SpreadX = (HashUnit(State) * 2.0f - 1.0f) * LLWorldSpatialContract::GridCellSizeUU;
            const float SpreadY = (HashUnit(State) * 2.0f - 1.0f) * LLWorldSpatialContract::GridCellSizeUU;
            float Scale = FMath::Lerp(MinScale, MaxScale, HashUnit(State));
            const float Yaw = HashUnit(State) * 360.0f;
            const int32 Slot = static_cast<int32>(HashUnit(State) * Target->Num()) % Target->Num();

            // An authoritative resource is never hidden for readability; it is
            // only drawn smaller inside the settlement bands.
            const FVector2D PatchLocation(PatchX + SpreadX, PatchY + SpreadY);
            Scale *= ResourcePatchScaleFactor(PatchLocation);

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

    // Rebuild only when the authoritative generation identity or the
    // materialized chunk set actually changed.
    const bool bSightlinePending = bClearInitialSightlineCanopy && !bInitialViewCaptured;
    if (!bForce
        && !bSightlinePending
        && World.WorldSeed == BuiltWorldSeed
        && World.GenerationVersion == BuiltGenerationVersion
        && World.MaterializedChunkCount == BuiltChunkCount)
    {
        return;
    }

    BuiltWorldSeed = World.WorldSeed;
    BuiltGenerationVersion = World.GenerationVersion;
    BuiltChunkCount = World.MaterializedChunkCount;

    // Resolved once per rebuild: the readability envelope is measured from the
    // Core start-region centre, not from the world origin.
    CachedSettlementReferenceUU = SettlementReferenceUU(World);

    // The observer camera is spawned by the game mode, which may run after this
    // actor's BeginPlay. Capturing it here means the first build can miss it;
    // the next refresh picks it up and rebuilds once.
    CaptureInitialViewOrigin();

    ClearInstances();
    BuildGround(World);
    BuildChunkDressing(World, World.InitialChunk);

    // Neighbouring materialized chunks, when the observation exposes them.
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

    // Report what actually reached the render components, not just how many
    // placements were attempted: a component with no mesh silently drops them.
    int32 TreeInstanceCount = 0;
    int32 ShrubInstanceCount = 0;
    int32 GrassInstanceCount = 0;
    int32 RockInstanceCount = 0;
    for (UHierarchicalInstancedStaticMeshComponent* Component : TreeInstances) { if (Component) { TreeInstanceCount += Component->GetInstanceCount(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : ShrubInstances) { if (Component) { ShrubInstanceCount += Component->GetInstanceCount(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : GrassInstances) { if (Component) { GrassInstanceCount += Component->GetInstanceCount(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : RockInstances) { if (Component) { RockInstanceCount += Component->GetInstanceCount(); } }

    UE_LOG(LogTemp, Log,
        TEXT("LLWorldPresentation seed=%lld gen=%d chunks=%d meshes=%d/%d/%d/%d instances=%d/%d/%d/%d thinned=%d sightline=%d/%d core=%.0f activity=%.0f ground=%s"),
        World.WorldSeed, World.GenerationVersion, World.MaterializedChunkCount,
        TreeMeshes.Num(), ShrubMeshes.Num(), GrassMeshes.Num(), RockMeshes.Num(),
        TreeInstanceCount, ShrubInstanceCount, GrassInstanceCount, RockInstanceCount,
        SuppressedDressing, SightlineCleared, bInitialViewCaptured ? 1 : 0,
        CoreClearRadiusUU, ActivityRadiusUU,
        (Ground && Ground->GetStaticMesh()) ? TEXT("yes") : TEXT("no"));

}
