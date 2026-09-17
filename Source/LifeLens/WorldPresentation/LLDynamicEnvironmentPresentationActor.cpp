#include "WorldPresentation/LLDynamicEnvironmentPresentationActor.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/GameInstance.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLEnvironmentReadTypes.h"
#include "Simulation/LLTimeReadTypes.h"
#include "UObject/UObjectIterator.h"

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
}

void ALLDynamicEnvironmentPresentationActor::BeginPlay()
{
    Super::BeginPlay();
    ResolveWorldComponents();
    RefreshFromCore(true);
}

void ALLDynamicEnvironmentPresentationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

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
        SunLight->bUsedAsAtmosphereSunLight = true;
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
    ApplyLighting(
        Saturate(Time.Daylight01),
        Saturate(Environment.CloudCover01),
        Saturate(Environment.Visibility01),
        Time.MinuteOfDay,
        Saturate(Time.AnnualPhase));
    ApplyFog(
        Saturate(Time.Daylight01),
        Saturate(Environment.CloudCover01),
        Saturate(Environment.Visibility01),
        Saturate(Environment.Humidity01),
        Saturate(Environment.PrecipitationIntensity01));
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
