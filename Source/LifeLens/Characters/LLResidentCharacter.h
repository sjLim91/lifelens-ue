#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/LLTypes.h"
#include "LLResidentCharacter.generated.h"

class ULLResidentAppearanceComponent;
class ULLResidentMotionComponent;
class ULLResidentPresentationComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class LIFELENS_API ALLResidentCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ALLResidentCharacter();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Resident")
    void BindResident(const FLLResidentData& ResidentData);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Resident")
    void SetMovementTarget(const FVector& TargetLocation);

    // Presentation-side route from the authoritative current position to the
    // authoritative Core target. Waypoints affect locomotion only; they never
    // rewrite Core GridPos or action intent.
    void SetMovementPath(const TArray<FVector>& PathPoints, const FVector& FinalTarget);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Resident")
    void ClearMovementTarget();

    UFUNCTION(BlueprintPure, Category="LifeLens|Resident")
    bool HasReachedMovementTarget() const;

    bool IsMovingToward(const FVector& TargetLocation, float ToleranceUU = 1.0f) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Resident")
    FGuid GetResidentId() const { return ResidentId; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Resident")
    FText GetResidentDisplayName() const { return ResidentDisplayName; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Resident")
    ELLActionIntent GetCurrentIntent() const { return CurrentIntent; }

    UFUNCTION(BlueprintCallable, Category="LifeLens|Resident")
    void SetCurrentIntent(ELLActionIntent NewIntent) { CurrentIntent = NewIntent; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LifeLens|Presentation")
    TObjectPtr<ULLResidentAppearanceComponent> AppearanceComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LifeLens|Presentation")
    TObjectPtr<ULLResidentPresentationComponent> PresentationComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LifeLens|Presentation")
    TObjectPtr<ULLResidentMotionComponent> MotionComponent;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LifeLens|Resident")
    TObjectPtr<UStaticMeshComponent> DebugBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LifeLens|Resident")
    TObjectPtr<UTextRenderComponent> NameLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LifeLens|Resident")
    FGuid ResidentId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LifeLens|Resident")
    FText ResidentDisplayName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LifeLens|Resident")
    ELLActionIntent CurrentIntent = ELLActionIntent::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LifeLens|Movement")
    float RuntimeMoveSpeed = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LifeLens|Movement")
    float TargetAcceptanceRadius = 45.0f;

private:
    void RefreshLifecyclePresentation();

    FVector MovementTarget = FVector::ZeroVector;
    TArray<FVector> MovementWaypoints;
    int32 MovementWaypointIndex = 0;
    bool bHasMovementTarget = false;

    UPROPERTY(EditAnywhere, Category="LifeLens|Movement", meta=(ClampMin="1.0", ClampMax="100.0"))
    float PathWaypointAcceptanceRadius = 22.0f;

    // A high observer speed can provide enough movement budget to cross several
    // Core-grid waypoints in one render tick. Bound the amount of route work so
    // unused distance is not discarded, while a pathological hitch cannot skip
    // an unbounded number of swept collision segments.
    UPROPERTY(EditAnywhere, Category="LifeLens|Movement", meta=(ClampMin="1", ClampMax="32"))
    int32 MaxMovementSegmentsPerTick = 8;

    // Presentation-only lifecycle cache. The Core LifeStage remains the sole
    // source of stage truth; these values only prevent cumulative rescaling.
    bool bLifecyclePresentationInitialized = false;
    int32 LastLifecycleStageIndex = INDEX_NONE;
    float AdultCapsuleHalfHeight = 0.0f;
    float AdultCapsuleRadius = 0.0f;
    float AdultRuntimeMoveSpeed = 0.0f;
    FVector AdultBodyScale = FVector::OneVector;
};
