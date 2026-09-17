#include "WorldPresentation/LLDynamicEnvironmentPresentationActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/GameInstance.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLEnvironmentReadTypes.h"
#include "Simulation/LLTimeReadTypes.h"
#include "UObject/UObjectIterator.h"
#include "WorldPresentation/LLWorldPresentationActor.h"

namespace
{
float Saturate(float Value)
{
    return FMath::Clamp(Value, 0.0f, 1.0f);
}

FLinearColor BlendColor(const FLinearColor& A, const FLinearColor& B, float Alpha)
{
    return A + (B - A) * Saturate(Alpha);
}

template <typename TActor, typename TComponent>
TComponent* FindFirstWorldComponent(UWorld* World)
{
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<TActor> It(World); It; ++It)
    {
        if (TComponent* Component = It->template FindComponentByClass<TComponent>())
        {
            return Component;
        }
    }
    return nullptr;
}

USkyAtmosphereComponent* FindSkyAtmosphereComponent(UWorld* World)
{
    if (!World)
    {
        return nullptr;
    }

    for (TObjectIterator<USkyAtmosphereComponent> It; It; ++It)
    {
        USkyAtmosphereComponent* Component = *It;
        if (Component
            && !Component->HasAnyFlags(RF_ClassDefaultObject)
            && Component->GetWorld() == World)
        {
            return Component;
        }
    }
    return nullptr;
}
}

ALLDynamicEnvironmentPresentationActor::ALLDynamicEnvironmentPresentationActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("EnvironmentRoot"));
    SetRootComponent(SceneRoot);

    RainEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RainEffect"));
    RainEffect->SetupAttachment(SceneRoot);
    RainEffect->SetAutoActivate(false);

    SnowEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SnowEffect"));
    SnowEffect->SetupAttachment(SceneRoot);
    SnowEffect->SetAutoActivate(false);

    FogEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FogEffect"));
    FogEffect->SetupAttachment(SceneRoot);
    FogEffect->SetAutoActivate(false);

    PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("EnvironmentPostProcess"));
    PostProcess->SetupAttachment(SceneRoot);
    PostProcess->bUnbound = true;
    PostProcess->BlendWeight = 1.0f;
    PostProcess->Priority = -10.0f;
}

void ALLDynamicEnvironmentPresentationActor::BeginPlay()
{
    Super::BeginPlay();
    ResolveWorldComponents();
    ConfigureEffectAssets();
    RefreshFromCore(true);
}

void ALLDynamicEnvironmentPresentationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdateEffectAnchor();

    RefreshAccumulator += FMath::Max(0.0f, DeltaSeconds);
    if (RefreshAccumulator < RefreshIntervalSeconds)
    {
        return;
    }

    RefreshAccumulator = 0.0f;
    RefreshFromCore(false);
}

void ALLDynamicEnvironmentPresentationActor::ResolveWorldComponents()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    SunLight = FindFirstWorldComponent<ADirectionalLight, UDirectionalLightComponent>(World);
    SkyLight = FindFirstWorldComponent<ASkyLight, USkyLightComponent>(World);
    SkyAtmosphere = FindSkyAtmosphereComponent(World);
    HeightFog = FindFirstWorldComponent<AExponentialHeightFog, UExponentialHeightFogComponent>(World);

    if (!SunLight)
    {
        if (ADirectionalLight* Actor = World->SpawnActor<ADirectionalLight>())
        {
            SunLight = Actor->FindComponentByClass<UDirectionalLightComponent>();
        }
    }
    if (!SkyLight)
    {
        if (ASkyLight* Actor = World->SpawnActor<ASkyLight>())
        {
            SkyLight = Actor->FindComponentByClass<USkyLightComponent>();
        }
    }
    if (!SkyAtmosphere)
    {
        SkyAtmosphere = NewObject<USkyAtmosphereComponent>(this, TEXT("FallbackSkyAtmosphere"));
        if (SkyAtmosphere)
        {
            SkyAtmosphere->SetupAttachment(SceneRoot);
            AddInstanceComponent(SkyAtmosphere);
            SkyAtmosphere->RegisterComponent();
        }
    }
    if (!HeightFog)
    {
        if (AExponentialHeightFog* Actor = World->SpawnActor<AExponentialHeightFog>())
        {
            HeightFog = Actor->FindComponentByClass<UExponentialHeightFogComponent>();
        }
    }

    if (SunLight)
    {
        SunLight->SetMobility(EComponentMobility::Movable);
        SunLight->bAtmosphereSunLight = true;
        SunLight->AtmosphereSunLightIndex = 0;
        SunLight->SetCastShadows(true);
    }
    if (SkyLight)
    {
        SkyLight->SetMobility(EComponentMobility::Movable);
        SkyLight->bRealTimeCapture = false;
    }
    if (SkyAtmosphere)
    {
        SkyAtmosphere->SetMobility(EComponentMobility::Movable);
    }
    if (HeightFog)
    {
        HeightFog->SetMobility(EComponentMobility::Movable);
        HeightFog->SetFogDensity(ClearFogDensity);
    }
}

void ALLDynamicEnvironmentPresentationActor::ConfigureEffectAssets()
{
    auto AssignSystem = [](UNiagaraComponent* Component, const TSoftObjectPtr<UNiagaraSystem>& SystemAsset)
    {
        if (!Component || SystemAsset.IsNull())
        {
            return;
        }
        if (UNiagaraSystem* System = SystemAsset.LoadSynchronous())
        {
            Component->SetAsset(System);
        }
    };

    AssignSystem(RainEffect, RainSystem);
    AssignSystem(SnowEffect, SnowSystem);
    AssignSystem(FogEffect, FogSystem);
}

void ALLDynamicEnvironmentPresentationActor::UpdateEffectAnchor()
{
    const bool bAnyEffectActive =
        (RainEffect && RainEffect->IsActive())
        || (SnowEffect && SnowEffect->IsActive())
        || (FogEffect && FogEffect->IsActive());
    if (!bAnyEffectActive)
    {
        return;
    }

    const UWorld* World = GetWorld();
    const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
    const APlayerCameraManager* CameraManager = Controller ? Controller->PlayerCameraManager : nullptr;
    if (!CameraManager)
    {
        return;
    }

    const FVector Anchor = CameraManager->GetCameraLocation() + FVector(0.0f, 0.0f, EffectAnchorHeightUU);
    if (RainEffect && RainEffect->IsActive()) { RainEffect->SetWorldLocation(Anchor); }
    if (SnowEffect && SnowEffect->IsActive()) { SnowEffect->SetWorldLocation(Anchor); }
    if (FogEffect && FogEffect->IsActive()) { FogEffect->SetWorldLocation(Anchor); }
}

void ALLDynamicEnvironmentPresentationActor::RefreshFromCore(bool bForce)
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    ULLCoreBridgeSubsystem* Bridge = GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>();
    if (!Bridge || !Bridge->IsCoreRunning())
    {
        return;
    }

    const FLLCoreTimeObservation Time = Bridge->GetTimeObservation();
    const FLLCoreDynamicEnvironmentObservation Environment =
        Bridge->GetInitialRegionDynamicEnvironmentObservation();
    if (!Environment.bAvailable)
    {
        return;
    }

    if (!bForce && Time.SimulationMinute == LastAppliedSimulationMinute)
    {
        return;
    }

    LastAppliedSimulationMinute = Time.SimulationMinute;

    const float Daylight01 = Saturate(Time.Daylight01);
    const float CloudCover01 = Saturate(Environment.CloudCover01);
    const float Visibility01 = Saturate(Environment.Visibility01);
    const float Humidity01 = Saturate(Environment.Humidity01);
    const float Precipitation01 = Saturate(Environment.PrecipitationIntensity01);
    const float SurfaceWetness01 = Saturate(Environment.SurfaceWetness01);
    const float Wind01 = Saturate(Environment.WindIntensity01);
    const float Rain01 = Environment.PrecipitationType == ELLCorePrecipitationType::Rain
        ? Precipitation01
        : 0.0f;
    const float Snow01 = Environment.PrecipitationType == ELLCorePrecipitationType::Snow
        ? Precipitation01
        : 0.0f;
    const float Fog01 = Saturate(
        (1.0f - Visibility01)
        + 0.35f * Humidity01
        + 0.25f * Precipitation01);

    ApplyLighting(
        Daylight01,
        CloudCover01,
        Visibility01,
        Time.MinuteOfDay,
        Saturate(Time.AnnualPhase));
    ApplyFog(
        Daylight01,
        CloudCover01,
        Visibility01,
        Humidity01,
        Precipitation01);
    ApplySurfaceMaterials(
        SurfaceWetness01,
        Snow01,
        Precipitation01,
        Environment.AirTemperatureC);
    ApplyWeatherEffects(Rain01, Snow01, Fog01, Wind01);
    ApplyPostProcess(Daylight01, CloudCover01, Visibility01, Precipitation01);
}

void ALLDynamicEnvironmentPresentationActor::ApplyLighting(
    float Daylight01,
    float CloudCover01,
    float Visibility01,
    int32 MinuteOfDay,
    float AnnualPhase)
{
    const float DayFraction = FMath::Fmod(FMath::Max(0.0f, static_cast<float>(MinuteOfDay)), 1440.0f) / 1440.0f;
    const float SolarElevationDegrees =
        FMath::Sin((DayFraction - 0.25f) * 2.0f * PI) * 90.0f;
    const float SeasonalYaw =
        FMath::Sin(AnnualPhase * 2.0f * PI) * SeasonalAzimuthSwingDegrees;

    if (SunLight)
    {
        SunLight->SetWorldRotation(FRotator(-SolarElevationDegrees, SunAzimuthDegrees + SeasonalYaw, 0.0f));

        const float CloudMultiplier =
            1.0f - MaximumCloudLightReduction * CloudCover01;
        const float SunIntensity = FMath::Lerp(NightSunIntensity, DaySunIntensity, Daylight01) * CloudMultiplier;
        SunLight->SetIntensity(FMath::Max(0.0f, SunIntensity));

        const FLinearColor DayColor(1.0f, 0.94f, 0.82f, 1.0f);
        const FLinearColor HorizonColor(1.0f, 0.48f, 0.24f, 1.0f);
        const FLinearColor NightColor(0.30f, 0.40f, 0.66f, 1.0f);
        const float HorizonWarmth = Daylight01 > KINDA_SMALL_NUMBER
            ? FMath::Square(1.0f - Daylight01)
            : 0.0f;
        FLinearColor SunColor = BlendColor(DayColor, HorizonColor, HorizonWarmth);
        SunColor = BlendColor(NightColor, SunColor, Daylight01);
        SunLight->SetLightColor(SunColor);
    }

    if (SkyLight)
    {
        const float VisibilityLift = FMath::Lerp(0.72f, 1.0f, Visibility01);
        const float CloudMultiplier = FMath::Lerp(1.0f, 0.72f, CloudCover01);
        const float SkyIntensity =
            FMath::Lerp(NightSkyIntensity, DaySkyIntensity, Daylight01) *
            VisibilityLift * CloudMultiplier;
        SkyLight->SetIntensity(FMath::Max(0.0f, SkyIntensity));
    }
}

void ALLDynamicEnvironmentPresentationActor::ApplyFog(
    float Daylight01,
    float CloudCover01,
    float Visibility01,
    float Humidity01,
    float Precipitation01)
{
    if (!HeightFog)
    {
        return;
    }

    const float VisibilityPressure = 1.0f - Visibility01;
    const float WeatherFog01 = Saturate(
        VisibilityPressure +
        HumidityFogWeight * Humidity01 +
        PrecipitationFogWeight * Precipitation01);
    HeightFog->SetFogDensity(FMath::Lerp(ClearFogDensity, SevereFogDensity, WeatherFog01));

    const FLinearColor NightFog(0.045f, 0.065f, 0.11f, 1.0f);
    const FLinearColor DayFog(0.62f, 0.72f, 0.80f, 1.0f);
    const FLinearColor StormFog(0.22f, 0.27f, 0.31f, 1.0f);
    FLinearColor FogColor = BlendColor(NightFog, DayFog, Daylight01);
    FogColor = BlendColor(FogColor, StormFog, Saturate(0.65f * CloudCover01 + 0.35f * WeatherFog01));
    HeightFog->SetFogInscatteringColor(FogColor);
}

void ALLDynamicEnvironmentPresentationActor::ApplySurfaceMaterials(
    float SurfaceWetness01,
    float Snow01,
    float Precipitation01,
    float AirTemperatureC)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<ALLWorldPresentationActor> It(World); It; ++It)
    {
        TInlineComponentArray<UMeshComponent*> MeshComponents(*It);
        for (UMeshComponent* Mesh : MeshComponents)
        {
            if (!Mesh)
            {
                continue;
            }

            Mesh->SetScalarParameterValueOnMaterials(WetnessMaterialParameter, SurfaceWetness01);
            Mesh->SetScalarParameterValueOnMaterials(SnowMaterialParameter, Snow01);
            Mesh->SetScalarParameterValueOnMaterials(PrecipitationMaterialParameter, Precipitation01);
            Mesh->SetScalarParameterValueOnMaterials(AirTemperatureMaterialParameter, AirTemperatureC);
        }
    }
}

void ALLDynamicEnvironmentPresentationActor::ApplyWeatherEffects(
    float Rain01,
    float Snow01,
    float Fog01,
    float Wind01)
{
    if (RainEffect)
    {
        RainEffect->SetVariableFloat(TEXT("User.Intensity"), Rain01);
        RainEffect->SetVariableFloat(TEXT("User.WindIntensity"), Wind01);
        SetEffectActive(RainEffect, Rain01 >= EffectActivationThreshold);
    }
    if (SnowEffect)
    {
        SnowEffect->SetVariableFloat(TEXT("User.Intensity"), Snow01);
        SnowEffect->SetVariableFloat(TEXT("User.WindIntensity"), Wind01);
        SetEffectActive(SnowEffect, Snow01 >= EffectActivationThreshold);
    }
    if (FogEffect)
    {
        FogEffect->SetVariableFloat(TEXT("User.Intensity"), Fog01);
        FogEffect->SetVariableFloat(TEXT("User.WindIntensity"), Wind01);
        SetEffectActive(FogEffect, Fog01 >= EffectActivationThreshold);
    }
}

void ALLDynamicEnvironmentPresentationActor::ApplyPostProcess(
    float Daylight01,
    float CloudCover01,
    float Visibility01,
    float Precipitation01)
{
    if (!PostProcess)
    {
        return;
    }

    const float WeatherPressure01 = Saturate(
        0.45f * CloudCover01
        + 0.35f * Precipitation01
        + 0.20f * (1.0f - Visibility01));

    PostProcess->Settings.bOverride_AutoExposureBias = true;
    PostProcess->Settings.AutoExposureBias =
        FMath::Lerp(NightExposureBias, DayExposureBias, Daylight01)
        - MaximumStormExposureReduction * WeatherPressure01;

    const float Saturation = FMath::Lerp(ClearSaturation, SevereWeatherSaturation, WeatherPressure01);
    PostProcess->Settings.bOverride_ColorSaturation = true;
    PostProcess->Settings.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.0f);

    PostProcess->Settings.bOverride_VignetteIntensity = true;
    PostProcess->Settings.VignetteIntensity = FMath::Lerp(0.18f, 0.28f, WeatherPressure01);
}

void ALLDynamicEnvironmentPresentationActor::SetEffectActive(
    UNiagaraComponent* Component,
    bool bShouldBeActive) const
{
    if (!Component)
    {
        return;
    }

    const bool bHasSystem = Component->GetAsset() != nullptr;
    if (bShouldBeActive && bHasSystem)
    {
        if (!Component->IsActive())
        {
            Component->Activate();
        }
    }
    else if (Component->IsActive())
    {
        Component->Deactivate();
    }
}
