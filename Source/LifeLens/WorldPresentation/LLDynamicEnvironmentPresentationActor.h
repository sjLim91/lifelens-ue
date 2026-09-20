#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LLDynamicEnvironmentPresentationActor.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
class UPostProcessComponent;
class USceneComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UStaticMesh;
struct FLLCoreSkyPresentationObservation;

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
    void ClearTransientEnvironmentPresentation();
    void RefreshFromCore(bool bForce);
    void ApplyLighting(
        const FLLCoreSkyPresentationObservation& Sky,
        float Daylight01,
        float CloudCover01,
        float Visibility01);
    void ApplyFog(
        float Daylight01,
        float CloudCover01,
        float Visibility01,
        float FogAmount01);
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
    UPROPERTY() TObjectPtr<UMaterialInterface> FallbackPrecipitationMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FallbackRainMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FallbackSnowMaterial;

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

    // Presentation-only interpolation between authoritative environment samples.
    // Higher values follow Core faster; zero snaps to each sampled value.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Environment", meta=(ClampMin="0.0", ClampMax="20.0"))
    float EnvironmentTransitionInterpSpeed = 3.0f;

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

    // Production safety fallback. Authored Niagara always wins when assigned;
    // this path exists so authoritative rain/snow can never become invisible
    // merely because an optional VFX asset is absent from a cook.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|VFX|Fallback")
    bool bAllowPrimitivePrecipitationFallback = true;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|VFX|Fallback", meta=(ClampMin="8", ClampMax="256"))
    int32 MaxFallbackRainInstances = 144;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|VFX|Fallback", meta=(ClampMin="8", ClampMax="192"))
    int32 MaxFallbackSnowInstances = 96;

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
    float NightSkyIntensity = 0.32f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Fog", meta=(ClampMin="0.0", ClampMax="0.2"))
    float ClearFogDensity = 0.004f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Fog", meta=(ClampMin="0.0", ClampMax="0.2"))
    float SevereFogDensity = 0.055f;

    // Keep the playable settlement crisp while allowing distant visual-only
    // terrain to dissolve into atmosphere before its outer edge can read.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Fog", meta=(ClampMin="0.0", ClampMax="30000.0"))
    float HorizonFogStartDistanceUU = 6500.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Fog", meta=(ClampMin="0.0", ClampMax="1.0"))
    float HorizonFogMaxOpacity = 0.78f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="-3.0", ClampMax="3.0"))
    float DayExposureBias = 0.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="-3.0", ClampMax="3.0"))
    float NightExposureBias = 0.30f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MaximumStormExposureReduction = 0.22f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="-1.0", ClampMax="2.0"))
    float MinimumNightExposureBias = 0.18f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="0.0", ClampMax="2.0"))
    float ClearSaturation = 1.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="0.0", ClampMax="2.0"))
    float SevereWeatherSaturation = 0.78f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="0.5", ClampMax="1.5"))
    float ClearContrast = 1.02f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="0.5", ClampMax="1.5"))
    float SevereWeatherContrast = 0.90f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="0.0", ClampMax="1.0"))
    float TwilightSaturationLift = 0.08f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="0.0", ClampMax="2.0"))
    float ClearBloomIntensity = 0.32f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|PostProcess", meta=(ClampMin="0.0", ClampMax="2.0"))
    float SevereWeatherBloomIntensity = 0.12f;

    float RefreshAccumulator = 0.0f;
    int64 LastAppliedSimulationMinute = TNumericLimits<int64>::Lowest();
    int64 LastObservedRuntimeGeneration = -1;
    float FallbackVisualTime = 0.0f;
    float FallbackRainIntensity01 = 0.0f;
    float FallbackSnowIntensity01 = 0.0f;
    float FallbackWind01 = 0.0f;

    // Presentation-only snow residue. Core still owns weather and temperature;
    // this only prevents the material snow cue from disappearing on the exact
    // minute snowfall stops when no authored snow-cover DTO exists yet.
    float PresentedSnowCover01 = 0.0f;
    bool bPresentedSnowInitialized = false;

    float PresentedDaylight01 = 0.0f;
    float PresentedCloudCover01 = 0.0f;
    float PresentedVisibility01 = 1.0f;
    float PresentedHumidity01 = 0.0f;
    float PresentedPrecipitation01 = 0.0f;
    float PresentedSurfaceWetness01 = 0.0f;
    float PresentedWind01 = 0.0f;
    float PresentedRain01 = 0.0f;
    float PresentedSnowfall01 = 0.0f;
    float PresentedAirTemperatureC = 20.0f;
    float PresentedSunElevationDegrees = 0.0f;
    float PresentedSunAzimuthDegrees = 0.0f;
    float PresentedSunIntensity01 = 0.0f;
    float PresentedSkyBrightness01 = 0.0f;
    float PresentedFogAmount01 = 0.0f;
    bool bPresentedEnvironmentInitialized = false;

    bool bFallbackRainActive = false;
    bool bFallbackSnowActive = false;
};
