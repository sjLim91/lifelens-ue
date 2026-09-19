#include "WorldPresentation/LLDynamicEnvironmentPresentationActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/GameInstance.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLEnvironmentReadTypes.h"
#include "Simulation/LLTimeReadTypes.h"
#include "UObject/ConstructorHelpers.h"
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

float StableNoise01(int32 Index, float Salt)
{
    const float Value = FMath::Sin(static_cast<float>(Index) * 12.9898f + Salt * 78.233f) * 43758.5453f;
    return FMath::Frac(FMath::Abs(Value));
}

float TwilightFactor(float Daylight01)
{
    // Strongest around low-but-nonzero daylight and fades at full day/night.
    return Saturate(1.0f - FMath::Abs(Saturate(Daylight01) - 0.24f) / 0.24f);
}

const FName PrecipitationColorParameter(TEXT("Color"));

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

    static ConstructorHelpers::FObjectFinder<UStaticMesh> FallbackRainMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> FallbackSnowMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackPrecipitationMaterialFinder(
        TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
    FallbackRainMesh = FallbackRainMeshFinder.Succeeded() ? FallbackRainMeshFinder.Object : nullptr;
    FallbackSnowMesh = FallbackSnowMeshFinder.Succeeded() ? FallbackSnowMeshFinder.Object : nullptr;
    FallbackPrecipitationMaterial = FallbackPrecipitationMaterialFinder.Succeeded()
        ? FallbackPrecipitationMaterialFinder.Object
        : nullptr;

    RainFallback = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RainFallback"));
    RainFallback->SetupAttachment(SceneRoot);
    RainFallback->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RainFallback->SetCanEverAffectNavigation(false);
    RainFallback->SetCastShadow(false);
    RainFallback->SetVisibility(false);

    SnowFallback = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SnowFallback"));
    SnowFallback->SetupAttachment(SceneRoot);
    SnowFallback->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SnowFallback->SetCanEverAffectNavigation(false);
    SnowFallback->SetCastShadow(false);
    SnowFallback->SetVisibility(false);

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
    ConfigureFallbackPrecipitation();
    RefreshFromCore(true);
}

void ALLDynamicEnvironmentPresentationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdateEffectAnchor();
    UpdateFallbackPrecipitation(DeltaSeconds);

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
        HeightFog->SetStartDistance(FMath::Max(0.0f, HorizonFogStartDistanceUU));
        HeightFog->SetFogMaxOpacity(FMath::Clamp(HorizonFogMaxOpacity, 0.0f, 1.0f));
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

void ALLDynamicEnvironmentPresentationActor::ConfigureFallbackPrecipitation()
{
    if (FallbackPrecipitationMaterial)
    {
        FallbackRainMaterial = UMaterialInstanceDynamic::Create(FallbackPrecipitationMaterial, this);
        FallbackSnowMaterial = UMaterialInstanceDynamic::Create(FallbackPrecipitationMaterial, this);

        if (FallbackRainMaterial)
        {
            FallbackRainMaterial->SetVectorParameterValue(
                PrecipitationColorParameter,
                FLinearColor(0.32f, 0.62f, 0.90f, 1.0f));
            if (RainFallback) { RainFallback->SetMaterial(0, FallbackRainMaterial); }
        }
        if (FallbackSnowMaterial)
        {
            FallbackSnowMaterial->SetVectorParameterValue(
                PrecipitationColorParameter,
                FLinearColor(0.88f, 0.95f, 1.0f, 1.0f));
            if (SnowFallback) { SnowFallback->SetMaterial(0, FallbackSnowMaterial); }
        }
    }

    auto Configure = [this](UInstancedStaticMeshComponent* Component, UStaticMesh* Mesh, int32 MaxInstances)
    {
        if (!Component || !Mesh)
        {
            return;
        }
        Component->SetStaticMesh(Mesh);
        Component->ClearInstances();
        const int32 Count = FMath::Max(1, MaxInstances);
        for (int32 Index = 0; Index < Count; ++Index)
        {
            Component->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector));
        }
    };

    Configure(RainFallback, FallbackRainMesh, MaxFallbackRainInstances);
    Configure(SnowFallback, FallbackSnowMesh, MaxFallbackSnowInstances);
}

void ALLDynamicEnvironmentPresentationActor::UpdateEffectAnchor()
{
    const bool bAnyEffectActive =
        (RainEffect && RainEffect->IsActive())
        || (SnowEffect && SnowEffect->IsActive())
        || (FogEffect && FogEffect->IsActive())
        || bFallbackRainActive
        || bFallbackSnowActive;
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

    const FVector CameraAnchor = CameraManager->GetCameraLocation();
    if (RainFallback) { RainFallback->SetWorldLocation(CameraAnchor); }
    if (SnowFallback) { SnowFallback->SetWorldLocation(CameraAnchor); }
}

void ALLDynamicEnvironmentPresentationActor::UpdateFallbackPrecipitation(float DeltaSeconds)
{
    if (!bFallbackRainActive && !bFallbackSnowActive)
    {
        return;
    }

    FallbackVisualTime += FMath::Max(0.0f, DeltaSeconds);
    const float Radius = FMath::Max(100.0f, FallbackPrecipitationRadiusUU);
    const float Height = FMath::Max(200.0f, FallbackPrecipitationHeightUU);

    auto Animate = [&](UInstancedStaticMeshComponent* Component, int32 MaxInstances, float Intensity01, bool bSnow)
    {
        if (!Component || Component->GetInstanceCount() == 0)
        {
            return;
        }

        const int32 Count = Component->GetInstanceCount();
        const int32 ActiveCount = FMath::Clamp(FMath::RoundToInt(Saturate(Intensity01) * static_cast<float>(MaxInstances)), 0, Count);
        const float Speed = bSnow ? 180.0f : 1500.0f;
        for (int32 Index = 0; Index < Count; ++Index)
        {
            FTransform Transform = FTransform::Identity;
            if (Index < ActiveCount)
            {
                const float NX = StableNoise01(Index, bSnow ? 3.0f : 1.0f) * 2.0f - 1.0f;
                const float NY = StableNoise01(Index, bSnow ? 4.0f : 2.0f) * 2.0f - 1.0f;
                const float Phase = StableNoise01(Index, bSnow ? 6.0f : 5.0f);
                const float Fall01 = FMath::Fmod(FallbackVisualTime * Speed / Height + Phase, 1.0f);
                const float DriftPhase = FallbackVisualTime * (bSnow ? 0.75f : 0.20f)
                    + StableNoise01(Index, 9.0f) * 2.0f * PI;
                const float WindShift = (1.0f - Fall01) * FallbackWind01 * (bSnow ? 520.0f : 320.0f);
                const float SideDrift = bSnow
                    ? FMath::Sin(DriftPhase) * (90.0f + 170.0f * FallbackWind01)
                    : 0.0f;
                const FVector Location(
                    NX * Radius + WindShift,
                    NY * Radius + SideDrift,
                    FMath::Lerp(-400.0f, Height, 1.0f - Fall01));
                const float SnowScale = FMath::Lerp(0.025f, 0.055f, StableNoise01(Index, 10.0f));
                const FVector Scale = bSnow
                    ? FVector(SnowScale)
                    : FVector(0.014f, 0.014f, 0.46f);
                const FRotator Rotation = bSnow
                    ? FRotator(
                        FMath::Sin(DriftPhase) * 18.0f,
                        StableNoise01(Index, 8.0f) * 360.0f + FallbackVisualTime * 24.0f,
                        FMath::Cos(DriftPhase) * 18.0f)
                    : FRotator(0.0f, 0.0f, -FallbackWind01 * 12.0f);
                Transform = FTransform(Rotation, Location, Scale);
            }
            Component->UpdateInstanceTransform(Index, Transform, false, Index == Count - 1, true);
        }
    };

    if (bFallbackRainActive)
    {
        Animate(RainFallback, MaxFallbackRainInstances, FallbackRainIntensity01, false);
    }
    if (bFallbackSnowActive)
    {
        Animate(SnowFallback, MaxFallbackSnowInstances, FallbackSnowIntensity01, true);
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
    const FLLCoreSkyPresentationObservation Sky =
        Bridge->GetInitialRegionSkyPresentationObservation();
    if (!Environment.bAvailable || !Sky.bAvailable)
    {
        return;
    }

    if (!bForce && Time.SimulationMinute == LastAppliedSimulationMinute)
    {
        return;
    }

    const int64 PreviousSimulationMinute = LastAppliedSimulationMinute;
    LastAppliedSimulationMinute = Time.SimulationMinute;

    const float Daylight01 = Saturate(Time.Daylight01);
    const float CloudCover01 = Saturate(Sky.CloudCover01);
    const float Visibility01 = Saturate(Environment.Visibility01);
    const float Humidity01 = Saturate(Environment.Humidity01);
    const float Precipitation01 = Saturate(Environment.PrecipitationIntensity01);
    const float SurfaceWetness01 = Saturate(Sky.SurfaceWetness01);
    const float Wind01 = Saturate(Sky.WindIntensity01);
    const float Rain01 = Environment.PrecipitationType == ELLCorePrecipitationType::Rain
        ? Precipitation01
        : 0.0f;
    const float Snow01 = Environment.PrecipitationType == ELLCorePrecipitationType::Snow
        ? Precipitation01
        : 0.0f;

    const bool bResetPresentedEnvironment =
        bForce
        || !bPresentedEnvironmentInitialized
        || PreviousSimulationMinute == TNumericLimits<int64>::Lowest()
        || Time.SimulationMinute < PreviousSimulationMinute;
    const float TransitionSpeed = FMath::Max(0.0f, EnvironmentTransitionInterpSpeed);
    auto PresentValue = [this, bResetPresentedEnvironment, TransitionSpeed](
        float Current,
        float Target)
    {
        if (bResetPresentedEnvironment || TransitionSpeed <= KINDA_SMALL_NUMBER)
        {
            return Target;
        }
        return FMath::FInterpTo(
            Current,
            Target,
            FMath::Max(RefreshIntervalSeconds, KINDA_SMALL_NUMBER),
            TransitionSpeed);
    };

    PresentedDaylight01 = Saturate(PresentValue(PresentedDaylight01, Daylight01));
    PresentedCloudCover01 = Saturate(PresentValue(PresentedCloudCover01, CloudCover01));
    PresentedVisibility01 = Saturate(PresentValue(PresentedVisibility01, Visibility01));
    PresentedHumidity01 = Saturate(PresentValue(PresentedHumidity01, Humidity01));
    PresentedPrecipitation01 = Saturate(PresentValue(PresentedPrecipitation01, Precipitation01));
    PresentedSurfaceWetness01 = Saturate(PresentValue(PresentedSurfaceWetness01, SurfaceWetness01));
    PresentedWind01 = Saturate(PresentValue(PresentedWind01, Wind01));
    PresentedRain01 = Saturate(PresentValue(PresentedRain01, Rain01));
    PresentedSnowfall01 = Saturate(PresentValue(PresentedSnowfall01, Snow01));
    PresentedAirTemperatureC = PresentValue(
        PresentedAirTemperatureC,
        Environment.AirTemperatureC);
    bPresentedEnvironmentInitialized = true;

    // SurfaceWetness01 is already authoritative Core residue. Snow currently
    // exposes snowfall intensity but no separate accumulated-cover read, so
    // maintain presentation-only cover that can build across a storm and thaw
    // gradually afterwards. The simulation still owns precipitation and
    // temperature; this never feeds back into movement or resources.
    if (!bPresentedSnowInitialized || bForce
        || PreviousSimulationMinute == TNumericLimits<int64>::Lowest()
        || Time.SimulationMinute < PreviousSimulationMinute)
    {
        PresentedSnowCover01 = Snow01 * 0.45f;
        bPresentedSnowInitialized = true;
    }
    else
    {
        const float ElapsedSimulationMinutes = FMath::Clamp(
            static_cast<float>(Time.SimulationMinute - PreviousSimulationMinute),
            0.0f,
            1440.0f);
        const float TemperatureC = Environment.AirTemperatureC;

        if (Snow01 > KINDA_SMALL_NUMBER)
        {
            const float FreezeSupport = FMath::Clamp(
                (2.0f - TemperatureC) / 8.0f,
                0.25f,
                1.0f);
            const float Accumulation =
                Snow01 * ElapsedSimulationMinutes * 0.0012f * FreezeSupport;
            PresentedSnowCover01 = FMath::Max(
                PresentedSnowCover01,
                Snow01 * 0.45f);
            PresentedSnowCover01 += Accumulation;
        }

        if (TemperatureC > 0.0f)
        {
            const float MeltPerMinute =
                0.00012f + TemperatureC * 0.0009f;
            PresentedSnowCover01 -=
                ElapsedSimulationMinutes * MeltPerMinute;
        }
        else if (Snow01 <= KINDA_SMALL_NUMBER)
        {
            // Frozen old snow compacts/clears very slowly instead of vanishing
            // the minute active snowfall ends.
            PresentedSnowCover01 -=
                ElapsedSimulationMinutes * 0.00001f;
        }

        PresentedSnowCover01 = Saturate(PresentedSnowCover01);
    }

    const float Fog01 = Saturate(Sky.FogAmount01);

    ApplyLighting(
        Sky,
        PresentedDaylight01,
        PresentedCloudCover01,
        PresentedVisibility01);
    ApplyFog(
        PresentedDaylight01,
        PresentedCloudCover01,
        Fog01);
    ApplySurfaceMaterials(
        PresentedSurfaceWetness01,
        PresentedSnowCover01,
        PresentedPrecipitation01,
        PresentedAirTemperatureC);
    ApplyWeatherEffects(
        PresentedRain01,
        PresentedSnowfall01,
        Fog01,
        PresentedWind01);
    ApplyPostProcess(
        PresentedDaylight01,
        PresentedCloudCover01,
        PresentedVisibility01,
        PresentedPrecipitation01);
}

void ALLDynamicEnvironmentPresentationActor::ApplyLighting(
    const FLLCoreSkyPresentationObservation& Sky,
    float Daylight01,
    float CloudCover01,
    float Visibility01)
{
    if (SunLight)
    {
        // Rotation and normalized intensity come from the deterministic Core
        // time/weather presentation provider. This actor only maps those hints
        // into Unreal light units and color grading.
        SunLight->SetWorldRotation(FRotator(
            -Sky.SunElevationDegrees,
            Sky.SunAzimuthDegrees,
            0.0f));

        const float SunIntensity = FMath::Lerp(
            NightSunIntensity,
            DaySunIntensity,
            Saturate(Sky.SunIntensity01));
        SunLight->SetIntensity(FMath::Max(0.0f, SunIntensity));

        const FLinearColor DayColor(1.0f, 0.94f, 0.82f, 1.0f);
        const FLinearColor HorizonColor(1.0f, 0.48f, 0.24f, 1.0f);
        const FLinearColor NightColor(0.30f, 0.40f, 0.66f, 1.0f);
        const FLinearColor OvercastColor(0.61f, 0.69f, 0.78f, 1.0f);
        const float HorizonWarmth = Daylight01 > KINDA_SMALL_NUMBER
            ? FMath::Square(1.0f - Daylight01)
            : 0.0f;
        FLinearColor SunColor = BlendColor(DayColor, HorizonColor, HorizonWarmth);
        SunColor = BlendColor(NightColor, SunColor, Daylight01);

        const float WeatherCooling = Saturate(
            0.72f * CloudCover01
            + 0.28f * (1.0f - Visibility01));
        SunColor = BlendColor(
            SunColor,
            OvercastColor,
            WeatherCooling * 0.62f);
        SunLight->SetLightColor(SunColor);
    }

    if (SkyLight)
    {
        const float SkyIntensity = FMath::Lerp(
            NightSkyIntensity,
            DaySkyIntensity,
            Saturate(Sky.SkyBrightness01));
        SkyLight->SetIntensity(FMath::Max(0.0f, SkyIntensity));
    }
}

void ALLDynamicEnvironmentPresentationActor::ApplyFog(
    float Daylight01,
    float CloudCover01,
    float FogAmount01)
{
    if (!HeightFog)
    {
        return;
    }

    const float WeatherFog01 = Saturate(FogAmount01);
    HeightFog->SetFogDensity(FMath::Lerp(
        ClearFogDensity,
        SevereFogDensity,
        WeatherFog01));

    const FLinearColor NightFog(0.045f, 0.065f, 0.11f, 1.0f);
    const FLinearColor DayFog(0.62f, 0.72f, 0.80f, 1.0f);
    const FLinearColor TwilightFog(0.74f, 0.43f, 0.30f, 1.0f);
    const FLinearColor StormFog(0.20f, 0.26f, 0.32f, 1.0f);
    FLinearColor FogColor = BlendColor(NightFog, DayFog, Daylight01);
    FogColor = BlendColor(
        FogColor,
        TwilightFog,
        TwilightFactor(Daylight01) * (1.0f - 0.75f * CloudCover01));
    FogColor = BlendColor(
        FogColor,
        StormFog,
        Saturate(0.65f * CloudCover01 + 0.35f * WeatherFog01));
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

    FallbackRainIntensity01 = Rain01;
    FallbackSnowIntensity01 = Snow01;
    FallbackWind01 = Wind01;

    const float OffThreshold = FMath::Clamp(
        FMath::Min(EffectActivationThreshold, EffectDeactivationThreshold),
        0.0f,
        EffectActivationThreshold);
    const bool bNeedsRainFallback = !RainEffect || RainEffect->GetAsset() == nullptr;
    const bool bNeedsSnowFallback = !SnowEffect || SnowEffect->GetAsset() == nullptr;
    bFallbackRainActive = bNeedsRainFallback
        && (bFallbackRainActive ? Rain01 >= OffThreshold : Rain01 >= EffectActivationThreshold);
    bFallbackSnowActive = bNeedsSnowFallback
        && (bFallbackSnowActive ? Snow01 >= OffThreshold : Snow01 >= EffectActivationThreshold);
    if (RainFallback) { RainFallback->SetVisibility(bFallbackRainActive, true); }
    if (SnowFallback) { SnowFallback->SetVisibility(bFallbackSnowActive, true); }
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

    const float Twilight01 = TwilightFactor(Daylight01);
    const float Saturation = FMath::Clamp(
        FMath::Lerp(ClearSaturation, SevereWeatherSaturation, WeatherPressure01)
            + TwilightSaturationLift * Twilight01 * (1.0f - WeatherPressure01),
        0.0f,
        2.0f);
    PostProcess->Settings.bOverride_ColorSaturation = true;
    PostProcess->Settings.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.0f);

    const float Contrast = FMath::Lerp(ClearContrast, SevereWeatherContrast, WeatherPressure01);
    PostProcess->Settings.bOverride_ColorContrast = true;
    PostProcess->Settings.ColorContrast = FVector4(Contrast, Contrast, Contrast, 1.0f);

    PostProcess->Settings.bOverride_BloomIntensity = true;
    PostProcess->Settings.BloomIntensity = FMath::Lerp(
        ClearBloomIntensity,
        SevereWeatherBloomIntensity,
        WeatherPressure01);

    PostProcess->Settings.bOverride_VignetteIntensity = true;
    const float NightVignette = (1.0f - Daylight01) * 0.05f;
    PostProcess->Settings.VignetteIntensity = FMath::Clamp(
        FMath::Lerp(0.16f, 0.26f, WeatherPressure01) + NightVignette,
        0.0f,
        1.0f);
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
