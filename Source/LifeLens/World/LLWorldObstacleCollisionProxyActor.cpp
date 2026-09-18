#include "World/LLWorldObstacleCollisionProxyActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLWorldGenerationReadTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "World/LLWorldSpatialContract.h"

namespace
{
    constexpr float EngineCubeSideUU = 100.0f;

    void ConfigureCollisionProxy(UHierarchicalInstancedStaticMeshComponent& Component, UStaticMesh* CubeMesh)
    {
        Component.SetStaticMesh(CubeMesh);
        Component.SetMobility(EComponentMobility::Movable);
        Component.SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Component.SetCollisionObjectType(ECC_WorldStatic);
        Component.SetCollisionResponseToAllChannels(ECR_Ignore);
        Component.SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        Component.SetGenerateOverlapEvents(false);
        Component.SetCanEverAffectNavigation(false);
        Component.SetCastShadow(false);
        Component.SetHiddenInGame(true);
    }

    uint32 MixSignature(uint32 Seed, uint32 Value)
    {
        return HashCombineFast(Seed, Value);
    }
}

ALLWorldObstacleCollisionProxyActor::ALLWorldObstacleCollisionProxyActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.25f;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    CollisionCube = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;

    TreeCollision = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TreeCollisionProxies"));
    TreeCollision->SetupAttachment(SceneRoot);
    ConfigureCollisionProxy(*TreeCollision, CollisionCube);

    RockCollision = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RockCollisionProxies"));
    RockCollision->SetupAttachment(SceneRoot);
    ConfigureCollisionProxy(*RockCollision, CollisionCube);
}

void ALLWorldObstacleCollisionProxyActor::BeginPlay()
{
    Super::BeginPlay();
    RefreshCollisionProxies(true);
}

void ALLWorldObstacleCollisionProxyActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    RefreshAccumulator += FMath::Max(0.0f, DeltaSeconds);
    if (RefreshAccumulator < FMath::Max(0.25f, RefreshIntervalSeconds))
    {
        return;
    }

    RefreshAccumulator = 0.0f;
    RefreshCollisionProxies(false);
}

void ALLWorldObstacleCollisionProxyActor::RefreshCollisionProxies(bool bForce)
{
    const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    ULLCoreBridgeSubsystem* Bridge = GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    if (!Bridge || !CollisionCube || !TreeCollision || !RockCollision)
    {
        if (bHasBuilt && TreeCollision && RockCollision)
        {
            TreeCollision->ClearInstances();
            RockCollision->ClearInstances();
            bHasBuilt = false;
            LastCoreSignature = 0;
        }
        return;
    }

    const FLLCoreWorldGenerationObservation World = Bridge->GetWorldGenerationObservation();
    if (!World.bAvailable || !World.bHasInitialStartRegion)
    {
        if (bHasBuilt)
        {
            TreeCollision->ClearInstances();
            RockCollision->ClearInstances();
            bHasBuilt = false;
            LastCoreSignature = 0;
        }
        return;
    }

    const TArray<FLLCoreNaturalChunkObservation> Chunks =
        Bridge->GetMaterializedNaturalChunkObservations();
    const uint32 Signature = ComputeCoreSignature(World, Chunks);
    if (!bForce && bHasBuilt && Signature == LastCoreSignature)
    {
        return;
    }

    RebuildFromCore(World, Chunks);
    LastCoreSignature = Signature;
    bHasBuilt = true;
}

uint32 ALLWorldObstacleCollisionProxyActor::ComputeCoreSignature(
    const FLLCoreWorldGenerationObservation& World,
    const TArray<FLLCoreNaturalChunkObservation>& Chunks) const
{
    uint32 Signature = 0x4C4C4F43u; // "LLOC"
    Signature = MixSignature(Signature, GetTypeHash(World.WorldSeed));
    Signature = MixSignature(Signature, GetTypeHash(World.GenerationVersion));
    Signature = MixSignature(Signature, GetTypeHash(World.MaterializedChunkCount));

    for (const FLLCoreNaturalChunkObservation& Chunk : Chunks)
    {
        Signature = MixSignature(Signature, GetTypeHash(Chunk.ChunkX));
        Signature = MixSignature(Signature, GetTypeHash(Chunk.ChunkY));
        Signature = MixSignature(Signature, GetTypeHash(Chunk.PhysicalObstacles.Num()));
        for (const FLLCoreNaturalObstacleObservation& Obstacle : Chunk.PhysicalObstacles)
        {
            Signature = MixSignature(Signature, GetTypeHash(Obstacle.ObstacleId));
            Signature = MixSignature(Signature, static_cast<uint32>(Obstacle.Kind));
            Signature = MixSignature(Signature, GetTypeHash(Obstacle.GridX));
            Signature = MixSignature(Signature, GetTypeHash(Obstacle.GridY));
            Signature = MixSignature(Signature, GetTypeHash(FMath::RoundToInt(Obstacle.OffsetXCells * 1000.0f)));
            Signature = MixSignature(Signature, GetTypeHash(FMath::RoundToInt(Obstacle.OffsetYCells * 1000.0f)));
            Signature = MixSignature(Signature, GetTypeHash(FMath::RoundToInt(Obstacle.HalfExtentXCells * 1000.0f)));
            Signature = MixSignature(Signature, GetTypeHash(FMath::RoundToInt(Obstacle.HalfExtentYCells * 1000.0f)));
        }
    }
    return Signature;
}

void ALLWorldObstacleCollisionProxyActor::RebuildFromCore(
    const FLLCoreWorldGenerationObservation& World,
    const TArray<FLLCoreNaturalChunkObservation>& Chunks)
{
    TreeCollision->ClearInstances();
    RockCollision->ClearInstances();

    for (const FLLCoreNaturalChunkObservation& Chunk : Chunks)
    {
        for (const FLLCoreNaturalObstacleObservation& Obstacle : Chunk.PhysicalObstacles)
        {
            AddObstacleProxy(World, Obstacle);
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("LifeLens Core obstacle proxies: trees=%d rocks=%d chunks=%d"),
        TreeCollision->GetInstanceCount(),
        RockCollision->GetInstanceCount(),
        Chunks.Num());
}

void ALLWorldObstacleCollisionProxyActor::AddObstacleProxy(
    const FLLCoreWorldGenerationObservation& World,
    const FLLCoreNaturalObstacleObservation& Obstacle)
{
    if (Obstacle.ObstacleId <= 0
        || Obstacle.HalfExtentXCells <= 0.0f
        || Obstacle.HalfExtentYCells <= 0.0f
        || Obstacle.HalfHeightCells <= 0.0f)
    {
        return;
    }

    const float CellSize = LLWorldSpatialContract::GridCellSizeUU;
    const float RelativeXCells =
        static_cast<float>(Obstacle.GridX - World.InitialCenterGridX) + Obstacle.OffsetXCells;
    const float RelativeYCells =
        static_cast<float>(Obstacle.GridY - World.InitialCenterGridY) + Obstacle.OffsetYCells;
    const float HalfX = Obstacle.HalfExtentXCells * CellSize;
    const float HalfY = Obstacle.HalfExtentYCells * CellSize;
    const float HalfZ = Obstacle.HalfHeightCells * CellSize;

    const FVector WorldCenter = GetActorLocation() + FVector(
        RelativeXCells * CellSize,
        RelativeYCells * CellSize,
        HalfZ);
    const FVector ProxyScale(
        (HalfX * 2.0f) / EngineCubeSideUU,
        (HalfY * 2.0f) / EngineCubeSideUU,
        (HalfZ * 2.0f) / EngineCubeSideUU);

    UHierarchicalInstancedStaticMeshComponent* Target =
        Obstacle.Kind == ELLCoreNaturalObstacleKind::Rock ? RockCollision.Get() : TreeCollision.Get();
    if (Target)
    {
        Target->AddInstance(FTransform(FRotator::ZeroRotator, WorldCenter, ProxyScale), true);
    }
}
