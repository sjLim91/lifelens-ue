#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LLDynamicEnvironmentPresentationActor.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UPostProcessComponent;
class USceneComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UStaticMesh;

/**
 * Read-only presentation of authoritative Core time/weather.
 *
 * This actor never advances simulation time and never generates weather. It
 * consumes ULLCoreBridgeSubsystem observations and maps them to lighting,
 * atmosphere, surface material parameters, weather VFX and post-process values.
 * Existing map lighting is reused when present; fallback actors are spawned
 * only when the production map does not provide the relevant primitive.
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
    void ResolveWorldComponents();
    void ConfigureEffectAssets();
    void ConfigureFallbackPrecipitation();
    void UpdateEffectAnchor();
    void UpdateFallbackPrecipitation(float DeltaSeconds);
    void RefreshFromCore(bool bForce);
    void ApplyLighting(float Daylight01, float CloudCover01, float Visibility01, int32 MinuteOfDay, float AnnualPhase);
    void ApplyFog(float Daylight01, float CloudCover01, float Visibility01, float Humidity01, float Precipitation01);
    void ApplySurfaceMaterials(float SurfaceWetness01, float Snow01, float Precipitation01, float AirTemperatureC);
    void ApplyWeatherEffects(float Rain01, float Snow01, float Fog01, float Wind01);
    void ApplyPostProcess(float Daylight01, float CloudCover01, float Visibility01, float Precipitation01);
    void SetEffectActive(UNiagaraComponent* Component, bool bShouldBeActive) const;

    UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY() TObjectPtr<UDirectionalLightComponent> SunLight;
    UPROPERTY() TObjectPtr<USkyLightComponent> SkyLight;
    UPROPERTY() TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;
    UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> HeightFog;
    UPROPERTY() TObjectPtr<UNiagaraComponent> RainEffect;
    UPROPERTY() TObjectPtr<UNiagaraComponent> SnowEffect;
    UPROPERTY() TObjectPtr<UNiagaraComponent> FogEffect;
    UPROPERTY() TObjectPtr<UPostProcessComponent> PostProcess;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> RainFallback;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SnowFallback;
    UPROPERTY() TObjectPtr<UStaticMesh> FallbackRainMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> FallbackSnowMesh;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Environment", meta=(ClampMin="0.05", ClampMax="5.0"))
    float RefreshIntervalSeconds = 0.25f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Environment", meta=(ClampMin="0.0", ClampMax="0.25"))
    float EffectActivationThreshold = 0.02f;

    // Lower threshold for shutting an already-visible weather effect off.
    // The small hysteresis prevents one-minute intensity noise from repeatedly
    // popping rain/snow VFX on and off near the activation boundary.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Environment", meta=(ClampMin="0.0", ClampMax="0.25"))
    float EffectDeactivationThreshold = 0.012f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Environment", meta=(ClampMin="0.0", ClampMax="5000.0"))
    float EffectAnchorHeightUU = 1200.0f;

    // Stable material parameter contract. WorldPresentation materials may opt
    // into any/all of these without creating a second weather authority.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Materials")
    FName WetnessMaterialParameter = TEXT("LL_Wetness");

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Materials")
    FName SnowMaterialParameter = TEXT("LL_Snow");

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Materials")
    FName PrecipitationMaterialParameter = TEXT("LL_Precipitation");

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Materials")
    FName AirTemperatureMaterialParameter = TEXT("LL_AirTemperatureC");

    // Optional authored Niagara systems. The binding and activation path is
    // fully runtime-ready even when a visual asset has not yet been assigned.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|VFX")
    TSoftObjectPtr<UNiagaraSystem> RainSystem;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|VFX")
    TSoftObjectPtr<UNiagaraSystem> SnowSystem;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|VFX")
    TSoftObjectPtr<UNiagaraSystem> FogSystem;

    // Code-only Android-safe fallback used only when an authored Niagara system
    // has not been assigned. This makes rain/snow visible in packaged builds
    // today while preserving Niagara as the preferred upgrade path.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|VFX|Fallback", meta=(ClampMin="8", ClampMax="256"))
    int32 MaxFallbackRainInstances = 96;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|VFX|Fallback", meta=(ClampMin="8", ClampMax="192"))
    int32 MaxFallbackSnowInstances = 64;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|VFX|Fallback", meta=(ClampMin="500.0", ClampMax="6000.0"))
    float FallbackPrecipitationRadiusUU = 2200.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|VFX|Fallback", meta=(ClampMin="500.0", ClampMax="6000.0"))
    float FallbackPrecipitationHeightUU = 2600.0f;

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

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="-3.0", ClampMax="3.0"))
    float DayExposureBias = 0.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="-3.0", ClampMax="3.0"))
    float NightExposureBias = -0.35f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MaximumStormExposureReduction = 0.30f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="0.0", ClampMax="2.0"))
    float ClearSaturation = 1.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="0.0", ClampMax="2.0"))
    float SevereWeatherSaturation = 0.78f;

    float RefreshAccumulator = 0.0f;
    int64 LastAppliedSimulationMinute = TNumericLimits<int64>::Lowest();
    float FallbackVisualTime = 0.0f;
    float FallbackRainIntensity01 = 0.0f;
    float FallbackSnowIntensity01 = 0.0f;
    float FallbackWind01 = 0.0f;
    bool bFallbackRainActive = false;
    bool bFallbackSnowActive = false;
};
