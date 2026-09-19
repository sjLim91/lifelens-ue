from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")
header = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")
collision_cpp = (root / "Source/LifeLens/World/LLWorldObstacleCollisionProxyActor.cpp").read_text(encoding="utf-8")
environment_cpp = (root / "Source/LifeLens/WorldPresentation/LLDynamicEnvironmentPresentationActor.cpp").read_text(encoding="utf-8")

# Regression guard: desktop must not rely on a sapling-only catalogue.
for token in (
    "MatureCommon1",
    "MaturePine1",
    "MaturePine4",
    "MatureTwisted2",
    "FlowerBush",
    "FlowerGroup",
    "Grass_Common_Tall",
    "desktop layered nature art",
):
    assert token in cpp, f"missing desktop visual recovery token: {token}"

# Regression guard: structural Engine Cube proxies are Android-only. On desktop
# their component pointers stay null, so visibility drift cannot expose giant
# stretched slabs again.
for token in (
    "FacilityFoundationInstances = nullptr;",
    "FacilityPostInstances = nullptr;",
    "FacilityRoofInstances = nullptr;",
    "FacilityCargoInstances = nullptr;",
):
    assert token in cpp, f"desktop facility proxy still constructible: {token}"

assert "FacilityFoundationInstances->SetVisibility(false, true)" not in cpp, (
    "desktop proxy safety must be construction-time, not a visibility toggle"
)

for token in (
    "TreeMinScale = 0.92f",
    "TreeMaxScale = 1.65f",
    "Desktop construction is allowed to be incomplete",
    "AddPhotorealFurnaceStone",
    "AddPhotorealStructureLog",
):
    assert token in cpp, f"missing runtime visual sanity token: {token}"

# Settlement should remain readable without looking like a huge shaved clearing.
for token in (
    "CoreClearRadiusUU = 520.0f",
    "ActivityRadiusUU = 2200.0f",
    "CoreZoneCanopyKeep = 0.18f",
    "CoreZoneUndergrowthKeep = 0.32f",
    "CoreZoneResourceScale = 0.32f",
    "ActivityZoneResourceScale = 0.62f",
    "FacilityClearRadiusUU = 340.0f",
    "FacilityActivityRadiusUU = 900.0f",
):
    assert token in header, f"missing dense settlement-edge recovery: {token}"

# Ground-detail boulders must participate in readability thinning.
assert "if (Layer == ELLDressingLayer::GroundDetail) { return 1.0f; }" not in cpp
assert "bGroundDetail ? 0.08f" in cpp
assert "bStonePatch ? 5 : 8" in cpp

# Active local terrain must not hard-switch to a dry material section during
# screenshot-driven recovery; that produced giant grey chunk polygons.
terrain_cpp = (root / "Source/LifeLens/WorldPresentation/LLDesktopTerrainPresentationActor.cpp").read_text(encoding="utf-8")
assert "if (GroundGrass) { return GroundGrass; }" in terrain_cpp
assert "if (bDry && GroundDry)" not in terrain_cpp
assert "if (bDry && GroundDry)" not in cpp

# Collision proxies remain physical-only and are doubly render-disabled.
assert "Component.SetHiddenInGame(true);" in collision_cpp
assert "Component.SetVisibility(false, true);" in collision_cpp

# Existing ground assets expose pre-contract weather parameter names. Runtime
# writes both the configured LL_* names and these authored names until assets
# are rebuilt under a single naming contract.
for token in (
    'TEXT("Wetness")',
    'TEXT("Snow")',
    'TEXT("Precipitation")',
    'TEXT("AirTemperatureC")',
):
    assert token in environment_cpp, f"missing ground weather compatibility token: {token}"

# Android retains the lightweight primitive presentation path.
assert "#if PLATFORM_ANDROID" in cpp
assert "PLATFORM_ANDROID" in header
print("LifeLens runtime visual recovery baseline: PASS")
