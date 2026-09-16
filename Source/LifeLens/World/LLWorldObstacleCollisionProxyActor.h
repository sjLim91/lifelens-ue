#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LLWorldObstacleCollisionProxyActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class ULLCoreBridgeSubsystem;
class USceneComponent;
class UStaticMesh;
struct FLLCoreNaturalChunkObservation;
struct FLLCoreNaturalObstacleObservation;
struct FLLCoreWorldGenerationObservation;

// Query-only resident collision projected from authoritative Core natural
// obstacle facts. This actor no longer inspects WorldPresentation instances;
// visual dressing may decorate the same area but cannot create blocking truth.
UCLASS()
class LIFELENS_API ALLWorldObstacleCollisionProxyActor : public AActor
{
    GENERATED_BODY()

public:
    ALLWorldObstacleCollisionProxyActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    void RefreshCollisionProxies(bool bForce);
    void CollectMaterializedChunks(
        ULLCoreBridgeSubsystem& Bridge,
        const FLLCoreWorldGenerationObservation& World,
        TArray<FLLCoreNaturalChunkObservation>& OutChunks) const;
    uint32 ComputeCoreSignature(
        const FLLCoreWorldGenerationObservation& World,
        const TArray<FLLCoreNaturalChunkObservation>& Chunks) const;
    void RebuildFromCore(
        const FLLCoreWorldGenerationObservation& World,
        const TArray<FLLCoreNaturalChunkObservation>& Chunks);
    void AddObstacleProxy(
        const FLLCoreWorldGenerationObservation& World,
        const FLLCoreNaturalObstacleObservation& Obstacle);

    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY()
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeCollision;

    UPROPERTY()
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RockCollision;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CollisionCube;

    UPROPERTY(EditAnywhere, Category="LifeLens|ObstacleCollision", meta=(ClampMin="0.25"))
    float RefreshIntervalSeconds = 1.0f;

    float RefreshAccumulator = 0.0f;
    uint32 LastCoreSignature = 0;
    bool bHasBuilt = false;
};
