#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LLWorldObstacleCollisionProxyActor.generated.h"

class ALLWorldPresentationActor;
class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;

// Mirrors solid-looking generated dressing into cheap query-only collision.
//
// This is intentionally a UE presentation-path concern: Core still owns every
// action/target/world fact. The proxies only prevent the resident capsule from
// visually walking through tree trunks and non-trivial rocks while travelling
// toward an already-authoritative target.
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
    uint32 ComputeSourceSignature(const ALLWorldPresentationActor& Source) const;
    void RebuildFromSource(ALLWorldPresentationActor& Source);
    void AddTreeProxy(const FTransform& SourceTransform, UStaticMesh& SourceMesh);
    void AddRockProxy(const FTransform& SourceTransform, UStaticMesh& SourceMesh);

    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY()
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeCollision;

    UPROPERTY()
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RockCollision;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CollisionCube;

    UPROPERTY(EditAnywhere, Category="LifeLens|ObstacleCollision", meta=(ClampMin="0.05", ClampMax="0.5"))
    float TreeTrunkRadiusFraction = 0.18f;

    UPROPERTY(EditAnywhere, Category="LifeLens|ObstacleCollision", meta=(ClampMin="10.0"))
    float TreeMinRadiusUU = 30.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|ObstacleCollision", meta=(ClampMin="20.0"))
    float TreeMaxRadiusUU = 72.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|ObstacleCollision", meta=(ClampMin="0.1", ClampMax="1.0"))
    float TreeTrunkHalfHeightFraction = 0.55f;

    UPROPERTY(EditAnywhere, Category="LifeLens|ObstacleCollision", meta=(ClampMin="20.0"))
    float TreeMinHalfHeightUU = 90.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|ObstacleCollision", meta=(ClampMin="40.0"))
    float TreeMaxHalfHeightUU = 240.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|ObstacleCollision", meta=(ClampMin="0.1", ClampMax="1.0"))
    float RockFootprintFraction = 0.72f;

    UPROPERTY(EditAnywhere, Category="LifeLens|ObstacleCollision", meta=(ClampMin="10.0"))
    float MinimumBlockingRockHalfExtentUU = 36.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|ObstacleCollision", meta=(ClampMin="0.25"))
    float RefreshIntervalSeconds = 1.0f;

    float RefreshAccumulator = 0.0f;
    uint32 LastSourceSignature = 0;
    bool bHasBuilt = false;
};
