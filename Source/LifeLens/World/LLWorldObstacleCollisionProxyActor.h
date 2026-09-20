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

// Query-only resident physical collision projected from authoritative Core
// materialized land and natural obstacle facts. This actor never inspects
// WorldPresentation instances; visual dressing cannot create physical truth.
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
    uint32 ComputeCoreSignature(
        const FLLCoreWorldGenerationObservation& World,
        const TArray<FLLCoreNaturalChunkObservation>& Chunks) const;
    void RebuildFromCore(
        const FLLCoreWorldGenerationObservation& World,
        const TArray<FLLCoreNaturalChunkObservation>& Chunks);
    void AddSurfaceProxy(
        const FLLCoreWorldGenerationObservation& World,
        const FLLCoreNaturalChunkObservation& Chunk);
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
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> SurfaceCollision;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CollisionCube;

    UPROPERTY(EditAnywhere, Category="LifeLens|ObstacleCollision", meta=(ClampMin="0.25"))
    float RefreshIntervalSeconds = 1.0f;

    float RefreshAccumulator = 0.0f;
    uint32 LastCoreSignature = 0;
    bool bHasBuilt = false;
};
