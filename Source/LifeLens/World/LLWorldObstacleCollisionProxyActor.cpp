#include "World/LLWorldObstacleCollisionProxyActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "WorldPresentation/LLWorldPresentationActor.h"

namespace
{
    constexpr float EngineCubeSideUU = 100.0f;

    bool IsTreeSource(const UHierarchicalInstancedStaticMeshComponent& Component)
    {
        return Component.GetName().StartsWith(TEXT("Trees_"));
    }

    bool IsRockSource(const UHierarchicalInstancedStaticMeshComponent& Component)
    {
        return Component.GetName().StartsWith(TEXT("Rocks_"));
    }

    uint32 MixSignature(uint32 Seed, uint32 Value)
    {
        return HashCombineFast(Seed, Value);
    }

    uint32 HashTransformSample(const FTransform& Transform)
    {
        const FVector Location = Transform.GetLocation();
        const FVector Scale = Transform.GetScale3D();
        uint32 Hash = GetTypeHash(FMath::RoundToInt(Location.X));
        Hash = MixSignature(Hash, GetTypeHash(FMath::RoundToInt(Location.Y)));
        Hash = MixSignature(Hash, GetTypeHash(FMath::RoundToInt(Location.Z)));
        Hash = MixSignature(Hash, GetTypeHash(FMath::RoundToInt(Scale.X * 100.0f)));
        Hash = MixSignature(Hash, GetTypeHash(FMath::RoundToInt(Scale.Y * 100.0f)));
        Hash = MixSignature(Hash, GetTypeHash(FMath::RoundToInt(Scale.Z * 100.0f)));
        return Hash;
    }

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
    if (!GetWorld() || !CollisionCube || !TreeCollision || !RockCollision)
    {
        return;
    }

    ALLWorldPresentationActor* Source = nullptr;
    for (TActorIterator<ALLWorldPresentationActor> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It))
        {
            Source = *It;
            break;
        }
    }

    if (!Source)
    {
        if (bHasBuilt)
        {
            TreeCollision->ClearInstances();
            RockCollision->ClearInstances();
            bHasBuilt = false;
            LastSourceSignature = 0;
        }
        return;
    }

    const uint32 Signature = ComputeSourceSignature(*Source);
    if (!bForce && bHasBuilt && Signature == LastSourceSignature)
    {
        return;
    }

    RebuildFromSource(*Source);
    LastSourceSignature = Signature;
    bHasBuilt = true;
}

uint32 ALLWorldObstacleCollisionProxyActor::ComputeSourceSignature(ALLWorldPresentationActor& Source) const
{
    uint32 Signature = 0x4C4C4F42u; // "LLOB"
    TInlineComponentArray<UHierarchicalInstancedStaticMeshComponent*> Components(&Source);

    for (const UHierarchicalInstancedStaticMeshComponent* Component : Components)
    {
        if (!Component || (!IsTreeSource(*Component) && !IsRockSource(*Component)))
        {
            continue;
        }

        const int32 Count = Component->GetInstanceCount();
        Signature = MixSignature(Signature, GetTypeHash(Component->GetFName()));
        Signature = MixSignature(Signature, GetTypeHash(Count));

        if (Count <= 0)
        {
            continue;
        }

        const int32 SampleIndices[3] = {0, Count / 2, Count - 1};
        for (const int32 SampleIndex : SampleIndices)
        {
            FTransform Transform;
            if (Component->GetInstanceTransform(SampleIndex, Transform, true))
            {
                Signature = MixSignature(Signature, HashTransformSample(Transform));
            }
        }
    }

    return Signature;
}

void ALLWorldObstacleCollisionProxyActor::RebuildFromSource(ALLWorldPresentationActor& Source)
{
    TreeCollision->ClearInstances();
    RockCollision->ClearInstances();

    TInlineComponentArray<UHierarchicalInstancedStaticMeshComponent*> Components(&Source);
    for (UHierarchicalInstancedStaticMeshComponent* Component : Components)
    {
        if (!Component)
        {
            continue;
        }

        const bool bTree = IsTreeSource(*Component);
        const bool bRock = IsRockSource(*Component);
        UStaticMesh* SourceMesh = Component->GetStaticMesh();
        if ((!bTree && !bRock) || !SourceMesh)
        {
            continue;
        }

        const int32 InstanceCount = Component->GetInstanceCount();
        for (int32 InstanceIndex = 0; InstanceIndex < InstanceCount; ++InstanceIndex)
        {
            FTransform SourceTransform;
            if (!Component->GetInstanceTransform(InstanceIndex, SourceTransform, true))
            {
                continue;
            }

            if (bTree)
            {
                AddTreeProxy(SourceTransform, *SourceMesh);
            }
            else
            {
                AddRockProxy(SourceTransform, *SourceMesh);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("LifeLens obstacle proxies: trees=%d rocks=%d"),
        TreeCollision->GetInstanceCount(), RockCollision->GetInstanceCount());
}

void ALLWorldObstacleCollisionProxyActor::AddTreeProxy(const FTransform& SourceTransform, UStaticMesh& SourceMesh)
{
    const FBoxSphereBounds Bounds = SourceMesh.GetBounds();
    const FVector SourceScale = SourceTransform.GetScale3D();
    const FVector AbsScale(FMath::Abs(SourceScale.X), FMath::Abs(SourceScale.Y), FMath::Abs(SourceScale.Z));
    const FVector VisualHalfExtent(
        Bounds.BoxExtent.X * AbsScale.X,
        Bounds.BoxExtent.Y * AbsScale.Y,
        Bounds.BoxExtent.Z * AbsScale.Z);

    const float VisualRadius = FMath::Min(VisualHalfExtent.X, VisualHalfExtent.Y);
    const float Radius = FMath::Clamp(
        VisualRadius * TreeTrunkRadiusFraction,
        TreeMinRadiusUU,
        TreeMaxRadiusUU);
    const float HalfHeight = FMath::Clamp(
        VisualHalfExtent.Z * TreeTrunkHalfHeightFraction,
        TreeMinHalfHeightUU,
        TreeMaxHalfHeightUU);

    FVector LocalCenter = Bounds.Origin;
    LocalCenter.Z -= Bounds.BoxExtent.Z * 0.30f;
    const FVector WorldCenter = SourceTransform.TransformPosition(LocalCenter);
    const FRotator YawOnly(0.0f, SourceTransform.Rotator().Yaw, 0.0f);
    const FVector ProxyScale(
        (Radius * 2.0f) / EngineCubeSideUU,
        (Radius * 2.0f) / EngineCubeSideUU,
        (HalfHeight * 2.0f) / EngineCubeSideUU);

    TreeCollision->AddInstance(FTransform(YawOnly, WorldCenter, ProxyScale), true);
}

void ALLWorldObstacleCollisionProxyActor::AddRockProxy(const FTransform& SourceTransform, UStaticMesh& SourceMesh)
{
    // Decorative pebble meshes are deliberately traversable even when a random
    // presentation scale makes one instance appear larger. Only true rock/
    // boulder meshes should participate in resident blocking.
    if (SourceMesh.GetName().Contains(TEXT("Pebble"), ESearchCase::IgnoreCase))
    {
        return;
    }

    const FBoxSphereBounds Bounds = SourceMesh.GetBounds();
    const FVector SourceScale = SourceTransform.GetScale3D();
    const FVector AbsScale(FMath::Abs(SourceScale.X), FMath::Abs(SourceScale.Y), FMath::Abs(SourceScale.Z));
    const FVector VisualHalfExtent(
        Bounds.BoxExtent.X * AbsScale.X,
        Bounds.BoxExtent.Y * AbsScale.Y,
        Bounds.BoxExtent.Z * AbsScale.Z);

    const float HalfX = VisualHalfExtent.X * RockFootprintFraction;
    const float HalfY = VisualHalfExtent.Y * RockFootprintFraction;
    if (FMath::Max(HalfX, HalfY) < MinimumBlockingRockHalfExtentUU)
    {
        return; // other genuinely tiny rock meshes also stay traversable
    }

    const float HalfZ = FMath::Max(24.0f, VisualHalfExtent.Z * RockFootprintFraction);
    const FVector WorldCenter = SourceTransform.TransformPosition(Bounds.Origin);
    const FVector ProxyScale(
        (HalfX * 2.0f) / EngineCubeSideUU,
        (HalfY * 2.0f) / EngineCubeSideUU,
        (HalfZ * 2.0f) / EngineCubeSideUU);

    RockCollision->AddInstance(FTransform(SourceTransform.Rotator(), WorldCenter, ProxyScale), true);
}
