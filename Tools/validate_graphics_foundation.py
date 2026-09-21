from pathlib import Path
import json
import re

root = Path(__file__).resolve().parents[1]


def read(path):
    return (root / path).read_text(encoding="utf-8")


def require(text, tokens, label):
    for token in tokens:
        assert token in text, f"{label}: missing {token}"


windows = read("Config/Windows/WindowsEngine.ini")
require(windows, (
    "DefaultGraphicsRHI=DefaultGraphicsRHI_DX12",
    "+D3D12TargetedShaderFormats=PCD3D_SM6",
    "r.GenerateMeshDistanceFields=True",
    "r.DynamicGlobalIlluminationMethod=1",
    "r.ReflectionMethod=1",
    "r.Shadow.Virtual.Enable=1",
    "r.AntiAliasingMethod=4",
    "r.Nanite.ProjectEnabled=True",
    "r.Lumen.HardwareRayTracing=False",
), "Windows cinematic renderer")

mac = read("Config/Mac/MacEngine.ini")
require(mac, (
    "r.GenerateMeshDistanceFields=True",
    "r.DynamicGlobalIlluminationMethod=1",
    "r.ReflectionMethod=1",
    "r.Shadow.Virtual.Enable=0",
    "r.AntiAliasingMethod=4",
    "r.Nanite.ProjectEnabled=False",
    "r.Lumen.HardwareRayTracing=False",
    "r.RayTracing=False",
), "macOS desktop cinematic renderer")

profiles = read("Config/DefaultDeviceProfiles.ini")
require(profiles, (
    "[Windows DeviceProfile]",
    "[Mac DeviceProfile]",
    "[Android DeviceProfile]",
), "cross-platform device profiles")

android = read("Config/Android/AndroidEngine.ini")
require(android, (
    "r.GenerateMeshDistanceFields=False",
    "r.DynamicGlobalIlluminationMethod=0",
    "r.ReflectionMethod=0",
    "r.Shadow.Virtual.Enable=0",
    "r.Nanite.ProjectEnabled=False",
    "r.Lumen.HardwareRayTracing=False",
    "r.RayTracing=False",
), "Android mobile renderer")

low_spec_launcher = read("Tools/run_lifelens_low_spec.ps1")
require(low_spec_launcher, (
    "[switch]$LegacyD3D11",
    '$RHIArgs = @("-dx12")',
    '"-d3d11", "-NoRHIThread"',
), "Windows low-spec RHI launcher")
assert "A previous helper always forced D3D11" in low_spec_launcher, (
    "low-spec launcher must document why DX12 is the safe default"
)

project = json.loads(read("LifeLens.uproject"))
plugins = {p["Name"] for p in project.get("Plugins", []) if p.get("Enabled")}
for plugin in ("Niagara", "PCG", "Water"):
    assert plugin in plugins, f"Unreal-native graphics plugin disabled: {plugin}"

diagnostics = read("Source/LifeLens/World/LLRendererDiagnosticsSubsystem.cpp")
require(diagnostics, (
    'ReadRendererCVar(TEXT("r.DynamicGlobalIlluminationMethod"))',
    'ReadRendererCVar(TEXT("r.ReflectionMethod"))',
    'ReadRendererCVar(TEXT("r.Shadow.Virtual.Enable"))',
    'ReadRendererCVar(TEXT("r.AntiAliasingMethod"))',
    'ReadRendererCVar(TEXT("r.Nanite.ProjectEnabled"))',
    'ReadRendererCVar(TEXT("r.GenerateMeshDistanceFields"))',
    'LifeLens renderer effective tier:',
    'IsCinematicMacTier',
    'LifeLens macOS desktop renderer contract is not fully active at runtime.',
), "runtime renderer diagnostics")

game_mode = read("Source/LifeLens/Core/LLLifeLensGameMode.cpp")
require(game_mode, (
    "FloorComponent->SetHiddenInGame(true, true);",
    "FloorComponent->SetCastShadow(false);",
    "ALLWaterPresentationActor",
), "runtime world bootstrap")
assert game_mode.index("FloorComponent->SetStaticMesh(RuntimeFloorMesh);") < game_mode.index(
    "FloorComponent->SetHiddenInGame(true, true);"
), "bootstrap collision floor must be hidden after its mesh is assigned"

water = read("Source/LifeLens/WorldPresentation/LLWaterPresentationActor.cpp")
require(water, (
    "GetMaterializedSurfaceWaterPresentationObservations()",
    "AWaterBodyRiver",
    "AWaterBodyLake",
    "WaterBody->SetActorEnableCollision(false);",
    "Component->SetCanEverAffectNavigation(false);",
    "ELLCoreSurfaceWaterKind::Ocean",
    "marineLocalSurface",
    "bHasMarineNeighbour",
), "authoritative Unreal Water projection")
for token in (
    "SurfaceWaterSignature(",
    "GetCivilizationWorldObservation(0)",
    "Civilization.Facilities",
    "World.InitialCenterGridX",
    "World.InitialCenterGridY",
    "World.InitialChunk.Elevation",
    "Water.bAvailable",
    "Water.CenterGridX",
    "Water.CenterGridY",
    "Water.bLinearChannel",
    "Water.bHasDownstreamTarget",
    "Water.FlowPotential",
):
    assert token in water, f"WaterBody refresh signature misses runtime input: {token}"

water_header = read("Source/LifeLens/WorldPresentation/LLWaterPresentationActor.h")
for token in (
    "TActorIterator<AWaterZone>",
    "bOwnsWaterZone = true",
    "bOwnsWaterZone = false",
    "reusing authored WaterZone",
):
    assert token in water, f"WaterZone singleton/ownership guard missing: {token}"
assert "bool bOwnsWaterZone = false;" in water_header
assert "if (SpawnedWaterZone && bOwnsWaterZone)" in water
assert water.index("TActorIterator<AWaterZone>") < water.index("SpawnActor<AWaterZone>"), (
    "Water presentation must search for an authored WaterZone before spawning one"
)

world_presentation = read(
    "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp")
require(world_presentation, (
    "SM_LL_fir_sapling",
    "SM_LL_pine_sapling_small",
    "SM_LL_boulder_01",
    "SM_LL_shrub_02",
    "SM_LL_shrub_03",
    "SM_LL_weed_plant_02",
), "photoreal natural presentation")
android_split = world_presentation.index("#if PLATFORM_ANDROID")
desktop_split = world_presentation.index("#else", android_split)
split_end = world_presentation.index("#endif", desktop_split)
android_nature = world_presentation[android_split:desktop_split]
desktop_nature = world_presentation[desktop_split:split_end]
assert "/Game/Environment/Quaternius/" in android_nature, (
    "Android presentation must keep an explicit lightweight nature set"
)
# Screenshot-driven recovery: the photoreal-only catalogue currently contains
# saplings but no mature canopy. Permit only the explicitly reviewed, already-
# shipped mature/understory recovery set on desktop; do not reopen an arbitrary
# catch-all fallback path.
allowed_desktop_recovery = {
    "/Game/Environment/Quaternius/StylizedNature/CommonTree_1/StaticMeshes/CommonTree_1.CommonTree_1",
    "/Game/Environment/Quaternius/StylizedNature/CommonTree_2/StaticMeshes/CommonTree_2.CommonTree_2",
    "/Game/Environment/Quaternius/StylizedNature/CommonTree_3/StaticMeshes/CommonTree_3.CommonTree_3",
    "/Game/Environment/Quaternius/StylizedNature/Pine_1/StaticMeshes/Pine_1.Pine_1",
    "/Game/Environment/Quaternius/StylizedNature/Pine_2/StaticMeshes/Pine_2.Pine_2",
    "/Game/Environment/Quaternius/StylizedNature/Pine_3/StaticMeshes/Pine_3.Pine_3",
    "/Game/Environment/Quaternius/StylizedNature/Pine_4/StaticMeshes/Pine_4.Pine_4",
    "/Game/Environment/Quaternius/StylizedNature/TwistedTree_2/StaticMeshes/TwistedTree_2.TwistedTree_2",
    "/Game/Environment/Quaternius/StylizedNature/Bush_Common_Flowers/StaticMeshes/Bush_Common_Flowers.Bush_Common_Flowers",
    "/Game/Environment/Quaternius/StylizedNature/Fern_1/StaticMeshes/Fern_1.Fern_1",
    "/Game/Environment/Quaternius/StylizedNature/Plant_1_Big/StaticMeshes/Plant_1_Big.Plant_1_Big",
    "/Game/Environment/Quaternius/StylizedNature/Grass_Common_Tall/StaticMeshes/Grass_Common_Tall.Grass_Common_Tall",
    "/Game/Environment/Quaternius/StylizedNature/Grass_Wispy_Tall/StaticMeshes/Grass_Wispy_Tall.Grass_Wispy_Tall",
    "/Game/Environment/Quaternius/StylizedNature/Flower_3_Group/StaticMeshes/Flower_3_Group.Flower_3_Group",
    "/Game/Environment/Quaternius/StylizedNature/Clover_1/StaticMeshes/Clover_1.Clover_1",
    "/Game/Environment/Quaternius/StylizedNature/Rock_Medium_1/StaticMeshes/Rock_Medium_1.Rock_Medium_1",
    "/Game/Environment/Quaternius/StylizedNature/Rock_Medium_2/StaticMeshes/Rock_Medium_2.Rock_Medium_2",
    "/Game/Environment/Quaternius/StylizedNature/Rock_Medium_3/StaticMeshes/Rock_Medium_3.Rock_Medium_3",
    "/Game/Environment/Quaternius/StylizedNature/Pebble_Round_2/StaticMeshes/Pebble_Round_2.Pebble_Round_2",
}
desktop_quaternius = set(re.findall(
    r'"/Game/Environment/Quaternius/[^"]+"',
    desktop_nature,
))
desktop_quaternius = {value.strip('"') for value in desktop_quaternius}
assert desktop_quaternius == allowed_desktop_recovery, (
    "desktop Quaternius recovery set drifted; only the screenshot-reviewed "
    f"mature/understory set is allowed: {desktop_quaternius ^ allowed_desktop_recovery}"
)
assert "/Game/Environment/Photoreal/PolyHaven/" not in android_nature, (
    "Android nature block must not hard-reference desktop photoreal dressing"
)

print("LifeLens graphics foundation regression gate: PASS")
