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

    UFUNCTION(BlueprintCallable, Category="LifeLens|Resident")
    void ClearMovementTarget();

    UFUNCTION(BlueprintPure, Category="LifeLens|Resident")
    bool HasReachedMovementTarget() const;

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
    bool bHasMovementTarget = false;

    // Presentation-only lifecycle cache. The Core LifeStage remains the sole
    // source of stage truth; these values only prevent cumulative rescaling.
    bool bLifecyclePresentationInitialized = false;
    int32 LastLifecycleStageIndex = INDEX_NONE;
    float AdultCapsuleHalfHeight = 0.0f;
    float AdultCapsuleRadius = 0.0f;
    FVector AdultBodyScale = FVector::OneVector;
};
