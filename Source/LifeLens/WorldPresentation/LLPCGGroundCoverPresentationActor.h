#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LLPCGGroundCoverPresentationActor.generated.h"

class UActorComponent;
class USceneComponent;

/**
 * Desktop-only runtime consumer for the project-authored PCG ground-cover graph.
 *
 * The graph is decorative presentation only. It consumes the authoritative
 * initial chunk VisualSeed and never creates gameplay resources/facilities.
 * Android does not spawn this actor and the PCG plugin is excluded there.
 */
UCLASS(Config=Game, DefaultConfig)
class LIFELENS_API ALLPCGGroundCoverPresentationActor : public AActor
{
    GENERATED_BODY()

public:
    ALLPCGGroundCoverPresentationActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    void TryGenerateFromCore();

    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY()
    TObjectPtr<UActorComponent> RuntimePCGComponent;

    UPROPERTY()
    TObjectPtr<UObject> GroundCoverGraphAsset;

    // The current authored graph still references weed_plant_02, which is
    // visibly chalk-white in the observed desktop runtime. Keep the runtime
    // consumer available, but fail closed until the graph is regenerated with
    // an approved ground-cover mesh.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PCG")
    bool bEnableGroundCoverPCG = false;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PCG", meta=(ClampMin="0.1", ClampMax="10.0"))
    float RetryIntervalSeconds = 1.0f;

    float RetryAccumulator = 0.0f;
    int64 LastGeneratedVisualSeed = 0;
    int64 LastGeneratedWorldSeed = 0;
    int32 LastGeneratedGenerationVersion = 0;
    int32 LastGeneratedChunkX = 0;
    int32 LastGeneratedChunkY = 0;
    bool bGenerated = false;
};
