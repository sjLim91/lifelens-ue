#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/LLTypes.h"
#include "LLActivityAnchor.generated.h"

UENUM(BlueprintType)
enum class ELLActivityAnchorState : uint8
{
    Available,
    Reserved,
    InUse
};

UCLASS(Blueprintable)
class LIFELENS_API ALLActivityAnchor : public AActor
{
    GENERATED_BODY()

public:
    ALLActivityAnchor();

    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    FVector GetUseLocation() const;

    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    FTransform GetUseTransform() const;

    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    bool SupportsIntent(ELLActionIntent Intent) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    bool CanBeUsedBy(FGuid ResidentId) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    bool IsClaimedBy(FGuid ResidentId) const;

    bool TryReserve(FGuid ResidentId);
    bool MarkInUse(FGuid ResidentId);
    void Release(FGuid ResidentId);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LifeLens|World")
    ELLActionIntent SupportedIntent = ELLActionIntent::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LifeLens|World")
    TArray<ELLActionIntent> AdditionalSupportedIntents;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LifeLens|World")
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LifeLens|World")
    FVector LocalUseOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LifeLens|World")
    FRotator LocalUseRotation = FRotator::ZeroRotator;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="LifeLens|World")
    ELLActivityAnchorState State = ELLActivityAnchorState::Available;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="LifeLens|World")
    FGuid ClaimedResidentId;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;
};
