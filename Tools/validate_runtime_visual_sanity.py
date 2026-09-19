from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")
header = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")

for token in (
    'FacilityFoundationInstances->SetVisibility(false, true)',
    'FacilityPostInstances->SetVisibility(false, true)',
    'FacilityRoofInstances->SetVisibility(false, true)',
    'FacilityCargoInstances->SetVisibility(false, true)',
    'TreeMinScale = 1.10f',
    'TreeMaxScale = 2.20f',
    'Desktop construction is allowed to be incomplete',
    'AddPhotorealFurnaceStone',
    'AddPhotorealStructureLog',
):
    assert token in cpp, f"missing runtime visual sanity token: {token}"

for token in (
    'CoreClearRadiusUU = 480.0f',
    'ActivityRadiusUU = 1800.0f',
    'CoreZoneCanopyKeep = 0.28f',
    'CoreZoneUndergrowthKeep = 0.42f',
):
    assert token in header, f"missing settlement density correction: {token}"

# Android still owns the lightweight primitive presentation path.
assert '#if !PLATFORM_ANDROID' in cpp
assert 'PLATFORM_ANDROID' in header
print("LifeLens runtime visual sanity v1: PASS")
