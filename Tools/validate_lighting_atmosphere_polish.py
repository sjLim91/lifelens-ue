from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/WorldPresentation/LLDynamicEnvironmentPresentationActor.cpp").read_text(encoding="utf-8")
header = (root / "Source/LifeLens/WorldPresentation/LLDynamicEnvironmentPresentationActor.h").read_text(encoding="utf-8")

for token in (
    "PresentAngle",
    "PresentedSunElevationDegrees",
    "PresentedSunAzimuthDegrees",
    "PresentedSunIntensity01",
    "PresentedSkyBrightness01",
    "PresentedFogAmount01",
    "FMath::FindDeltaAngleDegrees",
    "SetVolumetricFogExtinctionScale",
    "SetVolumetricFogScatteringDistribution",
):
    assert token in cpp or token in header, f"missing lighting/atmosphere polish token: {token}"

assert "GetInitialRegionSkyPresentationObservation" in cpp
assert "SolarElevationDegrees =" not in cpp
assert "#if PLATFORM_WINDOWS || PLATFORM_MAC" in cpp
assert "HeightFog->SetVolumetricFog(false);" in cpp

# Activation/deactivation hysteresis applies to authored Niagara too, not just
# the QA primitive fallback. Near-threshold weather must not pop every sample.
for token in (
    "ShouldRemainActive",
    "Component->IsActive()",
    "Intensity01 >= OffThreshold",
    "Intensity01 >= EffectActivationThreshold",
    "SetEffectActive(RainEffect, ShouldRemainActive(RainEffect, Rain01))",
    "SetEffectActive(SnowEffect, ShouldRemainActive(SnowEffect, Snow01))",
    "SetEffectActive(FogEffect, ShouldRemainActive(FogEffect, Fog01))",
):
    assert token in cpp, f"missing authored-weather hysteresis guard: {token}"

print("LifeLens lighting/atmosphere polish v1: PASS")
