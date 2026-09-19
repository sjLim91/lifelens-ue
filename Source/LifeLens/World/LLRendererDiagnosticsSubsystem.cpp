#include "World/LLRendererDiagnosticsSubsystem.h"

#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProperties.h"

namespace
{
int32 ReadRendererCVar(const TCHAR* Name)
{
    if (const IConsoleVariable* Variable =
        IConsoleManager::Get().FindConsoleVariable(Name))
    {
        return Variable->GetInt();
    }
    return -1;
}

bool IsCinematicWindowsTier(const FLLRendererDiagnosticsSnapshot& Snapshot)
{
    return Snapshot.DynamicGlobalIlluminationMethod == 1
        && Snapshot.ReflectionMethod == 1
        && Snapshot.VirtualShadowMaps == 1
        && Snapshot.AntiAliasingMethod == 4
        && Snapshot.NaniteProjectEnabled == 1
        && Snapshot.GenerateMeshDistanceFields == 1
        && Snapshot.LumenHardwareRayTracing == 0;
}

bool IsMobileSafeTier(const FLLRendererDiagnosticsSnapshot& Snapshot)
{
    return Snapshot.DynamicGlobalIlluminationMethod == 0
        && Snapshot.ReflectionMethod == 0
        && Snapshot.VirtualShadowMaps == 0
        && Snapshot.NaniteProjectEnabled == 0
        && Snapshot.LumenHardwareRayTracing == 0
        && Snapshot.RayTracing == 0;
}
}

void ULLRendererDiagnosticsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const FLLRendererDiagnosticsSnapshot Snapshot = GetRendererDiagnostics();
    UE_LOG(LogTemp, Log, TEXT("LifeLens renderer effective tier: %s"),
        *GetRendererDiagnosticsSummary());

    if (Snapshot.PlatformName.Contains(TEXT("Windows"), ESearchCase::IgnoreCase))
    {
        if (!IsCinematicWindowsTier(Snapshot))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("LifeLens Windows cinematic renderer contract is not fully active at runtime."));
        }
    }
    else if (Snapshot.PlatformName.Contains(TEXT("Android"), ESearchCase::IgnoreCase))
    {
        if (!IsMobileSafeTier(Snapshot))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("LifeLens Android renderer booted with an unexpected high-cost feature enabled."));
        }
    }
}

FLLRendererDiagnosticsSnapshot ULLRendererDiagnosticsSubsystem::GetRendererDiagnostics() const
{
    FLLRendererDiagnosticsSnapshot Snapshot;
    Snapshot.PlatformName = FPlatformProperties::PlatformName();
    Snapshot.DynamicGlobalIlluminationMethod =
        ReadRendererCVar(TEXT("r.DynamicGlobalIlluminationMethod"));
    Snapshot.ReflectionMethod = ReadRendererCVar(TEXT("r.ReflectionMethod"));
    Snapshot.VirtualShadowMaps = ReadRendererCVar(TEXT("r.Shadow.Virtual.Enable"));
    Snapshot.AntiAliasingMethod = ReadRendererCVar(TEXT("r.AntiAliasingMethod"));
    Snapshot.NaniteProjectEnabled = ReadRendererCVar(TEXT("r.Nanite.ProjectEnabled"));
    Snapshot.GenerateMeshDistanceFields = ReadRendererCVar(TEXT("r.GenerateMeshDistanceFields"));
    Snapshot.LumenHardwareRayTracing = ReadRendererCVar(TEXT("r.Lumen.HardwareRayTracing"));
    Snapshot.RayTracing = ReadRendererCVar(TEXT("r.RayTracing"));
    Snapshot.MobileHDR = ReadRendererCVar(TEXT("r.MobileHDR"));
    return Snapshot;
}

FString ULLRendererDiagnosticsSubsystem::GetRendererDiagnosticsSummary() const
{
    const FLLRendererDiagnosticsSnapshot Snapshot = GetRendererDiagnostics();
    return FString::Printf(
        TEXT("platform=%s gi=%d reflection=%d vsm=%d aa=%d nanite=%d distanceFields=%d lumenHWRT=%d rayTracing=%d mobileHDR=%d"),
        *Snapshot.PlatformName,
        Snapshot.DynamicGlobalIlluminationMethod,
        Snapshot.ReflectionMethod,
        Snapshot.VirtualShadowMaps,
        Snapshot.AntiAliasingMethod,
        Snapshot.NaniteProjectEnabled,
        Snapshot.GenerateMeshDistanceFields,
        Snapshot.LumenHardwareRayTracing,
        Snapshot.RayTracing,
        Snapshot.MobileHDR);
}
