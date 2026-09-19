from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "Source/LifeLens/WorldPresentation/LLDynamicEnvironmentPresentationActor.h").read_text(encoding="utf-8")
cpp = (root / "Source/LifeLens/WorldPresentation/LLDynamicEnvironmentPresentationActor.cpp").read_text(encoding="utf-8")

for token in (
    "NightSkyIntensity = 0.32f",
    "NightExposureBias = 0.30f",
    "MinimumNightExposureBias = 0.18f",
    "StormExposureReduction",
    "Daylight01 < 0.25f",
    "NightFog(0.075f, 0.095f, 0.16f",
):
    assert token in header or token in cpp, f"missing night visibility token: {token}"

assert "NightSunIntensity = 0.0f" in header
print("LifeLens night visibility hotfix: PASS")
