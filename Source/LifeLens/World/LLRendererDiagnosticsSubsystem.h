#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LLRendererDiagnosticsSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FLLRendererDiagnosticsSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Renderer") FString PlatformName;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Renderer") int32 DynamicGlobalIlluminationMethod = -1;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Renderer") int32 ReflectionMethod = -1;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Renderer") int32 VirtualShadowMaps = -1;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Renderer") int32 AntiAliasingMethod = -1;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Renderer") int32 NaniteProjectEnabled = -1;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Renderer") int32 GenerateMeshDistanceFields = -1;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Renderer") int32 LumenHardwareRayTracing = -1;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Renderer") int32 RayTracing = -1;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Renderer") int32 MobileHDR = -1;
};

/**
 * Runtime-only visibility into the renderer tier that actually booted.
 *
 * Config files express intent; this subsystem logs the effective CVars so a
 * successful compile cannot be mistaken for successful Lumen/Nanite/VSM/TSR
 * runtime activation. It never mutates rendering settings.
 */
UCLASS()
class LIFELENS_API ULLRendererDiagnosticsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintPure, Category="LifeLens|Renderer")
    FLLRendererDiagnosticsSnapshot GetRendererDiagnostics() const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Renderer")
    FString GetRendererDiagnosticsSummary() const;
};
