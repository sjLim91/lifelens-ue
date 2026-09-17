#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LLDynamicEnvironmentPresentationActor.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class USceneComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;

/**
 * Read-only presentation of authoritative Core time/weather.
 *
 * This actor never advances simulation time and never generates weather. It
 * consumes ULLCoreBridgeSubsystem observations and maps them to lighting,
 * atmosphere and fog parameters that Dagyeom can tune visually in PIE.
 */
UCLASS(Config=Game, DefaultConfig)
class LIFELENS_API ALLDynamicEnvironmentPresentationActor : public AActor
{
    GENERATED_BODY()

public:
    ALLDynamicEnvironmentPresentationActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    void RefreshFromCore(bool bForce);
    void ApplyLighting(float Daylight01, float CloudCover01, float Visibility01, int32 MinuteOfDay, float AnnualPhase);
    void ApplyFog(float Daylight01, float CloudCover01, float Visibility01, float Humidity01, float Precipitation01);

    UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY() TObjectPtr<UDirectionalLightComponent> SunLight;
    UPROPERTY() TObjectPtr<USkyLightComponent> SkyLight;
    UPROPERTY() TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;
    UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> HeightFog;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Environment", meta=(ClampMin="0.05", ClampMax="5.0"))
    float RefreshIntervalSeconds = 0.25f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Lighting", meta=(ClampMin="0.0"))
    float DaySunIntensity = 8.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Lighting", meta=(ClampMin="0.0"))
    float NightSunIntensity = 0.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Lighting", meta=(ClampMin="0.0"))
    float DaySkyIntensity = 1.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Lighting", meta=(ClampMin="0.0"))
    float NightSkyIntensity = 0.12f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Lighting", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MaximumCloudLightReduction = 0.55f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Lighting", meta=(ClampMin="-360.0", ClampMax="360.0"))
    float SunAzimuthDegrees = 125.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Lighting", meta=(ClampMin="0.0", ClampMax="45.0"))
    float SeasonalAzimuthSwingDegrees = 10.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Fog", meta=(ClampMin="0.0", ClampMax="0.2"))
    float ClearFogDensity = 0.004f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Fog", meta=(ClampMin="0.0", ClampMax="0.2"))
    float SevereFogDensity = 0.055f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Fog", meta=(ClampMin="0.0", ClampMax="1.0"))
    float HumidityFogWeight = 0.35f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Fog", meta=(ClampMin="0.0", ClampMax="1.0"))
    float PrecipitationFogWeight = 0.40f;

    float RefreshAccumulator = 0.0f;
    int64 LastAppliedSimulationMinute = TNumericLimits<int64>::Lowest();
};
